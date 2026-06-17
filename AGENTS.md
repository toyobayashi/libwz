# AGENTS.md

## Purpose
This file gives coding agents a reliable operating guide for `libwz`.
Use it to make focused, low-risk changes that match this codebase.

## Project Snapshot
- Language: C++23 (also provides C API and JNI bindings)
- Build: CMake 3.16+
- Target libraries: `wz` (static), `wz_capi` (static), `wz_jni` (shared)
- Test framework: Google Test
- Reference C# project: `Harepacker-resurrected/` submodule (HaRepacker/MapleLib)
- Purpose: Parse and read MapleStory `.wz` game data archives

## Environment Requirements
- C++23 compiler with `<expected>` support (MSVC on Windows, GCC/Clang on Linux)
- CMake 3.16+
- Git with submodule support

## Setup
```powershell
git submodule update --init --recursive
```

## Build

Windows:

```bat
cmake -H. -Bbuild -G "Visual Studio 17 2022" -A x64 -DBUILD_JNI=ON
cmake --build build --config Debug
```

Linux / macOS

```bash
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug -DBUILD_JNI=ON
cmake --build build
```

## lint

```bash
cpplint --recursive --config .cpplint src include
```

## Test
```powershell
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Tests require sample `.wz` files from the `Harepacker-resurrected/UnitTest_WzFile/WzFiles/` submodule.

## Code Map
- `include/wz/`: Public headers (core types, properties, utilities)
  - `WzFile.h` / `WzDirectory.h` / `WzImage.h`: Archive structure parsing
  - `WzImageProperty.h`: Base property class and `IPropertyContainer` interface
  - `Properties/`: 16 concrete property types (sub, canvas, vector, convex, PNG, UOL, Lua, scalar types, etc.)
  - `Util/`: Binary reader, AES key generation, mutable key, utility functions
  - `capi/wz_api.h`: C ABI for FFI consumers
  - `Result.h`: `std::expected`-based `Result<T, Error>` (no exceptions)
- `src/`: Implementation files mirroring `include/` structure
- `src/jni/wz_jni.cpp`: JNI bindings for Java/Kotlin consumers
- `tests/`: Google Test unit tests
- `Harepacker-resurrected/`: Upstream C# reference (submodule, test data only)

## Key Patterns
- **Error handling**: `Result<T, Error>` types alias `std::expected<T, Error>`, no C++ exceptions (`-fno-exceptions`). Use standard expected API (`has_value()`, `value()`, `error()`, `std::unexpected`).
- **Memory**: Raw pointers with parent-child ownership. Owners call `Dispose()` to free children.
- **Naming**: All types in `wz::` namespace, `Wz` prefix for classes. `#ifndef`-style header guards (e.g. `WZ_WZFILE_H_`).
- **C API**: Opaque handles (`wz_file`, `wz_image`, etc.), no STL in public signatures.

## Working Rules For Agents
- Keep changes minimal and scoped to the request.
- Do not refactor unrelated files in the same pass.
- Avoid broad formatting-only edits.
- Preserve existing `#include` grouping and namespace structure.
- Do not introduce exceptions — use `Result<T, Error>` for all error handling.
- When changing parsing/decryption logic, prefer adding or updating tests in `tests/`.
- When adding new property types, follow the existing pattern in `include/wz/Properties/` and `src/Properties/`.
- When committing, include only files added or modified by the agent in this task; do not include unrelated pre-existing uncommitted changes.
- Use Google C++ Style, reference .clang-format file in the project root.
- Run format and lint every time you change the C++ code.
- **C# reference rule**: Study the real C# source (`Harepacker-resurrected/MapleLib/`) to understand the WZ parsing algorithms, then reimplement the same behavior in C++ from scratch. This is a clean-room approach: read and understand, but never copy code text. Do not guess or invent new behavior — match the reference output. If the C# behavior seems wrong, flag it but implement as-is.
  - Run format and then lint after every C++ change. Both must pass with zero errors.
  - Type names follow C++ (`wz::WzFile`, `wz::WzImageProperty`), Java 封装类使用小写开头的驼峰命名（`parseWzFile()`、`getWzDirectory()`）。

## High-Risk Areas (Extra Caution)
- WZ parsing/decryption and AES crypto paths in `src/WzFile.cpp`, `src/WzDirectory.cpp`, `src/WzImage.cpp`
- Image decompression in `src/PngUtility.cpp` (DXT3/DXT5/RGB565 codepaths)
- Binary reader offset tracking in `src/Util/WzBinaryReader.cpp`
- Key generation/rotation in `src/Util/WzKeyGenerator.cpp` and `src/Util/WzMutableKey.cpp`
- Cross-platform wraparound for 64-bit WZ offsets
