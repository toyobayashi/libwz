# WASM Blob Data Source Progress — 2026-06-22

## Status

Implementation is paused at the end of plan Task 1. Do not start Task 2 until
Task 1 receives its final code-quality re-review for commit `cd97b9a`.

The user explicitly requested a pause because the remaining token budget is
too small for further safe implementation.

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

## Review State

Task 1 has completed specification review. Code-quality review found two real
issues, both fixed in follow-up commits:

1. Non-seekable streams looked like valid empty streams — fixed in `30a93376`.
2. Unexpected short reads after captured stream size looked successful — fixed
   in `cd97b9ad`.

Required next action: dispatch one fresh code-quality reviewer for the complete
Task 1 commit range through `cd97b9ad`. Only after approval should Task 1 be
marked complete and Task 2 begin.

## Not Started

- Task 2: migrate `WzBinaryReader` to `WzDataSource` and add its read cache.
- Task 3: source-backed `WzFile` and additive memory C API.
- Task 4: memory input through the existing Node-API binding and `index.ts`.
- Task 5: Emscripten-only `WzBlobDataSource` and additive Blob factories.
- Task 6: Emscripten/emnapi ESM-only package and Worker modules.
- Task 7: Worker Blob integration and Vite/Webpack fixtures.
- Task 8: explicit browser save bytes/Blob/download APIs.

## Workspace Notes

- Work is on branch `feat-wasm` in the current checkout, per user instruction.
- `.vscode/settings.json` is an unrelated, pre-existing untracked file and
  must not be staged or committed.
