
# This Makefile provides minimum set of tools required to
# build a libwebrtc linked binary. You will need the following environment
# to properly make build done.
#   - Linux (recommended: Ubuntu 18.04 LTS)
#   - Vulkan Driver Support
#   - Libraries:
#     - Boost 1.7.1 or above (just headers needed)
#     - OpenGL Mathematics libglm-dev 0.9 or above
#     - Vulkan SDK, libvulkan-dev 1.0 or above
# Build steps:
#   1. In advance, requires libwebrtc compiled using Chromium build toolchain.
#      Use the following git commit hash when you checkout branch in order to
#      ensure that we use the same version of libwebrtc.
#        cc1b32545db7823b85f5a83a92ed5f85970492c9
#      For more information of WebRTC build instructions,
#      see https://webrtc.org/native-code/development/
#   2. Specify the WebRTC /src directory path to `WEBRTC_ROOT`
#   3. Make sure that `WEBRTC_LIB_ROOT` matches the toolchain generated directory
#   4. make -j

WEBRTC_ROOT=/home/user/dev/git/webrtc/src
WEBRTC_LIB_ROOT=$(WEBRTC_ROOT)/out/Default

TARGET=main

BUILD_DIR=build
SOURCE_DIR=src

INCLUDES=-I$(WEBRTC_ROOT) \
	-I$(WEBRTC_ROOT)/third_party/abseil-cpp \
	-I$(WEBRTC_ROOT)/third_party/libyuv/include \
	-I$(SOURCE_DIR)

ISYSTEM_LIBCPP=-isystem$(WEBRTC_ROOT)/buildtools/third_party/libc++/trunk/include

CXX=$(WEBRTC_ROOT)/third_party/llvm-build/Release+Asserts/bin/clang++
CXXFLAGS=-Wno-macro-redefined \
	-O2 \
	-pthread \
	-DWEBRTC_LINUX \
	-DWEBRTC_POSIX \
	-D_LIBCPP_ABI_UNSTABLE \
	-fno-lto \
	-std=c++11 \
	-nostdinc++ \
	$(ISYSTEM_LIBCPP) \
	$(INCLUDES)

LD=$(CXX)
LDFLAGS_LINUX=-ldl -lX11
LDFLAGS_VULKAN=-lvulkan
LDFLAGS=-lpthread $(LDFLAGS_LINUX) $(LDFLAGS_VULKAN)

LIBWEBRTC_A=$(BUILD_DIR)/libwebrtc.a
LIBS=$(LIBWEBRTC_A)

AR=$(WEBRTC_ROOT)/third_party/llvm-build/Release+Asserts/bin/llvm-ar

SOURCES=$(shell find $(SOURCE_DIR) -name '*.cc')
OBJECTS=$(patsubst $(SOURCE_DIR)/%.cc, $(BUILD_DIR)/%.o, $(SOURCES))
HEADERS=$(shell find $(SOURCE_DIR) -name '*.h') $(shell find $(SOURCE_DIR) -name '*.inc')

.PHONY: all
all: $(TARGET)

.PHONY: shader
shader:
	glslangValidator -o shaders/frag.spv -V shaders/shader.frag
	glslangValidator -o shaders/vert.spv -V shaders/shader.vert 

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
	rm -f $(TARGET)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cc $(HEADERS)
	@mkdir -p "$(@D)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) $(LIBS)
	$(LD) -o $(TARGET) $(OBJECTS) $(LIBS) $(LDFLAGS)

$(LIBWEBRTC_A): $(shell find $(WEBRTC_LIB_ROOT)/obj -name '*.o')
	$(AR) -r $@ $^
