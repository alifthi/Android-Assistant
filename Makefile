APP_NAME ?= llama_cli
SRC := native/llama_cli.cpp

BUILD_DIR := build
OBJ := $(BUILD_DIR)/llama_cli.o
OUT := $(BUILD_DIR)/$(APP_NAME)
LIBCXX_SHARED_OUT := $(BUILD_DIR)/libc++_shared.so

ANDROID_ABI ?= arm64-v8a
ANDROID_API ?= 21
NDK ?= $(ANDROID_NDK_HOME)
NDK ?= $(ANDROID_NDK_ROOT)
NDK ?= $(NDK_HOME)

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
  ifeq ($(UNAME_M),arm64)
    HOST_TAG ?= darwin-arm64
  else
    HOST_TAG ?= darwin-x86_64
  endif
else
  HOST_TAG ?= linux-x86_64
endif

ifeq ($(strip $(NDK)),)
$(error NDK not set. Export ANDROID_NDK_HOME (or ANDROID_NDK_ROOT/NDK_HOME).)
endif

NDK_TOOLCHAIN := $(NDK)/toolchains/llvm/prebuilt/$(HOST_TAG)

ifeq ($(ANDROID_ABI),arm64-v8a)
  TARGET_TRIPLE := aarch64-linux-android
else ifeq ($(ANDROID_ABI),armeabi-v7a)
  TARGET_TRIPLE := armv7a-linux-androideabi
else ifeq ($(ANDROID_ABI),x86_64)
  TARGET_TRIPLE := x86_64-linux-android
else ifeq ($(ANDROID_ABI),x86)
  TARGET_TRIPLE := i686-linux-android
else
$(error Unsupported ANDROID_ABI: $(ANDROID_ABI))
endif

CXX := $(NDK_TOOLCHAIN)/bin/$(TARGET_TRIPLE)$(ANDROID_API)-clang++

INCLUDES := -Iinclude
CXXFLAGS := -O2 -g -fPIE -fPIC -DANDROID -std=c++17 $(INCLUDES)
LDFLAGS := -pie

LIBCXX_SHARED_CANDIDATES := \
	$(NDK_TOOLCHAIN)/sysroot/usr/lib/$(TARGET_TRIPLE)/libc++_shared.so \
	$(NDK_TOOLCHAIN)/sysroot/usr/lib/$(TARGET_TRIPLE)/$(ANDROID_API)/libc++_shared.so
LIBCXX_SHARED := $(firstword $(wildcard $(LIBCXX_SHARED_CANDIDATES)))

ifeq ($(strip $(LIBCXX_SHARED)),)
$(error libc++_shared.so not found in NDK sysroot for $(TARGET_TRIPLE) (API $(ANDROID_API)))
endif

.PHONY: all clean print-vars

all: $(OUT) deps

$(BUILD_DIR):
	mkdir -p $@

$(OBJ): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

$(LIBCXX_SHARED_OUT): | $(BUILD_DIR)
	cp $(LIBCXX_SHARED) $@

deps: $(LIBCXX_SHARED_OUT)

clean:
	rm -rf $(BUILD_DIR)

print-vars:
	@echo "NDK=$(NDK)"
	@echo "HOST_TAG=$(HOST_TAG)"
	@echo "ANDROID_ABI=$(ANDROID_ABI)"
	@echo "ANDROID_API=$(ANDROID_API)"
	@echo "CXX=$(CXX)"
	@echo "LIBCXX_SHARED=$(LIBCXX_SHARED)"
