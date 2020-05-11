# Rigel

Stadia-like realtime graphics streaming software using WebRTC and Vulkan

![Preview](https://user-images.githubusercontent.com/25946200/59385662-d25ae700-8d9f-11e9-9fa6-43368472e115.png)

Source code for Rigel-compatible signaling server is also available, written in Go:

https://github.com/ogukei/belt

## Build using Docker
You will need the following environment.

- Linux (recommended: Ubuntu 18.04 LTS)
- [Docker](https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository) 19.03.8 or above (via apt recommended)
- [NVIDIA Container Toolkit](https://github.com/NVIDIA/nvidia-docker)

```
# make sure we have GPUs enabled in containers 
sudo docker run --gpus all nvidia/cuda:10.0-base nvidia-smi

# building. takes approximately 45~ minutes...
git clone https://github.com/ogukei/rigel
cd rigel
sudo docker build -t image-rigel .

# running
sudo docker run --gpus all --network="host" --name rigel -d image-rigel
google-chrome http://localhost:8080

# clean up
sudo docker stop rigel && sudo docker rm rigel
```


## Build manually

The Makefile provides minimum set of tools required to
build a libwebrtc linked binary. You will need the following environment
to properly make build done.

- Linux (recommended: Ubuntu 18.04 LTS)
- Vulkan Driver Support
- Libraries:
    - Boost 1.70.0 or above (just headers needed)
    - OpenGL Mathematics libglm-dev 0.9 or above
    - Vulkan SDK, libvulkan-dev 1.0 or above

1. In advance, requires libwebrtc compiled using Chromium build toolchain.
  Use the following git commit hash when you checkout branch in order to
  ensure that we use the same version of libwebrtc.

    ```159c16f3ceea1d02d08d51fc83d843019d773ec6```

    For more information of WebRTC build instructions,
    see https://webrtc.org/native-code/development/

    Use the following command to generate Ninja build file.

    ```gn gen out/Default --args='target_os="linux" is_debug=false rtc_include_tests=false is_component_build=false use_rtti=true'```

2. Specify the WebRTC /src directory path to `WEBRTC_ROOT` in Makefile
3. Make sure that `WEBRTC_LIB_ROOT` matches the toolchain generated directory
4. `make -j`

## Links to similar projects

- WebRTC Native Client Momo
https://github.com/shiguredo/momo

