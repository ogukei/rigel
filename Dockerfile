
# Requires Docker 19.03.8 or above (via apt recommended)
# @see https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository
# Requires NVIDIA Container Toolkit (nvidia-container-toolkit)
# @see https://github.com/NVIDIA/nvidia-docker

# Host Usage:
# cd rigel
# sudo docker build -t image-rigel .
# sudo docker run --gpus all --network="host" --name rigel -d image-rigel
# google-chrome http://localhost:8080

# Host Usage: (Interactive for Debug)
# sudo docker run --gpus all --rm -it --network="host" --entrypoint=/bin/bash image-rigel

FROM nvidia/cudagl:10.1-base-ubuntu18.04
MAINTAINER Keitaro Oguri "ogukei256@gmail.com"

# GPU Driver
ENV NVIDIA_DRIVER_CAPABILITIES compute,graphics,utility

RUN apt-get update && apt-get install -y --no-install-recommends \
    libx11-xcb-dev \
    libxkbcommon-dev \
    libwayland-dev \
    libxrandr-dev \
    libegl1-mesa-dev && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /etc/vulkan/icd.d
RUN echo '{"file_format_version":"1.0.0","ICD":{"library_path":"libGLX_nvidia.so.0","api_version":"1.1.99"}}' >> /etc/vulkan/icd.d/nvidia_icd.json

# Vulkan SDK
# https://vulkan.lunarg.com/sdk/home#linux
RUN apt-get update && apt-get install -qy wget
RUN wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
RUN wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.1.130-bionic.list http://packages.lunarg.com/vulkan/1.1.130/lunarg-vulkan-1.1.130-bionic.list
RUN apt-get update && apt-get install -qy vulkan-sdk && rm -rf /var/lib/apt/lists/*

# Boost CXX (Clang)
RUN apt-get update && apt-get install -qy git build-essential clang curl && rm -rf /var/lib/apt/lists/*

# Boost 1.71.0 (headers)
WORKDIR /tmp
RUN apt-get update && apt-get install -qy wget && rm -rf /var/lib/apt/lists/*
RUN wget -qO boost_1_71_0.tar.bz2 https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.bz2
RUN tar --bzip2 -xf boost_1_71_0.tar.bz2
WORKDIR /tmp/boost_1_71_0
RUN ./bootstrap.sh --prefix=/usr/local
RUN ./b2 install --with-headers
WORKDIR /tmp
RUN rm -rf boost_1_71_0 && rm -f boost_1_71_0.tar.bz2

# GLM
RUN apt-get update && apt-get install -qy libglm-dev && rm -rf /var/lib/apt/lists/*

# Chromium
# https://hub.docker.com/r/dockerview/chromium-builder/dockerfile
RUN apt-get update
RUN apt-get install -qy git build-essential clang curl
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -qy tzdata
#RUN apt-get install -qy lsb-release sudo

# Setup user
RUN useradd -m user
USER user
ENV HOME /home/user
WORKDIR /home/user

# Install Chromium's depot_tools.
# https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up
ENV DEPOT_TOOLS /home/user/depot_tools
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $DEPOT_TOOLS
ENV PATH $DEPOT_TOOLS:$PATH
RUN echo "" >> .bashrc
RUN echo "# Add Chromium's depot_tools to the PATH." >> .bashrc
RUN echo "export PATH=\"$DEPOT_TOOLS:\$PATH\"" >> .bashrc

# https://webrtc.googlesource.com/src/+/refs/heads/master/docs/native-code/development/prerequisite-sw/index.md
# https://webrtc.googlesource.com/src/+/refs/heads/master/docs/native-code/development/index.md
RUN mkdir -p /home/user/webrtc-checkout
WORKDIR /home/user/webrtc-checkout
RUN fetch --nohooks webrtc
RUN gclient sync

# install-build-deps
# A script is provided for Ubuntu, which is unfortunately only available after your first gclient sync
USER root
RUN apt-get update && apt-get install -qy lsb-release sudo
RUN /home/user/webrtc-checkout/src/build/install-build-deps.sh --no-prompt --no-chromeos-fonts

# Build libwebrtc (M75)
# http://webrtc.github.io/webrtc-org/native-code/development/
USER user
WORKDIR /home/user/webrtc-checkout/src
RUN gclient sync -r branch-heads/m75
RUN gn gen out/Default --args='target_os="linux" is_debug=false rtc_include_tests=false is_component_build=false use_rtti=true rtc_use_dummy_audio_file_devices=true'
RUN ninja -C out/Default

# Install Golang
USER root
RUN apt-get install -qy golang-1.10
ENV PATH $PATH:/usr/lib/go-1.10/bin

# Build Belt
USER user
WORKDIR /home/user
RUN git clone https://github.com/ogukei/belt belt
WORKDIR /home/user/belt
RUN go get "github.com/google/uuid"
RUN go get "github.com/gorilla/websocket"
RUN go build -o main src/*.go

# Build Rigel
RUN mkdir -p /home/user/rigel
COPY . /home/user/rigel
WORKDIR /home/user/rigel

ENV WEBRTC_ROOT /home/user/webrtc-checkout/src
RUN make -j

# Run Setup
WORKDIR /home/user/rigel
EXPOSE 8080

ENTRYPOINT ["/bin/bash"]
CMD ["-c", "((../belt/main) &) && (sleep 1 && ./main)"]
