# WASM Blob Data Source Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an ESM-only emnapi/Emscripten browser package that parses a
Worker-owned Blob with bounded random reads, while preserving native and
C#-compatible APIs.

**Architecture:** Replace `WzBinaryReader`'s `std::ifstream` dependency with a
random-access `WzDataSource`. Native paths use a stream implementation; browser
files use a Worker-only Blob implementation backed by `Blob.slice` and
`FileReaderSync`. The existing Node-API C++ binding is compiled for both native
and Wasm; browser-only constructors are additive exports. ESM worker and client
modules load the emnapi runtime without UMD or CJS browser fallbacks.

**Tech Stack:** C++23, CMake, Emscripten, emnapi, `@emnapi/runtime`, Node-API,
TypeScript, Vite, Webpack, Google Test, cpplint.

---

## File Map

- Create `include/wz/Util/WzDataSource.h`: platform-neutral random-access input
  interface and stream/memory declarations.
- Create `src/Util/WzDataSource.cpp`: native stream and owned memory sources.
- Create `include/wz/Util/WzBlobDataSource.h`: Emscripten-only blob source
  declaration, isolated from the core interface.
- Create `src/Util/WzBlobDataSource.cpp`: Worker JavaScript import bridge and
  bounded Blob range reads.
- Modify `include/wz/Util/WzBinaryReader.h` and
  `src/Util/WzBinaryReader.cpp`: source ownership, positions, cache, and
  compatibility stream construction.
- Modify `include/wz/WzFile.h` and `src/WzFile.cpp`: path, memory, and
  source-backed file construction without changing current constructors.
- Modify `include/wz/wz_api.h` and `src/capi/wz_api.cpp`: additive memory and
  browser-source C API factories.
- Modify `src/node/binding.cpp`: additive N-API open methods in the existing
  binding source.
- Modify `CMakeLists.txt`, `binding.gyp`, `package.json`, and `tsconfig.json`:
  add the Wasm build without breaking native node-gyp.
- Modify `index.ts`: keep current Node wrappers and add typed source/worker
  initialization exports.
- Create `wasm/worker.ts`: ESM Worker entry, Blob registry, and Worker message
  bridge.
- Create `wasm/browser.ts`: ESM browser client and a worker factory using
  `new URL("./worker.js", import.meta.url)`.
- Create `wasm/loader.ts`: ESM-only emnapi module initialization.
- Create `tests/TestWzDataSource.cpp`: core source/cache tests.
- Create `tests/wasm/worker.test.ts`, `tests/wasm/vite/`, and
  `tests/wasm/webpack/`: Worker and bundler integration fixtures.

## Task 1: Define and test the core random-access source

**Files:**
- Create: `include/wz/Util/WzDataSource.h`
- Create: `src/Util/WzDataSource.cpp`
- Create: `tests/TestWzDataSource.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing source tests**

Create `tests/TestWzDataSource.cpp` with direct tests for owned bytes and a
seekable stream. The tests establish exact short-read behavior before reader
integration:

```cpp
#include <array>
#include <sstream>
#include <vector>
#include "gtest/gtest.h"
#include "wz/Util/WzDataSource.h"

TEST(WzMemoryDataSource, ReadsRequestedRange) {
  wz::WzMemoryDataSource source({0x10, 0x20, 0x30, 0x40});
  std::array<uint8_t, 2> bytes = {};
  auto result = source.ReadAt(1, bytes);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 2u);
  EXPECT_EQ(bytes, (std::array<uint8_t, 2>{0x20, 0x30}));
}

TEST(WzMemoryDataSource, ReportsShortReadAtEnd) {
  wz::WzMemoryDataSource source({0x10, 0x20});
  std::array<uint8_t, 4> bytes = {};
  auto result = source.ReadAt(1, bytes);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 1u);
  EXPECT_EQ(bytes[0], 0x20);
}

TEST(WzStreamDataSource, SupportsRandomReads) {
  std::istringstream stream(std::string("abcd"), std::ios::binary);
  wz::WzStreamDataSource source(stream);
  std::array<uint8_t, 2> bytes = {};
  ASSERT_TRUE(source.ReadAt(2, bytes).has_value());
  EXPECT_EQ(bytes, (std::array<uint8_t, 2>{'c', 'd'}));
}
```

- [ ] **Step 2: Register and run the test to verify failure**

Append `tests/TestWzDataSource.cpp` to `WZ_TEST_SOURCES` in
`CMakeLists.txt`, then run:

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=OFF
cmake --build build --target wz_test
```

Expected: compilation fails because `wz/Util/WzDataSource.h` is absent.

- [ ] **Step 3: Implement the platform-neutral interface and two sources**

Create the header with the exact public boundary:

```cpp
class WzDataSource {
 public:
  virtual ~WzDataSource() = default;
  virtual Result<size_t> ReadAt(uint64_t offset,
                                std::span<uint8_t> destination) = 0;
  virtual uint64_t Size() const = 0;
};
```

Implement `WzStreamDataSource` by recording stream size once, clearing stream
state before each `seekg`, and returning `gcount()`. Implement
`WzMemoryDataSource` by owning a moved `std::vector<uint8_t>` and returning a
short count at EOF. An offset beyond the source size returns
`std::unexpected(Error::IoError(...))`. Add the implementation and header to
the `wz` source/header lists in `CMakeLists.txt`.

- [ ] **Step 4: Run focused tests**

```bash
cmake --build build --target wz_test
ctest --test-dir build -R wz_test --output-on-failure
```

Expected: all tests pass, including the three new source tests.

- [ ] **Step 5: Format, lint, and commit Task 1**

```bash
clang-format -i include/wz/Util/WzDataSource.h src/Util/WzDataSource.cpp \
  tests/TestWzDataSource.cpp
cpplint --recursive --config .cpplint src include tests
git add CMakeLists.txt include/wz/Util/WzDataSource.h \
  src/Util/WzDataSource.cpp tests/TestWzDataSource.cpp
git commit -m "feat: add random access WZ data source"
```

Expected: cpplint reports zero errors and the commit contains only Task 1
files.

## Task 2: Migrate `WzBinaryReader` with an internal cache

**Files:**
- Modify: `include/wz/Util/WzBinaryReader.h`
- Modify: `src/Util/WzBinaryReader.cpp`
- Modify: `src/Util/ListFileParser.cpp`
- Modify: `src/WzDirectory.cpp`
- Modify: `tests/TestWzDataSource.cpp`

- [ ] **Step 1: Add failing reader cache tests**

Add a counting fake source to `tests/TestWzDataSource.cpp` and assert that two
adjacent scalar reads use one source call:

```cpp
class CountingSource final : public wz::WzDataSource {
 public:
  Result<size_t> ReadAt(uint64_t offset,
                        std::span<uint8_t> destination) override;
  uint64_t Size() const override { return bytes_.size(); }
  std::vector<uint8_t> bytes_ = {1, 0, 0, 0, 2, 0, 0, 0};
  int read_count_ = 0;
};

TEST(WzBinaryReader, CachesSequentialScalarReads) {
  auto source = std::make_shared<CountingSource>();
  wz::WzBinaryReader reader(source, wz::WzAESConstant::GMSIv);
  EXPECT_EQ(reader.ReadInt32(), 1);
  EXPECT_EQ(reader.ReadInt32(), 2);
  EXPECT_EQ(source->read_count_, 1);
}
```

- [ ] **Step 2: Run the new test to verify failure**

```bash
cmake --build build --target wz_test
ctest --test-dir build -R wz_test --output-on-failure
```

Expected: compilation fails because the source constructor is absent.

- [ ] **Step 3: Add source construction and cache reads**

Add these constructors without removing the current stream constructor:

```cpp
WzBinaryReader(std::istream& input, const std::array<uint8_t, 4>& wz_iv,
               int64_t start_offset = 0);
WzBinaryReader(std::shared_ptr<WzDataSource> source,
               const std::array<uint8_t, 4>& wz_iv,
               int64_t start_offset = 0);
```

The stream constructor creates a `WzStreamDataSource`. Replace direct stream
reads with a private `ReadExact(void*, size_t)` that first serves the current
cache and otherwise fills a 256 KiB window at the current position. `ReadBytes`
returns an empty vector on an incomplete requested read; scalar reads use
zero-initialized local storage. `Position`, `SetPosition`, `Seek`, and
`Available` operate on source size and the reader position.

Remove `BaseStream()` from the reader API. Change `ListFileParser` EOF testing
to `reader.Available() > 0`. In `WzDirectory::SaveImages`, validate unchanged
image ranges against `reader.SourceSize()` before `ReadBytes`.

- [ ] **Step 4: Run core regression tests**

```bash
cmake --build build --target wz_test
ctest --test-dir build --output-on-failure
```

Expected: new cache test and all existing tests pass.

- [ ] **Step 5: Format, lint, and commit Task 2**

```bash
clang-format -i include/wz/Util/WzBinaryReader.h \
  src/Util/WzBinaryReader.cpp src/Util/ListFileParser.cpp src/WzDirectory.cpp \
  tests/TestWzDataSource.cpp
cpplint --recursive --config .cpplint src include tests
git add include/wz/Util/WzBinaryReader.h src/Util/WzBinaryReader.cpp \
  src/Util/ListFileParser.cpp src/WzDirectory.cpp tests/TestWzDataSource.cpp
git commit -m "refactor: read WZ data through cached sources"
```

## Task 3: Add source-backed WZ file and memory C API

**Files:**
- Modify: `include/wz/WzFile.h`
- Modify: `src/WzFile.cpp`
- Modify: `include/wz/wz_api.h`
- Modify: `src/capi/wz_api.cpp`
- Modify: `tests/TestCapiError.cpp`

- [ ] **Step 1: Write failing memory-open tests**

Add C API declarations and tests that verify validation and that input is copied:

```cpp
TEST(WzCapi, OpenMemoryRejectsNullDataWithNonzeroSize) {
  wz_file file = nullptr;
  EXPECT_EQ(wz_open_memory("input.wz", nullptr, 1, -1, WZ_GMS, &file),
            WZ_ERROR_INVALID_ARGUMENT);
}

TEST(WzCapi, OpenMemoryOwnsInputBytes) {
  std::vector<uint8_t> bytes = {'P', 'K', 'G', '1'};
  wz_file file = nullptr;
  ASSERT_EQ(wz_open_memory("input.wz", bytes.data(), bytes.size(), -1,
                            WZ_GMS, &file), WZ_ERROR_NONE);
  bytes[0] = 0;
  EXPECT_NE(file, nullptr);
  EXPECT_EQ(wz_close_file(file), WZ_ERROR_NONE);
}
```

- [ ] **Step 2: Run the C API test to verify failure**

```bash
cmake --build build --target wz_test
ctest --test-dir build -R wz_test --output-on-failure
```

Expected: compilation fails because `wz_open_memory` is absent.

- [ ] **Step 3: Add source-backed construction without changing paths**

Add `WzFile` constructors or named factories accepting a file name and
`std::shared_ptr<WzDataSource>`. Path constructors continue opening an
`std::ifstream` and construct `WzStreamDataSource` at parse time. Memory
constructors use `WzMemoryDataSource` and preserve the supplied file name as
`Name()` while returning an empty `FilePath()`.

Add exactly these additive C functions:

```c
wz_error_code wz_open_memory(const char* file_name, const uint8_t* data,
                              size_t data_size, short game_version,
                              wz_maple_version version, wz_file* out_file);
wz_error_code wz_open_memory_with_iv(const char* file_name,
                                      const uint8_t* data, size_t data_size,
                                      const uint8_t iv[4], wz_file* out_file);
```

Reject a null file name, a null output handle, and `data == NULL` when
`data_size != 0`. Copy bytes into the source before returning. Empty input is
valid to create a handle but parsing it returns the existing parse error.

- [ ] **Step 4: Run full C++ regression tests**

```bash
cmake --build build --target wz_test
ctest --test-dir build --output-on-failure
```

Expected: all tests pass and existing path callers behave unchanged.

- [ ] **Step 5: Format, lint, and commit Task 3**

```bash
clang-format -i include/wz/WzFile.h src/WzFile.cpp include/wz/wz_api.h \
  src/capi/wz_api.cpp tests/TestCapiError.cpp
cpplint --recursive --config .cpplint src include tests
git add include/wz/WzFile.h src/WzFile.cpp include/wz/wz_api.h \
  src/capi/wz_api.cpp tests/TestCapiError.cpp
git commit -m "feat: open WZ files from owned memory"
```

## Task 4: Reuse the existing Node-API binding for memory input

**Files:**
- Modify: `src/node/binding.cpp`
- Modify: `index.ts`
- Modify: `tests/smoke.test.js`
- Modify: `binding.gyp`

- [ ] **Step 1: Write failing TypeScript/Node tests**

Add a test that reads an existing sample into a `Uint8Array`, opens it via the
new wrapper, and parses it:

```js
test("opens a WZ file from owned bytes", () => {
  const bytes = new Uint8Array(fs.readFileSync(sampleWz));
  const file = wz.WzFile.fromBytes("TamingMob_GMS_87.wz", bytes,
                                   wz.MapleVersion.GMS);
  try {
    assert.equal(file.parseWzFile(), wz.ParseStatus.SUCCESS);
    assert.equal(file.getName(), "TamingMob_GMS_87.wz");
  } finally {
    file.close();
  }
});
```

- [ ] **Step 2: Run the test to verify failure**

```bash
npm run build:ts
node --test tests/smoke.test.js
```

Expected: `WzFile.fromBytes is not a function`.

- [ ] **Step 3: Add methods to the current binding source**

In `src/node/binding.cpp`, add `OpenMemory` and `OpenMemoryWithIv`. Each reads
a `Napi::Uint8Array`, passes `Data()` and `ByteLength()` to the C API, and
returns the existing BigInt handle. Register both beside `openFile` in the
existing `Init` export table. Do not create a second binding target.

Extend `NativeBinding` in `index.ts` with:

```ts
openMemory(name: string, bytes: Uint8Array, gameVersion: number,
           mapleVersion: MapleVersionValue): NullableNativeHandle;
openMemoryWithIv(name: string, bytes: Uint8Array,
                 iv: ArrayBufferViewLike): NullableNativeHandle;
```

Add static overloads through `WzFile.fromBytes`; they call the matching native
method, validate a non-null result, and construct the existing `WzFile`
wrapper. Add `src/Util/WzDataSource.cpp` to the `wz` target's `sources` in
`binding.gyp`.

- [ ] **Step 4: Run Node and C++ tests**

```bash
npm run build
node --test tests/smoke.test.js
cmake --build build --target wz_test
ctest --test-dir build --output-on-failure
```

Expected: the new byte-input test and all regressions pass.

- [ ] **Step 5: Format, lint, and commit Task 4**

```bash
clang-format -i src/node/binding.cpp
cpplint --recursive --config .cpplint src include
git add src/node/binding.cpp index.ts index.d.ts binding.gyp tests/smoke.test.js
git commit -m "feat: expose WZ memory input through Node API"
```

## Task 5: Add Emscripten Blob source and additive binding factories

**Files:**
- Create: `include/wz/Util/WzBlobDataSource.h`
- Create: `src/Util/WzBlobDataSource.cpp`
- Modify: `include/wz/WzFile.h`
- Modify: `src/WzFile.cpp`
- Modify: `include/wz/wz_api.h`
- Modify: `src/capi/wz_api.cpp`
- Modify: `src/node/binding.cpp`
- Create: `tests/TestWzBlobDataSource.cpp`

- [ ] **Step 1: Write an Emscripten-only failing source test**

Create `tests/TestWzBlobDataSource.cpp` guarded by `#ifdef __EMSCRIPTEN__`.
Inject a test registry reader that records requested ranges, then verify a
source of size 1 MiB can read at offset 700 KiB without requesting byte zero:

```cpp
TEST(WzBlobDataSource, ReadsOnlyRequestedRange) {
  wz::WzBlobDataSource source(7, 1024 * 1024);
  std::array<uint8_t, 16> bytes = {};
  ASSERT_TRUE(source.ReadAt(700 * 1024, bytes).has_value());
  EXPECT_EQ(wz_test_last_blob_read_offset(), 700 * 1024);
}
```

- [ ] **Step 2: Build the Wasm test target to verify failure**

```bash
emcmake cmake -S . -B build/wasm -DBUILD_WASM=ON -DBUILD_TESTS=ON
cmake --build build/wasm --target wz_wasm_test
```

Expected: compilation fails because `WzBlobDataSource` and the Wasm test target
are absent.

- [ ] **Step 3: Implement the isolated Blob source**

Declare `WzBlobDataSource(uint32_t blob_id, uint64_t size)`. In the Emscripten
implementation, use an `EM_JS` import that calls a globally supplied Worker
registry function with `(blobId, offset, destinationPointer, length)`. The
import must return a byte count or a negative errno-like value. Translate a
negative result and short read to `Error::IoError`.

Do not use `Blob`, `FileReaderSync`, `EM_JS`, or Emscripten headers from
`WzDataSource.h` or any native source. Compile `WzBlobDataSource.cpp` only when
`EMSCRIPTEN` is true.

Add C API factories:

```c
wz_error_code wz_open_blob_source(uint32_t blob_id, uint64_t size,
                                  const char* file_name, short game_version,
                                  wz_maple_version version, wz_file* out_file);
wz_error_code wz_open_blob_source_with_iv(uint32_t blob_id, uint64_t size,
                                          const char* file_name,
                                          const uint8_t iv[4],
                                          wz_file* out_file);
```

In non-Emscripten builds they return `WZ_ERROR_NOT_IMPLEMENTED`. In Emscripten
builds they create a normal source-backed `WzFile`.

Add matching `openBlobSource` methods to the existing `binding.cpp` and register
them alongside `openMemory`; they do not add another addon target.

- [ ] **Step 4: Verify native and Wasm behavior**

```bash
cmake --build build --target wz_test
ctest --test-dir build --output-on-failure
cmake --build build/wasm --target wz_wasm_test
```

Expected: native tests pass and browser-source factory calls are explicitly not
implemented; the Emscripten test proves range reads do not start at zero.

- [ ] **Step 5: Format, lint, and commit Task 5**

```bash
clang-format -i include/wz/Util/WzBlobDataSource.h \
  src/Util/WzBlobDataSource.cpp include/wz/WzFile.h src/WzFile.cpp \
  include/wz/wz_api.h src/capi/wz_api.cpp src/node/binding.cpp \
  tests/TestWzBlobDataSource.cpp
cpplint --recursive --config .cpplint src include tests
git add include/wz/Util/WzBlobDataSource.h src/Util/WzBlobDataSource.cpp \
  include/wz/WzFile.h src/WzFile.cpp include/wz/wz_api.h src/capi/wz_api.cpp \
  src/node/binding.cpp tests/TestWzBlobDataSource.cpp
git commit -m "feat: add worker blob WZ source"
```

## Task 6: Add ESM-only Emscripten/emnapi packaging

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `package.json`
- Modify: `tsconfig.json`
- Create: `wasm/loader.ts`
- Create: `wasm/worker.ts`
- Create: `wasm/browser.ts`
- Create: `wasm/tsconfig.json`

- [ ] **Step 1: Write failing ESM import checks**

Create `tests/wasm/esm-import.test.mjs`:

```js
import assert from "node:assert/strict";
import { createWzWorker } from "../../dist/wasm/browser.js";

assert.equal(typeof createWzWorker, "function");
```

Run `node tests/wasm/esm-import.test.mjs`. Expected: module-not-found because
the ESM output does not exist.

- [ ] **Step 2: Add `BUILD_WASM` CMake target**

Add `option(BUILD_WASM "Build the emnapi WebAssembly module" OFF)`. Under
`if(BUILD_WASM)`, require `EMSCRIPTEN`, disable JNI, add the current
`src/node/binding.cpp` to `wz_wasm`, and link it with the same `wz_capi`, zlib,
and tiny-AES targets. Configure the Emscripten output for modular ESM glue and
an externally loaded `.wasm`; do not set `SINGLE_FILE`, UMD, or CommonJS
wrapper options.

Add npm scripts:

```json
"build:wasm": "npm run build:wasm:ts && emcmake cmake -S . -B build/wasm -DBUILD_WASM=ON -DBUILD_TESTS=OFF && cmake --build build/wasm",
"build:wasm:ts": "tsc -p wasm/tsconfig.json",
"test:wasm": "npm run build:wasm && node tests/wasm/esm-import.test.mjs"
```

Pin `@emnapi/runtime` in dependencies. Add an `exports` map with an `import`
condition for `./wasm`; do not add `require`, UMD, or browser CommonJS entries.
Keep the current native `main` and native scripts intact.

- [ ] **Step 3: Implement ESM worker modules**

`wasm/loader.ts` imports `@emnapi/runtime` using ESM syntax and exports an
async `initializeWasm` that accepts the Wasm URL produced by the bundler.

`wasm/worker.ts` maintains `Map<number, Blob>`, exposes a synchronous global
range reader consumed by `WzBlobDataSource`, and receives RPC messages to add
or remove blobs and invoke existing binding methods. Its Blob read function is:

```ts
function readBlobRange(id: number, offset: number, length: number,
                       destination: Uint8Array): number {
  const blob = blobs.get(id);
  if (!blob) return -1;
  const end = Math.min(offset + length, blob.size);
  const bytes = new Uint8Array(
    new FileReaderSync().readAsArrayBuffer(blob.slice(offset, end))
  );
  destination.set(bytes);
  return bytes.byteLength;
}
```

`wasm/browser.ts` exports `createWzWorker`, constructs an ESM Worker with
`new Worker(new URL("./worker.js", import.meta.url), { type: "module" })`, and
returns an async RPC client. It does not duplicate C++ N-API binding code or
the C++ object model.

- [ ] **Step 4: Build and verify the ESM artifact**

```bash
npm run build:wasm
node tests/wasm/esm-import.test.mjs
```

Expected: the test imports the ESM entry without `require` and prints no error.

- [ ] **Step 5: Commit Task 6**

```bash
git add CMakeLists.txt package.json tsconfig.json wasm/loader.ts wasm/worker.ts \
  wasm/browser.ts wasm/tsconfig.json tests/wasm/esm-import.test.mjs
git commit -m "feat: build libwz as an ESM wasm module"
```

## Task 7: Add browser Blob integration and bundler fixtures

**Files:**
- Modify: `index.ts`
- Modify: `wasm/browser.ts`
- Create: `tests/wasm/worker.test.ts`
- Create: `tests/wasm/vite/package.json`
- Create: `tests/wasm/vite/vite.config.ts`
- Create: `tests/wasm/vite/src/main.ts`
- Create: `tests/wasm/webpack/package.json`
- Create: `tests/wasm/webpack/webpack.config.mjs`
- Create: `tests/wasm/webpack/src/index.mjs`

- [ ] **Step 1: Write failing Blob integration test**

Create a Worker test that wraps a fixture WZ file in a Blob whose
`arrayBuffer` method throws. It should create a worker client, call the
additive `fromBlob` operation, parse, request a lazy image, and close:

```ts
const blob = new Blob([fixtureBytes]);
Object.defineProperty(blob, "arrayBuffer", {
  value: () => { throw new Error("full Blob read is forbidden"); },
});
const file = await client.fromBlob(blob, options);
await file.parseWzFile();
await file.getWzDirectory().getImage(0).parseImage();
await file.close();
```

- [ ] **Step 2: Run the test to verify failure**

```bash
npm run test:wasm
```

Expected: failure because the worker client has no `fromBlob` operation.

- [ ] **Step 3: Add the additive Blob client API**

Add `fromBlob(blob, options)` to the ESM Worker client. It allocates a numeric
registry id, sends the Blob to the Worker through structured clone, then invokes
the same `openBlobSource` export from the existing binding. On close, it sends a
release message and then calls the existing `closeFile` binding method.

Keep `index.ts` as the native object wrapper. Export the browser worker client
from the `./wasm` ESM condition rather than changing the synchronous Node
`WzFile` constructor.

- [ ] **Step 4: Add Vite and Webpack smoke builds**

The Vite fixture imports `createWzWorker` from the package's `./wasm` ESM
export. The Webpack fixture does the same from an `.mjs` entry with
`experiments.asyncWebAssembly = true`. Each fixture build must complete without
manually copying the `.wasm` file or using `require`.

Run:

```bash
npm run test:wasm
npm --prefix tests/wasm/vite run build
npm --prefix tests/wasm/webpack run build
```

Expected: Blob test passes without calling full `arrayBuffer`, and both bundler
builds emit ESM bundles and a referenced Wasm asset.

- [ ] **Step 5: Commit Task 7**

```bash
git add index.ts wasm/browser.ts tests/wasm
git commit -m "test: verify ESM worker Blob integration"
```

## Task 8: Add explicit browser save output

**Files:**
- Modify: `include/wz/WzFile.h`
- Modify: `src/WzFile.cpp`
- Modify: `include/wz/wz_api.h`
- Modify: `src/capi/wz_api.cpp`
- Modify: `src/node/binding.cpp`
- Modify: `wasm/browser.ts`
- Create: `tests/TestWzSaveBytes.cpp`
- Create: `tests/wasm/save-output.test.ts`

- [ ] **Step 1: Write failing save-to-bytes test**

Create a small editable WZ file, call the proposed C API size-query and copy
operations, and reopen the result through `wz_open_memory`:

```cpp
size_t size = 0;
ASSERT_EQ(wz_file_save_to_bytes(file, nullptr, 0, &size), WZ_ERROR_NONE);
std::vector<uint8_t> bytes(size);
ASSERT_EQ(wz_file_save_to_bytes(file, bytes.data(), bytes.size(), &size),
          WZ_ERROR_NONE);
ASSERT_EQ(wz_open_memory("roundtrip.wz", bytes.data(), bytes.size(), 95,
                          WZ_GMS, &reopened), WZ_ERROR_NONE);
```

- [ ] **Step 2: Run to verify failure**

```bash
cmake --build build --target wz_test
ctest --test-dir build -R wz_test --output-on-failure
```

Expected: compilation fails because `wz_file_save_to_bytes` is absent.

- [ ] **Step 3: Implement save bytes and explicit browser helpers**

Refactor the existing `WzFile::SaveToDisk` writing sequence into a shared
private `BuildSerializedData()` returning `Result<std::vector<uint8_t>>`.
`SaveToDisk` writes those bytes to its requested native path without changed
semantics. Add a C API size-query/copy function:

```c
wz_error_code wz_file_save_to_bytes(wz_file file, uint8_t* buffer,
                                    size_t buffer_size, size_t* out_size);
```

Expose it through the current binding as `fileSaveToBytes`. Add the ESM client
methods `saveToBlob()` and `download(filename)`. `download` explicitly creates
an object URL and clicks a temporary anchor. Browser `fileSaveToDisk` calls
return `WZ_ERROR_NOT_IMPLEMENTED` before any path operation.

- [ ] **Step 4: Run save regression tests**

```bash
cmake --build build --target wz_test
ctest --test-dir build --output-on-failure
npm run test:wasm
```

Expected: C++ bytes round-trip passes, browser output creates a Blob only when
requested, and native `saveToDisk` tests remain unchanged.

- [ ] **Step 5: Format, lint, test, and commit Task 8**

```bash
clang-format -i include/wz/WzFile.h src/WzFile.cpp include/wz/wz_api.h \
  src/capi/wz_api.cpp src/node/binding.cpp tests/TestWzSaveBytes.cpp
cpplint --recursive --config .cpplint src include tests
cmake --build build --target wz_test
ctest --test-dir build --output-on-failure
git add include/wz/WzFile.h src/WzFile.cpp include/wz/wz_api.h \
  src/capi/wz_api.cpp src/node/binding.cpp wasm/browser.ts \
  tests/TestWzSaveBytes.cpp tests/wasm/save-output.test.ts
git commit -m "feat: add explicit browser WZ save output"
```

## Final Verification

- [ ] Run the complete native verification suite:

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=OFF
cmake --build build
ctest --test-dir build --output-on-failure
npm run build
node --test
cpplint --recursive --config .cpplint src include tests
```

- [ ] Run the complete browser verification suite:

```bash
npm run build:wasm
npm run test:wasm
npm --prefix tests/wasm/vite run build
npm --prefix tests/wasm/webpack run build
```

- [ ] Confirm `git status --short` contains only expected task changes and
  excludes the user's pre-existing `.vscode/settings.json`.
