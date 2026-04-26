# md1img_repacker

`md1img.img` is a binary image file containing the MD1 (Modem 1) component found in MediaTek-based devices.
MD1 is responsible for cellular communication and data transfer (3G/4G/5G).
The `md1img.img` file is part of MTK firmware packages and is used when updating or flashing the modem.

The tool automatically decompresses embedded `.gz` and `.xz`/`.lzma` files on the fly during unpacking,
and recompresses them on packing using the same format and compression parameters as the original image.

## Usage

```
md1img_repacker <pack|unpack> <input> [output_dir]
```

**Unpack:**
```bash
./md1img_repacker unpack ./md1img.img ./img_unpacked
```

**Pack:**
```bash
./md1img_repacker pack ./img_unpacked
```

---

## Building

### Requirements

All dependencies (zlib and liblzma from XZ Utils) are downloaded and built automatically
from their official sources via CMake FetchContent — no manual installation required.

You will need:
- CMake ≥ 3.28
- Git (for FetchContent)
- A C++20 compiler: GCC ≥ 10, Clang ≥ 11, or MSVC 2019+

---

### Linux / macOS

```bash
git clone https://github.com/blackeangel/md1img_repacker.git
cd md1img_repacker
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The binary will be placed in `out/md1img_repacker`.

---

### Android / Termux (native build)

```bash
pkg install cmake git clang
git clone https://github.com/blackeangel/md1img_repacker.git
cd md1img_repacker
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

> **Note:** Termux uses the Android bionic libc, which has no static variant by design.
> The resulting binary will be partially dynamic (`libc.so`), but `zlibstatic`, `liblzma`
> and `libc++` are linked statically.

---

### Android (cross-compilation via NDK)

Produces a fully static binary on Linux/macOS for running on Android devices.

**Requires:** Android NDK r21+. Download from
[developer.android.com](https://developer.android.com/ndk/downloads)
or via Android Studio SDK Manager.

```bash
export NDK=~/Android/Sdk/ndk/27.0.12077973   # path to your NDK

git clone https://github.com/blackeangel/md1img_repacker.git
cd md1img_repacker

cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build -j$(nproc)
```

| Parameter | Value | Description |
|---|---|---|
| `ANDROID_ABI` | `arm64-v8a` | 64-bit devices (most modern devices) |
| | `armeabi-v7a` | 32-bit devices |
| | `x86_64` | emulator |
| `ANDROID_PLATFORM` | `android-21` | minimum API level (Android 5.0) |
| `ANDROID_STL` | `c++_static` | statically linked C++ STL |

Copy the binary to your device and make it executable:

```bash
adb push out/md1img_repacker /data/local/tmp/
adb shell chmod +x /data/local/tmp/md1img_repacker
```

---

### Windows

```bat
git clone https://github.com/blackeangel/md1img_repacker.git
cd md1img_repacker
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j%NUMBER_OF_PROCESSORS%
```

The binary will be placed in `out/md1img_repacker.exe`.

> For a fully static build on Windows, use a MinGW-w64 toolchain and add
> `-G "MinGW Makefiles"` to the cmake command.
