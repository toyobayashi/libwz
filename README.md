# libwz

libwz is a C++23 library for reading, traversing, editing, and exporting
MapleStory `.wz` archive data. The core parser is implemented in native C++ and
is exposed through three public surfaces:

- C++: static libraries for the parser and optional C ABI layer
- Java: JNI bindings plus Java wrapper classes
- JavaScript: one ESM package that uses a native Node addon in Node.js and a
  WebAssembly backend when requested or selected by browser bundlers

The project is an independent clean-room reimplementation guided by the
HaRepacker/MapleLib behavior in the `Harepacker-resurrected/` submodule. That
submodule is used as a reference and as a source of test data; source code is not
copied from it.

## Status

This project is under active development. The parser already covers common read,
traversal, image, sound, Lua, UOL, and editing workflows, but APIs may still
change before a stable release.

## Features

- PKG1 `.wz` archive parsing with regional MapleStory keys
- 32-bit and 64-bit WZ version handling
- Directory and image traversal
- Property support for null, numeric, string, sub, canvas, vector, convex,
  sound, raw, UOL, Lua, PNG, and video-like/raw data properties
- UOL and canvas link resolution
- Canvas/PNG export to PNG bytes or files
- Sound export to raw MP3/WAV-compatible bytes or files
- Lua script decoding
- WZ editing and save-to-disk support in native-capable builds
- C ABI layer for FFI consumers
- JNI wrapper with `AutoCloseable` resource management
- JavaScript ESM wrapper with native Node and WebAssembly backends

## Requirements

Core C++ build:

- CMake 3.16+
- C++23 compiler with `<expected>` support
- Git with submodule support

Optional bindings:

- Java 21 and Maven for JNI/JAR builds
- Node.js `^20.19.0 || >=22.12.0` for the JavaScript package
- node-gyp toolchain for the native Node addon
- Emscripten for the WebAssembly build

## Repository Layout

| Path | Purpose |
|------|---------|
| `include/wz/` | Public C++ headers |
| `include/wz/Properties/` | Concrete WZ property types |
| `include/wz/wz_api.h` | C ABI header |
| `src/` | C++ implementation |
| `src/capi/` | C ABI implementation |
| `src/jni/` | JNI native bridge |
| `src/node/` | Node-API / emnapi binding |
| `java/` | Java wrapper and Maven project |
| `js/` | TypeScript JavaScript wrapper source |
| `tests/` | C++ and JavaScript tests |
| `Harepacker-resurrected/` | C# reference and WZ test data submodule |

## Build From Source

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/toyobayashi/libwz.git
cd libwz
```

Build the core C++ libraries:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Build with JNI enabled:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_JNI=ON
cmake --build build --config Release
```

Build the JavaScript package from source:

```bash
npm install
npm run build
```

Build the WebAssembly artifacts:

```bash
npm install
npm run build:wasm
```

Run tests:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build

npm test
```

The parser tests use sample WZ files from
`Harepacker-resurrected/UnitTest_WzFile/WzFiles/`.

## C++ Usage

Include the public headers from `include/wz/` and link the `wz` target.

```cpp
#include <iostream>
#include "wz/WzFile.h"
#include "wz/WzImage.h"
#include "wz/Properties/WzCanvasProperty.h"

int main() {
  wz::WzFile file("Character.wz", wz::WzMapleVersion::GMS);

  if (file.ParseWzFile() != wz::WzFileParseStatus::Success) {
    std::cerr << "failed to parse WZ file\n";
    return 1;
  }

  auto* root = file.GetWzDirectory();
  if (root == nullptr) return 1;

  std::cout << "root: " << root->Name() << "\n";

  for (auto* image : root->WzImages()) {
    auto parsed = image->ParseImage();
    if (!parsed.has_value()) continue;

    for (auto* prop : *image->WzProperties()) {
      std::cout << image->Name() << "/" << prop->Name() << "\n";
    }
  }
}
```

Open a file when you already know the encrypted version or IV:

```cpp
wz::WzFile byVersion("Character.wz", 87, wz::WzMapleVersion::GMS);
wz::WzFile byIv("Character.wz", std::array<uint8_t, 4>{0x4D, 0x23, 0xC7, 0x2B});
```

Look up an object by WZ path:

```cpp
wz::WzObject* obj = file.GetObjectFromPath("Character/00002000.img/info");
if (obj != nullptr) {
  std::cout << obj->FullPath() << "\n";
}
```

Export a canvas or PNG property:

```cpp
auto* image = root->GetImageByName("00002000.img");
if (image != nullptr && image->ParseImage().has_value()) {
  auto* prop = image->GetFromPath("stand/0");
  if (prop != nullptr && prop->PropertyType() == wz::WzPropertyType::Canvas) {
    auto* canvas = static_cast<wz::WzCanvasProperty*>(prop);
    auto* png = canvas->PngProperty();
    if (png != nullptr) {
      auto saved = png->SaveToFile("stand-0.png");
      if (!saved.has_value()) {
        std::cerr << saved.error().message() << "\n";
      }
    }
  }
}
```

Create and save a small WZ file:

```cpp
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzStringProperty.h"

wz::WzFile out(87, wz::WzMapleVersion::GMS);
auto* dir = out.GetWzDirectory();
auto imageResult = dir->CreateImage("Example.img");
if (!imageResult.has_value()) return 1;

auto* image = imageResult.value();
auto added = image->AddProperty(
    std::make_unique<wz::WzStringProperty>("name", "libwz"));
if (!added.has_value()) return 1;

auto saved = out.SaveToDisk("Example.wz");
if (!saved.has_value()) return 1;
```

libwz is built with exceptions disabled. APIs that can fail return
`wz::Result<T>` (`std::expected<T, wz::Error>`) where practical. Check
`has_value()` before using a result.

## Java Usage

The Java layer lives in the Maven project under `java/`. It wraps the JNI bridge
with ordinary Java classes such as `WzFile`, `WzDirectory`, `WzImage`, and
`WzImageProperty`.

Build the JNI library and package the JAR locally:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_JNI=ON
cmake --build build --config Release

cd java
mvn package
```

When consuming a published artifact, the Maven coordinates are:

```xml
<dependency>
    <groupId>io.github.toyobayashi</groupId>
    <artifactId>libwz</artifactId>
    <version>0.1.0</version>
</dependency>
```

Open and traverse a WZ file:

```java
import io.github.toyobayashi.libwz.WzDirectory;
import io.github.toyobayashi.libwz.WzEnums.MapleVersion;
import io.github.toyobayashi.libwz.WzEnums.ParseStatus;
import io.github.toyobayashi.libwz.WzFile;
import io.github.toyobayashi.libwz.WzImage;

public final class ReadWz {
    public static void main(String[] args) {
        try (WzFile file = new WzFile("Character.wz", MapleVersion.GMS)) {
            ParseStatus status = file.parseWzFile();
            if (status != ParseStatus.SUCCESS) {
                throw new IllegalStateException("parse failed: " + status);
            }

            WzDirectory root = file.getWzDirectory();
            System.out.println(root.getName());

            for (WzImage image : root.wzImages()) {
                image.parseImage();
                System.out.println(image.getName());
            }
        }
    }
}
```

Read properties and export a canvas:

```java
import io.github.toyobayashi.libwz.WzCanvasProperty;
import io.github.toyobayashi.libwz.WzFile;
import io.github.toyobayashi.libwz.WzImage;
import io.github.toyobayashi.libwz.WzImageProperty;
import static io.github.toyobayashi.libwz.WzEnums.MapleVersion.GMS;

try (WzFile file = new WzFile("Character.wz", GMS)) {
    file.parseWzFile();

    WzImage image = file.getWzDirectory().getImageByName("00002000.img");
    image.parseImage();

    WzImageProperty prop = image.getFromPath("stand/0");
    if (prop instanceof WzCanvasProperty canvas) {
        canvas.saveToFile("stand-0.png");
    }
}
```

Look up by WZ path:

```java
var object = file.getObjectFromPath("Character/00002000.img/info");
if (object != null) {
    System.out.println(object.getName());
}
```

`WzFile` owns the native file handle and implements `AutoCloseable`; use
try-with-resources. Objects returned from a file, directory, image, or property
are borrowed wrappers and become invalid after the owning file is closed.

## JavaScript Usage

The JavaScript package is ESM-only. Its root entry exports the same wrapper
classes for native Node and WebAssembly:

```js
import {
  init,
  MapleVersion,
  ParseStatus,
  WzBinaryProperty,
  WzCanvasProperty,
  WzFile,
  getWzBindingType
} from 'libwz'
```

When installed from npm after publication:

```bash
npm install libwz
```

When using the repository directly:

```bash
npm install
npm run build
```

### Node.js Native Backend

In Node.js, the package tries to install the native addon when imported. Calling
`init()` is still supported and is a no-op when native loading succeeded.

```js
import { init, MapleVersion, ParseStatus, WzFile } from 'libwz'

await init()

using file = new WzFile('Character.wz', MapleVersion.GMS)
const status = file.parseWzFile()
if (status !== ParseStatus.SUCCESS) {
  throw new Error(`parse failed: ${status}`)
}

const root = file.getWzDirectory()
for (const image of root.wzImages()) {
  image.parseImage()
  console.log(image.getName())
}
```

JavaScript wrappers support explicit resource management through
`[Symbol.dispose]()` and `close()`. If your runtime does not support `using`,
write the cleanup explicitly:

```js
const file = new WzFile('Character.wz', MapleVersion.GMS)
try {
  file.parseWzFile()
  console.log(file.getWzDirectory()?.getName())
} finally {
  file.close()
}
```

### WebAssembly Backend

Force WebAssembly in Node.js:

```js
import { init, MapleVersion, WzFile, getWzBindingType } from 'libwz'

await init({ forceWasm: true })
console.log(getWzBindingType()) // "wasm"

using file = new WzFile('Character.wz', MapleVersion.GMS)
file.parseWzFile()
```

Pass a custom Wasm URL when a bundler or CDN serves `libwz.wasm` from another
location:

```js
await init(new URL('./assets/libwz.wasm', import.meta.url))
```

Open from bytes:

```js
import { MapleVersion, WzFile } from 'libwz'

const bytes = new Uint8Array(await fileInput.files[0].arrayBuffer())
using file = WzFile.fromBytes('Character.wz', bytes, MapleVersion.GMS)
file.parseWzFile()
```

For large browser files, use callback-backed input so the Wasm backend can read
ranges without copying the whole file. In a Worker, `FileReaderSync` is one
simple way to implement the callback:

```js
const reader = new FileReaderSync()

using file = WzFile.fromBlobSource(
  'Character.wz',
  blob.size,
  MapleVersion.GMS,
  (offset, length, destination) => {
    const start = Number(offset)
    const end = start + Number(length)
    const bytes = new Uint8Array(
      reader.readAsArrayBuffer(blob.slice(start, end))
    )
    if (destination !== undefined) {
      destination.set(bytes)
      return
    }
    return bytes
  }
)
```

### JavaScript Properties

Traverse and inspect properties:

```js
const image = file.getWzDirectory()?.getImageByName('00002000.img')
image?.parseImage()

for (const prop of image?.wzProperties() ?? []) {
  console.log(prop.getName(), prop.getPropertyType())
}

const info = image?.getFromPath('info')
const name = info?.getChildByName('name')?.getString()
```

Export images and sounds:

```js
const canvas = image?.getFromPath('stand/0')
if (canvas instanceof WzCanvasProperty) {
  canvas.saveToFile('stand-0.png')

  const png = canvas.getPngProperty()
  console.log(png?.getWidth(), png?.getHeight())
}
```

```js
const sound = image?.getFromPath('sound')
if (sound instanceof WzBinaryProperty) {
  sound.saveToFile('sound.mp3')
}
```

## Build Options

| CMake option | Default | Description |
|--------------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build Google Test unit tests |
| `BUILD_CAPI` | `ON` | Build the C ABI layer |
| `BUILD_JNI` | `OFF` | Build the JNI shared library |
| `BUILD_WASM` | `OFF` | Build the WebAssembly target |

| Target | Type | Description |
|--------|------|-------------|
| `wz` | Static library | Core C++ parser |
| `wz_capi` | Static library | C ABI wrapper |
| `wz_jni` | Shared library | JNI native bridge |
| `wz_wasm` | Emscripten executable | JavaScript/Wasm artifact |

## Development Commands

```bash
# C++
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build

# Java
cmake -S . -B build -DBUILD_JNI=ON
cmake --build build
(cd java && mvn test)

# JavaScript
npm run build:ts
npm run build:native
npm test
npm run test:wasm
```

Format and lint:

```bash
./format.sh
./lint.sh
npm run lint:js
```

## Notes

- The C++ core is built with exceptions and RTTI disabled.
- C++ failure paths use `wz::Result<T>` where possible.
- Java and JavaScript wrappers translate native failures to Java exceptions or
  JavaScript errors at the binding boundary.
- JavaScript `saveToDisk()` and path-based file APIs depend on the active
  backend's runtime capabilities. The WebAssembly backend supports host paths in
  Node.js through a mounted filesystem; browser code should prefer bytes or
  callback-backed input.
- Published Java artifacts are intended to bundle native libraries for Windows
  x86_64, Linux x86_64, and macOS ARM64 through the GitHub Actions release
  workflow.
- The npm package publishes `dist/index.js`, `dist/index.d.ts`,
  `dist/libwz.js`, and `dist/libwz.wasm`.

## License

libwz is licensed under the MIT License. The `Harepacker-resurrected/` submodule 
(test data reference, not part of libwz source) has its own licenses; see that 
submodule for details.
