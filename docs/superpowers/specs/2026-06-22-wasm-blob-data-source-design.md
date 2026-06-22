# WASM Blob Data Source Design

## Context

`libwz` currently parses WZ archives from `std::ifstream`. Browser users supply
a selected `Blob`, not a readable native path. WZ parsing and lazy image parsing
both require random access, while copying a large Blob through `arrayBuffer()`
into WebAssembly memory is unacceptable.

The project has one Node-API binding implementation (`src/node/binding.cpp`)
and one TypeScript wrapper (`index.ts`). WASM support must reuse both. Existing
path-based APIs and the C#-compatible WZ object model remain unchanged; all
browser capabilities are additive.

## Goals

- Build `libwz` for WebAssembly with Emscripten and emnapi.
- Read a browser Blob on demand without loading the complete Blob into Wasm
  memory.
- Preserve synchronous random seek/read semantics for the C++ parser.
- Reuse the existing Node-API binding and TypeScript object wrappers.
- Keep native API behavior and C#-compatible WZ semantics unchanged.
- Emit ESM-only browser artifacts compatible with Vite, Webpack, and similar
  bundlers; do not emit UMD or CommonJS browser loaders.
- Make browser save behavior explicit rather than implicit.

## Non-Goals

- Do not use Asyncify.
- Do not read Blob bytes synchronously from the browser main thread; browsers do
  not provide that capability.
- Do not expose DOM, Blob, Worker, or emnapi types in platform-neutral C++
  headers.
- Do not add a second set of directory/image/property bindings.
- Do not change the meaning of native `SaveToDisk(path)`.

## Architecture

### Random-access source

Introduce `WzDataSource` as the sole input dependency of `WzBinaryReader`:

```cpp
class WzDataSource {
 public:
  virtual ~WzDataSource() = default;
  virtual Result<size_t> ReadAt(uint64_t offset,
                                std::span<uint8_t> destination) = 0;
  virtual uint64_t Size() const = 0;
};
```

`WzBinaryReader` owns a source, a current offset, and a bounded read window
(default 256 KiB). Seeking changes only the current offset. Read methods use the
window for sequential data and refill it from the source when necessary.

`WzFile` owns the source until it and all lazily parsed images are destroyed.

### Native and memory sources

`WzStreamDataSource` adapts the existing `std::ifstream` path flow. Therefore
existing `WzFile(path, ...)`, `wz_open_file`, and all native callers preserve
their behavior.

`WzMemoryDataSource` owns a vector of bytes. It is added for unit tests and
small in-memory callers. Its C API explicitly copies caller bytes because WZ
images may be parsed lazily after the caller releases the original buffer.

### Browser Blob source

`WzBlobDataSource` is compiled only for Emscripten. It holds an opaque Blob
registry id and the Blob size; it never owns the Blob payload.

A dedicated Worker owns a registry from id to Blob. On a cache miss the source
requests a bounded range. The Worker uses `Blob.slice(offset, end)` and
`FileReaderSync.readAsArrayBuffer()` to synchronously obtain that range and
copies only the requested read window into Wasm memory. This preserves the
parser's synchronous C++ stack without Asyncify or a full-file Wasm copy.

`FileReaderSync` is unavailable on the main thread, so parsing and Blob access
run in a dedicated Worker. The main-thread library interface uses Worker RPC.
Closing a WZ file releases the corresponding registry id.

## Binding and TypeScript API

There remains one `src/node/binding.cpp` and one `index.ts`.

Existing synchronous Node use remains unchanged:

```ts
const file = new WzFile(path, MapleVersion.GMS);
file.parseWzFile();
```

Additive binding methods include `openMemory`, `openMemoryWithIv`, and, in a
WASM build, `openBlobSource` and `openBlobSourceWithIv`. They all return the
existing opaque `wz_file` pointer handle. Existing directory, image, property,
PNG, raw-data, and video methods therefore require no duplicate binding code.

The browser adds asynchronous initialization and Blob construction only:

```ts
await initializeWasm();
const file = await WzFile.fromBlob(blob, { gameVersion, mapleVersion });
```

After construction, the normal WZ object API runs synchronously inside the
Worker. The main thread receives promise-based RPC results.

## Browser Save Semantics

`saveFileToDisk(path)` retains its native meaning. In a browser build it returns
`NotImplemented`; it must not silently trigger a download.

Additive APIs are `saveToBytes()` / `saveToBlob()`, followed by an explicit
`download()` helper. A later `saveToFileHandle(handle)` may support a
caller-supplied File System Access API handle.

## Build and Packaging

Add a `BUILD_WASM` CMake option. It builds the same core `wz`, `wz_capi`, and
the existing Node-API binding source with Emscripten. JNI and native-only test
targets are excluded from this configuration.

Pin `@emnapi/runtime` in npm dependencies. The browser package emits ESM only:

- package metadata exposes an ESM entry with an `import` export condition;
- the runtime loader and Worker entry are ESM modules;
- no UMD or CommonJS fallback is emitted for browser consumers;
- emitted Wasm is loaded through bundler-recognized ESM URLs so Vite and
  Webpack can bundle it without runtime path guessing.

Existing native node-gyp scripts remain explicit and continue to build the
CommonJS native addon separately.

## Errors and Lifecycle

All new C++ fallible operations use `Result<T, Error>`; C API calls translate
them to existing `wz_error_code` values.

- Out-of-range or short Blob reads return `IoError`.
- A released Blob registry entry returns `IoError`.
- Browser-only calls in a native build return `NotImplemented`.
- Existing parse failures retain `WzFileParseStatus` behavior.

The Blob registry entry survives until `wz_close_file`, because later lazy image
parsing can require source data.

## Testing and Verification

1. Add C++ unit tests for stream and memory sources, seeks, cache boundaries,
   and source lifetime.
2. Add C API and Node tests for additive memory input while retaining current
   path-based regression tests.
3. Build the Emscripten target and load its ESM entry in a Worker test.
4. Add browser integration tests using a real Blob, asserting that full-file
   `Blob.arrayBuffer()` is never called and that lazy image reads request only
   ranges.
5. Test ESM imports with Vite and Webpack fixture builds.
6. After every C++ change, run clang-format and
   `cpplint --recursive --config .cpplint src include`; run CMake/CTest and
   relevant JavaScript tests before completion.
