# libwz

> ⚠️ **Vibe Coding / Clean-room Reimplementation / Under Development**
> This project was developed with significant AI assistance (GitHub Copilot).
> It is an independent C++ reimplementation — no code was copied from the
> C# reference (included as a submodule for test data only).

A native C++ library for reading and parsing **MapleStory `.wz` game data files**.
Provides C API for FFI consumers and JNI bindings for Java/Kotlin.

<!-- [![Build & Publish](https://github.com/toyobayashi/libwz/actions/workflows/build.yml/badge.svg)](https://github.com/toyobayashi/libwz/actions/workflows/build.yml) -->

## Features

- Decrypt and parse PKG1-format `.wz` archives across all major MapleStory
  regions (GMS, EMS, BMS, Classic, Custom)
- Support for 32-bit and 64-bit WZ file versions
- Hierarchical directory/image traversal with all 16 property types
- Image decompression: DXT3, DXT5, BGRA4444, ARGB1555, RGB565, Form517
- UOL (link) property resolution with cross-file traversal
- Audio extraction (MP3/WAV) from Sound properties
- Lua script decoding
- PNG export from Canvas properties
- C API (`wz_api.h`) for FFI consumers (Python, Node.js, etc.)
- JNI bindings for Java/Kotlin consumers
- Java wrapper classes with `AutoCloseable` resource management
- Node.js native addon and WebAssembly package entries for JavaScript users

## Requirements

- **C++23** compiler with `<expected>` support
- **CMake 3.16+**
- **Git** (for submodules)

## Quick Start — C++

```bash
git clone --recurse-submodules https://github.com/toyobayashi/libwz.git
cd libwz

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## Quick Start — Java (Maven)

Add the dependency (available on Maven Central):

```xml
<dependency>
    <groupId>io.github.toyobayashi</groupId>
    <artifactId>libwz</artifactId>
    <version>0.1.0</version>
</dependency>
```

Alternatively, from GitHub Packages (requires authentication):

```xml
<repository>
    <id>github</id>
    <url>https://maven.pkg.github.com/toyobayashi/libwz</url>
</repository>
```

Authenticate with a [GitHub personal access token](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-apache-maven-registry#authenticating-to-github-packages):

```xml
<!-- ~/.m2/settings.xml -->
<server>
    <id>github</id>
    <username>YOUR_GITHUB_USERNAME</username>
    <password>YOUR_GITHUB_TOKEN</password>
</server>
```

The JAR bundles native libraries for **Windows x86_64**, **Linux x86_64**,
and **macOS ARM64 (Apple Silicon)**. `NativeLibraryLoader` automatically
extracts the correct one at runtime.

## Quick Start — JavaScript / Wasm

The root `libwz` package entry uses the native Node addon in Node.js and the
Wasm build through the package `browser` export in browser bundlers. Both
entries export the same classes and constants, plus one async `init()` function
for the Wasm runtime.

Node.js requires `^20.19.0 || >=22.12.0`. `process.getBuiltinModule()` exists
in Node.js 20.16.0 and 22.3.0, but the package also relies on the modern
`require(esm)` interoperability that is available by default from Node.js
20.19.0 and 22.12.0; Node.js 24.x is covered by the `>=22.12.0` range.

```js
import { init, MapleVersion, WzFile } from "libwz";

await init();
using file = new WzFile("Character.wz", MapleVersion.GMS);
file.parseWzFile();
```

Pass a custom Wasm URL or path when your bundler or CDN serves the binary from
a different location.

```js
import { init } from "libwz";

await init(new URL("./assets/libwz.wasm", import.meta.url));
```

Use `getWzBindingType()` to inspect whether the active backend is `"native"` or
`"wasm"`. If the native addon cannot be loaded, `init()` falls back to Wasm.
In Node.js, pass a Wasm URL or `{ forceWasm: true }` to force the Wasm backend
even when the native addon is available. Path-based APIs still accept normal
host filesystem paths; the Wasm runtime maps them internally.

The package publishes one Wasm binary, `dist/libwz.wasm`, and no synchronous or
main-thread Worker wrapper entry.

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build Google Test unit tests |
| `BUILD_CAPI` | `ON` | Build C ABI wrapper (`libwz_capi`) |
| `BUILD_JNI` | `OFF` | Build JNI shared library (`libwz_jni`) |

## Targets

| Target | Type | Description |
|--------|------|-------------|
| `wz` | Static | Core WZ parsing engine |
| `wz_capi` | Static | C ABI layer for FFI consumers |
| `wz_jni` | Shared | JNI native library for Java |

## Code Map

| Directory | Purpose |
|-----------|---------|
| `include/wz/` | Public C++ headers |
| `include/wz/Properties/` | 16 property type headers |
| `include/wz/Util/` | Binary reader, key gen, path utils |
| `include/wz/wz_api.h` | C ABI (opaque handles, no STL) |
| `src/` | C++ implementation files |
| `src/capi/` | C API implementation |
| `src/jni/` | JNI bindings (`wz_jni.cpp`) |
| `java/` | Java wrapper classes + Maven project |
| `tests/` | Google Test unit tests |
| `Harepacker-resurrected/` | C# reference (submodule, test data only) |

## Dependencies

- [tiny-AES-c](https://github.com/kokke/tiny-AES-c) — AES-256-ECB decryption
- [zlib](https://github.com/madler/zlib) — Image data decompression
- [Google Test](https://github.com/google/googletest) — Unit tests

## License

MIT. The `Harepacker-resurrected/` submodule (test data reference, not part of
libwz source) contains code under MPL-2.0 and GPL-3.0 — see respective
LICENSE files in that submodule.
