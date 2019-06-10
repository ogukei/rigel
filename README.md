# Rigel (ALPHA)

Stadia-like realtime graphics streaming software using WebRTC and Vulkan

Source code for Rigel-compatible signaling server is also available, written in Go:

https://github.com/ogukei/belt

## Build

The Makefile provides minimum set of tools required to
build a libwebrtc linked binary. You will need the following environment
to properly make build done.

- Linux (recommended: Ubuntu 18.04 LTS)
- Vulkan Driver Support
- Libraries:
    - Boost 1.7.1 or above (just headers needed)
    - OpenGL Mathematics libglm-dev 0.9 or above
    - Vulkan SDK, libvulkan-dev 1.0 or above

1. In advance, requires libwebrtc compiled using Chromium build toolchain.
  Use the following git commit hash when you checkout branch in order to
  ensure that we use the same version of libwebrtc.

    ```2a1d802a35ad3d7c2a8371ce0d4263ee2476e81e```

    For more information of WebRTC build instructions,
    see https://webrtc.org/native-code/development/

2. Specify the WebRTC /src directory path to `WEBRTC_ROOT`
3. Make sure that `WEBRTC_LIB_ROOT` matches the toolchain generated directory
4. `make -j`

## Links to similar projects

- WebRTC Native Client Momo
https://github.com/shiguredo/momo

