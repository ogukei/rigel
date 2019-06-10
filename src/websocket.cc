
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "websocket.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

extern "C" {
#include <pthread.h>
void *RGLRunLoopThreadEntry(void *state);
}

namespace {

// ref: https://www.boost.org/doc/libs/1_67_0/libs/beast/example/advanced/server/advanced_server.cpp

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
static void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>, public rigel::WebSocketInterface {
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    net::steady_timer timer_;
    int ping_state_;
    std::string host_;
    std::string path_;
    std::vector<boost::shared_ptr<std::string const>> queue_;
    rigel::WebSocketSink *sink_;
public:
    // Resolver and socket require an io_context
    explicit session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc)),
        timer_(net::make_strand(ioc), 
            (std::chrono::steady_clock::time_point::max)()),
        ping_state_(0)
        {}

    ~session() {
        sink_->OnRelease(this);    
    }

    // Start the asynchronous operation
    void run(const std::string &host, 
             const std::string &port, 
             const std::string &path,
             rigel::WebSocketSink *sink) {
        // Save these for later
        host_ = host;
        path_ = path;
        sink_ = sink;
        // Look up the domain name
        resolver_.async_resolve(host, port,
            beast::bind_front_handler(&session::on_resolve, shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if(ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if(ec)
            return fail(ec, "connect");

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async");
            }));
        // Perform the websocket handshake
        ws_.async_handshake(host_, path_,
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void on_handshake(beast::error_code ec) {
        if(ec)
            return fail(ec, "handshake");
        // start reading
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));

        // Set the control callback. This will be called
        // on every incoming ping, pong, and close frame.
        ws_.control_callback(
            std::bind(
                &session::on_control_callback,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));

        timer_.expires_after(std::chrono::seconds(1));
        // start timer for ping
        timer_.async_wait(
            net::bind_executor(
                timer_.get_executor(),
                std::bind(
                    &session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
        // callback
        sink_->OnHandshake(this);
    }

    void on_control_callback(websocket::frame_type kind, boost::beast::string_view payload) {
        boost::ignore_unused(kind, payload);

        if (kind == websocket::frame_type::pong) {
            //std::cout << "PONG" << std::endl;
            ping_state_ = 0;
        }
    }

    // Called when the timer expires.
    void on_timer(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) return;
        if (ec) {
            return fail(ec, "timer");
        }
        // See if the timer really expired since the deadline may have moved.
        if (timer_.expiry() <= std::chrono::steady_clock::now()) {
            if (ws_.is_open() && ping_state_ == 0) {
                // Note that we are sending a ping
                ping_state_ = 1;
                // Now send the ping
                ws_.async_ping({},
                    boost::asio::bind_executor(
                        ws_.get_executor(),
                        std::bind(
                            &session::on_ping,
                            shared_from_this(),
                            std::placeholders::_1)));
            }
        }
        // next tick
        timer_.expires_after(std::chrono::seconds(15));
        timer_.async_wait(
            net::bind_executor(
                timer_.get_executor(),
                std::bind(
                    &session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void on_ping(beast::error_code ec) {
        // Happens when the timer closes the socket
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (ec)
            return fail(ec, "ping");

        //std::cout << "PING" << std::endl;
        if (ping_state_ == 1) {
            ping_state_ = 2;
        }
    }

    void send(boost::shared_ptr<std::string const> const& ss) {
        net::post(
            ws_.get_executor(),
            beast::bind_front_handler(
                &session::on_send,
                shared_from_this(),
                ss));
    }

    void on_send(boost::shared_ptr<std::string const> const& ss) {
        // Always add to queue
        queue_.push_back(ss);

        // Are we already writing?
        if(queue_.size() > 1)
            return;

        // We are not currently writing, so send this immediately
        ws_.async_write(
            net::buffer(*queue_.front()),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        if(ec)
            return fail(ec, "write");

        // Remove the string from the queue
        queue_.erase(queue_.begin());

        // Send the next message if any
        if(!queue_.empty())
            ws_.async_write(
                net::buffer(*queue_.front()),
                beast::bind_front_handler(
                    &session::on_write,
                    shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if(ec) {
            timer_.cancel();
            return fail(ec, "read");
        }
        //
        auto string = beast::buffers_to_string(buffer_.data());
        // Clear the buffer
        buffer_.consume(buffer_.size());
        // notify the sink
        sink_->OnReceiveMessage(this, string);

        // read another
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void on_close(beast::error_code ec) {
        if(ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer_.data()) << std::endl;
    }

    // WebSocketInterface
    virtual void SendMessage(const std::string &str) {
        send(boost::make_shared<std::string const>(str));
    }
};

class RunLoop: public rigel::WebSocketInstance {
  boost::asio::io_context io_;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
  pthread_t thread_;
  // internal state

public:
  RunLoop() :
    io_(), work_(boost::asio::make_work_guard(io_)) {
    // isolate thread
    pthread_create(&thread_, nullptr, RGLRunLoopThreadEntry, this);
  }

  virtual ~RunLoop() {
    work_.reset();
    pthread_join(thread_, NULL);
  }

  void Run() {
    io_.run();
  }

  void DoFire(std::string host, std::string port,
      std::string path, rigel::WebSocketSink *sink) {
    rigel::RGLLaunchWebSocketSession(host, port, path, sink);
  }

  void Fire(const std::string &host, const std::string &port,
      const std::string &path, rigel::WebSocketSink *sink) {
    boost::asio::post(io_.get_executor(), 
      boost::bind(&RunLoop::DoFire, this, host, port, path, sink));
  }
};

}  // unnamed namespace

void *RGLRunLoopThreadEntry(void *state) {
  static_cast<RunLoop *>(state)->Run();
  return 0;
}

// export
namespace rigel {

void RGLLaunchWebSocketSession(const std::string &host,
    const std::string &port,
    const std::string &path, WebSocketSink *sink) {
  // The io_context is required for all I/O
  net::io_context ioc;

  // Launch the asynchronous operation
  std::make_shared<session>(ioc)->run(host, port, path, sink);

  // Run the I/O service. The call will return when
  // the socket is closed.
  ioc.run();
}

std::unique_ptr<WebSocketInstance> RGLAsyncLaunchWebSocketSession(
    const std::string &host, const std::string &port,
    const std::string &path, WebSocketSink *sink) {
  auto *run_loop = new RunLoop();
  run_loop->Fire(host, port, path, sink);
  return std::unique_ptr<WebSocketInstance>(std::move(run_loop));
}

}  // namespace rigel
