Android-Assistant (llama_cli)
=============================

Minimal Android NDK build of a `llama.cpp`-based CLI that runs an interactive
chat loop on-device. This repo builds a single native binary plus the NDK
`libc++_shared.so` runtime.

**Requirements**
- Android NDK installed (r21+ should work; uses LLVM toolchain)
- `adb` for pushing/running on a device or emulator
- A GGUF model file compatible with `llama.cpp`

**Build**
1. Set the NDK path (one of these is required). Example:
```sh
export ANDROID_NDK_HOME=/path/to/android-ndk
```
You can also use `ANDROID_NDK_ROOT` or `NDK_HOME` instead of `ANDROID_NDK_HOME`.

2. Optional build tuning (defaults shown):
```sh
export ANDROID_ABI=arm64-v8a
export ANDROID_API=21
```

3. Build:
```sh
make
```

Outputs:
- `build/llama_cli`
- `build/libc++_shared.so`

**Run (device/emulator)**
1. Push the binaries and model:
```sh
adb push build/llama_cli /data/local/tmp/
adb push build/libc++_shared.so /data/local/tmp/
adb push /path/to/model.gguf /data/local/tmp/model.gguf
```
2. Run on device:
```sh
adb shell
cd /data/local/tmp
chmod 755 llama_cli
export LD_LIBRARY_PATH=/data/local/tmp
./llama_cli
```
3. Type a prompt and press Enter. Type `exit` to quit.

**Configuration**
Model path is currently fixed at compile time via `include/config.h`. The
default is `MODEL_PATH "/data/local/tmp/model.gguf"`.

You can also tune `N_CTX`, `N_BATCH`, `N_THREADS`, and `N_GPU_LAYERS` in
`include/config.h`.

If you want `-m /path/to/model.gguf` to work at runtime, update `native/llama_cli.cpp`
to pass the CLI argument into `load_model(...)` instead of `MODEL_PATH`.
