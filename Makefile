APP_NAME ?= llama_cli

BUILD_DIR := build
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

CC := $(NDK_TOOLCHAIN)/bin/$(TARGET_TRIPLE)$(ANDROID_API)-clang
CXX := $(NDK_TOOLCHAIN)/bin/$(TARGET_TRIPLE)$(ANDROID_API)-clang++

GGML_CPU_ARCH_CPP :=
GGML_CPU_ARCH_C :=
ifeq ($(ANDROID_ABI),arm64-v8a)
  GGML_CPU_ARCH_CPP += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/arm/*.cpp)
  GGML_CPU_ARCH_C += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/arm/*.c)
else ifeq ($(ANDROID_ABI),armeabi-v7a)
  GGML_CPU_ARCH_CPP += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/arm/*.cpp)
  GGML_CPU_ARCH_C += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/arm/*.c)
else ifeq ($(ANDROID_ABI),x86_64)
  GGML_CPU_ARCH_CPP += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/x86/*.cpp)
  GGML_CPU_ARCH_C += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/x86/*.c)
else ifeq ($(ANDROID_ABI),x86)
  GGML_CPU_ARCH_CPP += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/x86/*.cpp)
  GGML_CPU_ARCH_C += $(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/arch/x86/*.c)
endif

SRC_CPP := \
	native/llama_cli.cpp \
	native/inferece.cpp \
	native/prompt.cpp \
	native/states.cpp \
	$(wildcard third_party/llama.cpp/src/*.cpp) \
	$(wildcard third_party/llama.cpp/src/models/*.cpp) \
	$(wildcard third_party/llama.cpp/ggml/src/*.cpp) \
	$(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/*.cpp) \
	$(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/amx/*.cpp) \
	$(GGML_CPU_ARCH_CPP)

SRC_C := \
	$(wildcard third_party/llama.cpp/ggml/src/*.c) \
	$(wildcard third_party/llama.cpp/ggml/src/ggml-cpu/*.c) \
	$(GGML_CPU_ARCH_C)

SRC_CPP := $(sort $(SRC_CPP))
SRC_C := $(sort $(SRC_C))

OBJ_CPP := $(patsubst %.cpp,$(BUILD_DIR)/%.cpp.o,$(SRC_CPP))
OBJ_C := $(patsubst %.c,$(BUILD_DIR)/%.c.o,$(SRC_C))
OBJ := $(OBJ_CPP) $(OBJ_C)

DEFS := -DANDROID -DGGML_USE_CPU -DGGML_SCHED_MAX_COPIES=4 -D_GNU_SOURCE -D_XOPEN_SOURCE=600 -DGGML_VERSION=\"0.9.5\" -DGGML_COMMIT=\"unknown\"
INCLUDES := -Iinclude -Ithird_party/llama.cpp/include -Ithird_party/llama.cpp/ggml/include -Ithird_party/llama.cpp/ggml/src -Ithird_party/llama.cpp/ggml/src/ggml-cpu
CPPFLAGS := $(DEFS) $(INCLUDES) -include include/llama_compat.h
CFLAGS := -O2 -g -fPIE -fPIC -std=c11 $(CPPFLAGS)
CXXFLAGS := -O2 -g -fPIE -fPIC -std=c++17 $(CPPFLAGS)
LDFLAGS := -pie -ldl -lm

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

$(BUILD_DIR)/%.cpp.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

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
