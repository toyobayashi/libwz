# WASM Blob Data Source Progress — 2026-06-22

## Status

Implementation has completed and committed plan Tasks 1 through 6. The next
task is Task 7: Worker Blob integration tests and Vite/Webpack fixtures.

Task 6 passed subagent specification and code-quality review after fixing
reviewer-found packaging and Wasm runtime export issues. Continue with the same
subagent review workflow: failing test first, implementation, verification,
spec review, code-quality review, then commit.

## Completed Design and Planning Work

- Approved design: `docs/superpowers/specs/2026-06-22-wasm-blob-data-source-design.md`
  (`fa9e80e`).
- Approved implementation plan:
  `docs/superpowers/plans/2026-06-22-wasm-blob-data-source-implementation.md`
  (`e6c8a1c`).
- Browser direction is fixed:
  - Dedicated Worker only.
  - `Blob.slice()` plus `FileReaderSync` for synchronous, bounded reads.
  - No Asyncify and no complete Blob copy into Wasm memory.
  - ESM-only browser output for Vite/Webpack; no UMD/CJS browser loader.
  - Reuse the existing C++ Node-API binding and `index.ts`; only additive APIs.
  - Browser `saveFileToDisk(path)` must return unsupported, never trigger a
    download implicitly. Future save output is `saveToBytes`/`saveToBlob` plus
    explicit `download()`.

## Completed Code Changes

### Task 0 — Native node-gyp baseline repair

Commit: `c677408 fix: use C++23 for Xcode node-gyp builds`

- Added the Xcode-only `CLANG_CXX_LANGUAGE_STANDARD: c++23` setting in
  `binding.gyp`.
- Root cause: node-gyp generated `-std=gnu++20`, while the project requires
  C++23 `std::expected`; CMake already used `-std=c++2b` successfully.
- Verification performed by the implementer:
  `npm run build && npm test` passed with 13 tests.
- Task 0 passed independent specification and code-quality reviews.

### Task 1 — Core random-access data source

Commits:

- `fce0d602 feat: add random access WZ data source`
- `a024d882 fix: handle data source reads at EOF`
- `30a93376 fix: reject nonseekable WZ sources`
- `cd97b9ad fix: reject unexpected WZ stream short reads`

Files added:

- `include/wz/Util/WzDataSource.h`
- `src/Util/WzDataSource.cpp`
- `tests/TestWzDataSource.cpp`

Files modified:

- `CMakeLists.txt`

Implemented API:

```cpp
class WzDataSource {
 public:
  virtual ~WzDataSource() = default;
  virtual Result<size_t> ReadAt(uint64_t offset,
                                std::span<uint8_t> destination) = 0;
  virtual uint64_t Size() const = 0;
};
```

Implemented concrete sources:

- `WzMemoryDataSource`, which owns caller-provided bytes.
- `WzStreamDataSource`, which adapts a seekable `std::istream`.

Verified behavior:

- Exact memory reads.
- Short memory reads at EOF.
- Reads beginning exactly at EOF return successful zero-byte reads.
- Reads beginning beyond EOF return `IoError`.
- Stream random reads.
- Non-seekable or stream-sizing-failed inputs return `IoError`, not a false
  empty source.
- A stream that becomes shorter after size capture returns `IoError`, not a
  successful short read.

Verification performed by the implementer after each corrective commit:

```bash
cmake --build build --target wz_test -j2
ctest --test-dir build --output-on-failure
cpplint --recursive --config .cpplint src include
```

All reported as passing. Changed C++ files were formatted with `clang-format`.

### Task 2 — Cached source-backed `WzBinaryReader`

Commit: `e2524c9 refactor: read WZ data through cached sources`

- Replaced direct `std::ifstream` reads in `WzBinaryReader` with
  `std::shared_ptr<WzDataSource>`.
- Added a 256 KiB read cache and source-backed constructor while preserving the
  stream constructor.
- Removed `BaseStream()` from the reader API.
- Changed `ListFileParser` EOF handling to `reader.Available() > 0`.
- Changed unchanged-image save range validation to use `reader.SourceSize()`.
- Added reader cache, incomplete read, availability, and source-size tests.
- Passed independent spec and code-quality reviews.

### Task 3 — Source-backed WZ file and memory C API

Commit: `95216cb feat: open WZ files from owned memory`

- Added source-backed `WzFile` construction.
- Added `wz_open_memory` and `wz_open_memory_with_iv`.
- Memory input is copied into `WzMemoryDataSource` before returning.
- Memory-backed files preserve `Name()` and return an empty `FilePath()`.
- Empty or malformed memory input creates a handle but parse returns the
  existing parse error instead of silently succeeding or allocating huge
  buffers.
- Passed independent spec and code-quality reviews after a reviewer-requested
  malformed-header guard.

### Task 4 — Node-API memory input

Commit: `44a83d7 feat: expose WZ memory input through Node API`

- Added `openMemory` and `openMemoryWithIv` to the existing N-API binding.
- Added `WzFile.fromBytes(...)` TypeScript overloads.
- Added `src/Util/WzDataSource.cpp` to the node-gyp `wz` target.
- Added a Node smoke test that opens a sample WZ from `Uint8Array` bytes and
  parses it.
- Passed independent spec and code-quality reviews.

### Task 5 — Emscripten Blob source and additive Blob factories

Commit: `6b88e06 feat: add worker blob WZ source`

- Added Emscripten-only `WzBlobDataSource`.
- Added `wz_open_blob_source` and `wz_open_blob_source_with_iv`.
- Native builds return `WZ_ERROR_NOT_IMPLEMENTED` for Blob-source factories.
- Added matching `openBlobSource` binding exports in the existing N-API source.
- Added `BUILD_WASM` CMake support and `wz_wasm_test`.
- Added a Wasm-only test proving the Blob source reads the requested range
  without first requesting offset zero.
- Passed independent spec and code-quality reviews after a reviewer-requested
  stronger no-offset-zero assertion.

### Task 6 — ESM-only Emscripten/emnapi packaging

Commit: `2d389a3 feat: build libwz as an ESM wasm module`

- Added an emnapi-backed `wz_wasm` CMake target that reuses
  `src/node/binding.cpp`, links `wz_capi`, and emits ESM glue plus an external
  `libwz.wasm`.
- Added `build:wasm`, `build:wasm:ts`, and `test:wasm` npm scripts.
- Added a `./wasm` import-only package export and pinned `@emnapi/runtime`.
- Added `emnapi` as a dev dependency for Emscripten Node-API headers and CMake
  support.
- Added `wasm/loader.ts`, `wasm/worker.ts`, `wasm/browser.ts`, and
  `wasm/tsconfig.json`.
- `loader.ts` exports `initializeWasm(wasmUrl)` and a default
  `loadWzModule()` helper.
- `worker.ts` keeps a `Map<number, Blob>` and serves synchronous range reads
  through `FileReaderSync` and the `__libwzReadBlobRange` bridge.
- `browser.ts` exports an ESM Worker factory returning a small async RPC
  client.
- Added `tests/wasm/esm-import.test.mjs`; it now imports the browser entry,
  initializes the Wasm module, and verifies `HEAPU8` is exported for Blob range
  copies.
- `prepack` now builds the native TypeScript output and the Wasm artifact so
  the public `./wasm` export is present in packed releases.
- Passed independent review after fixes for missing `initializeWasm`, shallow
  RPC return shape, stale pack artifacts, and missing `HEAPU8` runtime export.

## Review State

Task 1 received the required fresh code-quality re-review before Task 2 began.
No blocking or important issues were found. The only note was a non-blocking
stream-source test gap for `offset > Size()`.

Task 2, Task 3, Task 4, Task 5, and Task 6 each completed specification and
code-quality reviews. Reviewer-found issues were fixed and re-reviewed before
commit.

Task 1 earlier code-quality review found two real issues, both fixed in
follow-up commits:

1. Non-seekable streams looked like valid empty streams — fixed in `30a93376`.
2. Unexpected short reads after captured stream size looked successful — fixed
   in `cd97b9ad`.

## Not Started

- Task 7: Worker Blob integration and Vite/Webpack fixtures.
- Task 8: explicit browser save bytes/Blob/download APIs.

## Latest Verification

Task 6 verification included:

```bash
npm test
npm run test:wasm
npm pack --dry-run
emcmake cmake -S . -B build/wasm-no-capi -DBUILD_WASM=ON -DBUILD_CAPI=OFF -DBUILD_TESTS=OFF
cmake --build build/wasm-no-capi --target wz_wasm
```

Task 5 verification included:

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=OFF
cmake --build build --target wz_test -j2
ctest --test-dir build -C Debug --output-on-failure
cpplint --recursive --config .cpplint src include tests
emcmake cmake -S . -B build/wasm-task5 -DBUILD_WASM=ON -DBUILD_TESTS=ON
cmake --build build/wasm-task5 --target wz_wasm_test
node build/wasm-task5/wz_wasm_test.js
$env:PYTHONUTF8='1'; npx node-gyp clean; npm run build
node --test tests/smoke.test.js
```

Notes:

- `node-gyp` and CMake both use `build/`; switching between them can invalidate
  the other tool's generated cache.
- On this Windows machine, `PYTHONUTF8=1` avoided node-gyp reading generated
  `.filters` files with the GBK locale.
- `node-gyp clean` removes `build/wasm`; if the Wasm test target must be
  rebuilt afterward, rerun `emcmake cmake`.
- A later Wasm verification used local cached googletest source from
  `.worktrees/wz-editing/build/_deps/googletest-src` because GitHub fetches
  intermittently failed.

## Workspace Notes

- Work is on branch `feat-wasm` in the current checkout, per user instruction.
- `.vscode/settings.json` is an unrelated, pre-existing untracked file and
  must not be staged or committed.
