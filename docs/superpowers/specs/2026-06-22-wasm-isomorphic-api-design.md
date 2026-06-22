# WASM Isomorphic API Design

## Goal

Make the browser API feel like the existing Node API while respecting browser
asynchrony. Users should be able to learn one object model and use it in four
places:

- Node.js through the existing `libwz` entry.
- Node.js through the Wasm direct target, without a Worker proxy.
- Browser Workers through the same Wasm direct target.
- Browser main thread through an async Worker proxy.

The public browser API must not expose raw native handles. Handles remain an
internal implementation detail of the RPC layer.

## Current Problem

The current `libwz/wasm` export only exposes a thin `createWzWorker()` helper
and a very small Blob-open RPC. That proves the Wasm module can load, but it is
not enough for users who want the same `WzFile`, `WzDirectory`, `WzImage`, and
property workflow that exists in Node.

The next design step is to make browser usage match the Node package shape as
closely as possible. The unavoidable difference is that main-thread Worker
communication is asynchronous, so proxy methods return `Promise` values.

## API Surfaces

### Node Entry: `libwz`

The existing Node API remains synchronous and unchanged:

```ts
import { MapleVersion, WzFile } from "libwz";

using file = WzFile.fromBytes("Base.wz", bytes, MapleVersion.GMS);
file.parseWzFile();
const root = file.getWzDirectory();
```

This entry continues to load the native Node addon and is not browser-safe.

### Direct Wasm Entry: `libwz/wasm`

`libwz/wasm` is the direct Wasm runtime. It is meant for Node.js and for users
who are already inside their own browser Worker. It does not create another
Worker. It can initialize the Wasm module asynchronously through
`createWzApi()` or synchronously through `createWzApiSync()` when the caller
provides synchronously available Wasm input. After initialization, both forms
expose synchronous Node-like object APIs:

```ts
import { createWzApi, MapleVersion } from "libwz/wasm";

const wz = await createWzApi({ wasmUrl });

using file = new wz.WzFile("Base.wz", MapleVersion.GMS);
file.parseWzFile();
const root = file.getWzDirectory();
```

Node.js callers that want no async initialization can use sync loading:

```ts
import { createWzApiSync, MapleVersion } from "libwz/wasm";

const wz = createWzApiSync({ wasmPath: "node_modules/libwz/dist/wasm/libwz.wasm" });

using file = new wz.WzFile("Base.wz", MapleVersion.GMS);
file.parseWzFile();
```

In a browser Worker, the same entry is used with Blob/File input:

```ts
import { createWzApi, MapleVersion } from "libwz/wasm";

const wz = await createWzApi({ wasmUrl });

using file = wz.WzFile.fromBlob("Base.wz", blob, MapleVersion.GMS);
file.parseWzFile();
const root = file.getWzDirectory();
```

Rules:

- `createWzApi()` initializes asynchronously because URL-based Wasm loading is
  asynchronous.
- After initialization, object methods are synchronous.
- `createWzApiSync()` is available for runtimes that can provide Wasm bytes,
  a compiled `WebAssembly.Module`, or a Node.js filesystem path synchronously.
  It must not perform asynchronous fetch.
- The runtime detects whether it is running in Node.js or a browser-like
  Worker environment.
- In Node.js, path-based `new WzFile(...)` and `saveToDisk(path)` are supported
  when the configured Emscripten filesystem can access the requested path.
- In browser Workers, Blob/File input is supported through `FileReaderSync`;
  path-based input and `saveToDisk(path)` are unsupported unless a caller
  explicitly configures an Emscripten filesystem mount later.
- Direct Wasm APIs support `Symbol.dispose`, not `Symbol.asyncDispose`, because
  there is no Worker round trip after initialization.
- The public API should look like the native Node API regardless of host.

### Browser Main Thread Entry: `libwz/wasm/browser-main`

The main-thread entry returns an async proxy client:

```ts
import { createWzWorker, MapleVersion } from "libwz/wasm/browser-main";

await using wz = await createWzWorker({ wasmUrl });
await using file = await wz.WzFile.fromBlob(
  "Base.wz",
  blob,
  MapleVersion.GMS,
);

const status = await file.parseWzFile();
const root = await file.getWzDirectory();
const image = await root.getImage(0);
await image.parseImage();
```

Rules:

- Class and method names mirror the Node API.
- Methods that cross the Worker boundary return `Promise`.
- Object wrappers represent remote objects, not raw `bigint` handles.
- `createWzWorker()` returns a client with the same class namespace shape as the
  direct API, plus Worker lifecycle controls.
- The client and remote owning objects support `Symbol.asyncDispose`.

## Shared Object Model

Browser APIs should expose the same conceptual classes as Node:

- `WzObject`
- `WzFile`
- `WzDirectory`
- `WzImage`
- `WzImageProperty`
- Concrete property classes where the Node API has them
- `WzProperty` static factory namespace
- `WzTool` helpers
- `MapleVersion`, `ParseStatus`, `PropertyType`, `ObjectType`,
  `BinaryType`

The direct Wasm APIs can reuse the same wrapper logic as Node where practical
by injecting a `NativeBinding`-compatible implementation. The async proxy API
uses similar wrappers, but every native call is sent through RPC and returns a
`Promise`.

The direct runtime chooses a host adapter at initialization:

- Node.js adapter: synchronous path I/O is available.
- Browser Worker adapter: synchronous Blob range reads are available, but
  arbitrary host filesystem paths are not.
- Browser main-thread direct usage is allowed only for byte-backed operations
  that do not need synchronous Blob reads; Blob/File direct input must throw a
  clear unsupported-environment error on the main thread.

## Construction APIs

The Wasm APIs should provide these file constructors:

```ts
wz.WzFile.fromBlob(name, blob, mapleVersion);
wz.WzFile.fromBlob(name, blob, gameVersion, mapleVersion);
wz.WzFile.fromBlobWithIv(name, blob, iv);
wz.WzFile.fromFile(file, mapleVersion);
wz.WzFile.fromFile(file, gameVersion, mapleVersion);
wz.WzFile.fromBytes(name, bytes, mapleVersion);
wz.WzFile.fromBytes(name, bytes, gameVersion, mapleVersion);
wz.WzFile.fromBytesWithIv(name, bytes, iv);
```

Node.js Wasm direct also supports the path constructors from the native Node
entry:

```ts
new wz.WzFile(path, mapleVersion);
new wz.WzFile(path, gameVersion, mapleVersion);
new wz.WzFile(path, iv);
```

`fromBlob` and `fromFile` use the Blob range reader and must not call full
`blob.arrayBuffer()`. `fromBytes` may copy bytes into Wasm memory because the
caller already owns the bytes. Node.js callers can use paths, bytes, or
Blob-compatible objects depending on their deployment constraints.

## Disposal Semantics

### Node and Direct Wasm APIs

Owning objects keep `Symbol.dispose`:

```ts
using file = wz.WzFile.fromBlob("Base.wz", blob, MapleVersion.GMS);
```

Disposal closes the underlying `wz_file` handle and invalidates known borrowed
wrappers, matching current Node behavior. This applies to the native Node entry
and the direct Wasm entry in both Node.js and browser Worker environments.

### Main Thread Async Proxy API

Remote owning objects and the Worker client support `Symbol.asyncDispose`:

```ts
await using wz = await createWzWorker({ wasmUrl });
await using file = await wz.WzFile.fromBlob("Base.wz", blob, MapleVersion.GMS);
```

`RemoteWzFile[Symbol.asyncDispose]()` sends a close RPC that:

- Closes the `wz_file` handle in the Worker.
- Releases the associated Blob registry entry when the file was opened from a
  Blob or File.
- Invalidates known remote wrappers for descendants of that file.

`WzWorkerClient[Symbol.asyncDispose]()` terminates the Worker after rejecting
pending RPC calls and requesting best-effort cleanup.

Repeated dispose calls are no-ops. Calls on disposed remote wrappers reject with
an error whose message includes `disposed`.

## Worker RPC Design

The RPC protocol should be object-oriented at the wrapper boundary and
function-oriented on the wire:

```ts
type Request = {
  id: number;
  method: string;
  args: unknown[];
};

type Response =
  | { id: number; ok: true; value: unknown }
  | { id: number; ok: false; error: { name: string; message: string } };
```

Remote objects are serialized as references:

```ts
type RemoteRef = {
  kind: "file" | "directory" | "image" | "property" | "object";
  handle: string;
  type?: number;
  ownerFile?: string;
};
```

`bigint` handles should be converted to strings for message payloads so the
protocol is explicit and easy to inspect. The Worker converts strings back to
`bigint` before calling the Wasm binding.

Binary data returned by APIs such as PNG or raw binary reads should be returned
as `Uint8Array` and transferred when possible.

## Customization

`createWzWorker()` accepts:

```ts
type WzWorkerOptions = {
  wasmUrl?: string | URL;
  workerUrl?: string | URL;
  name?: string;
  workerFactory?: (url: string | URL) => Worker;
  onError?: (error: Error) => void;
};
```

This lets users integrate with bundlers, CSP setups, custom Worker hosting, or
test harnesses. If `wasmUrl` is omitted, the default URL is resolved relative to
the package's generated ESM files.

`createWzApi()` accepts:

```ts
type WzApiOptions = {
  wasmUrl?: string | URL;
  mount?: {
    hostPath: string;
    wasmPath: string;
  };
};
```

`createWzApiSync()` accepts:

```ts
type WzApiSyncOptions = {
  wasmPath?: string;
  wasmBytes?: Uint8Array | ArrayBuffer;
  wasmModule?: WebAssembly.Module;
  mount?: {
    hostPath: string;
    wasmPath: string;
  };
};
```

Exactly one of `wasmPath`, `wasmBytes`, or `wasmModule` should be provided for
sync initialization unless the runtime can synchronously find the packaged
`libwz.wasm`. In Node.js, `wasmPath` is read with synchronous filesystem APIs.
In browser Workers, callers should pass `wasmBytes` or `wasmModule` because
fetching package assets is asynchronous. If sync initialization cannot obtain
Wasm synchronously, it must throw a setup error that names the missing input.

The sync path should reuse the same `libwz.wasm` binary as the async path. It
may use a separate Emscripten JavaScript glue artifact built with synchronous
Wasm compilation settings, such as `WASM_ASYNC_COMPILATION=0`, but it should
not publish a second `.wasm` binary unless an implementation constraint is
verified during development. The async and sync APIs should return the same
object model even if their generated glue files are different internally.

`mount` is used only by the Node.js host adapter. If no mount is provided,
Node.js direct should use a sensible default that allows ordinary absolute paths
and relative paths from `process.cwd()` when possible. If the runtime cannot
provide path access, path constructors must throw a clear setup error instead of
silently behaving like an empty file.

## Save APIs

Browser APIs must not make `saveToDisk(path)` trigger a download implicitly.
Direct and proxy browser APIs should report `saveToDisk` as unsupported unless
an Emscripten filesystem path is intentionally introduced later.

Node.js Wasm direct should support `saveToDisk(path)` when the configured
filesystem adapter can write to that path, matching the native Node API.

Future explicit browser save APIs should be:

```ts
file.saveToBytes();
file.saveToBlob();
file.download(filename);
```

`download(filename)` must be explicit and should be implemented only as a
browser helper over `saveToBlob()`.

## Error Handling

Node errors currently become thrown JavaScript exceptions from the addon. The
browser APIs should preserve that shape:

- Node.js Wasm direct and browser Worker direct APIs throw synchronously after
  initialization. `createWzApiSync()` also throws synchronously for setup
  failures.
- Main-thread proxy API rejects promises with `Error` objects.
- Worker-side native errors include the C API last-error message.
- Unsupported environment errors should name the missing feature, such as
  `FileReaderSync is required for Blob-backed WZ reads`.

## Testing Strategy

Tests should prove API parity, not just module loading:

- A shared fixture test should open the same WZ file through native Node
  `fromBytes`, direct Wasm path input in Node.js, direct Wasm `fromBlob` in a
  browser-like Worker, and async proxy `fromBlob` from the main-thread entry.
- Assertions should compare parse status, root directory name, image counts,
  and at least one lazy image parse.
- A Blob test should override `blob.arrayBuffer()` to throw, proving the Blob
  path uses range reads instead of full reads.
- Direct Wasm tests should cover async initialization, sync initialization,
  Node.js path input, and browser Worker Blob input through the same
  `libwz/wasm` import.
- Browser main-thread proxy tests should import `libwz/wasm/browser-main`, use
  `await using`, and verify disposed wrappers reject follow-up calls.
- Bundler tests should import `libwz/wasm` and `libwz/wasm/browser-main` from
  Vite and Webpack fixtures without `require`, UMD, or manual Wasm copying.

## Implementation Direction

1. Extract the current Node wrapper so it can accept an injected binding instead
   of closing over the Node native addon directly.
2. Build a shared direct Wasm wrapper on top of the injected binding returned
   by `initializeWasm()` and `initializeWasmSync()`.
3. Add host adapters for Node.js path I/O and browser Worker Blob range reads.
4. Expose the direct runtime as `libwz/wasm` and choose the Node.js or browser
   Worker host adapter at runtime.
5. Expand the Worker to expose the same binding operations over RPC.
6. Expose the async proxy as `libwz/wasm/browser-main`, with the same class and
   method names as the direct wrappers, returning promises and supporting
   `Symbol.asyncDispose`.
7. Keep raw N-API handles private to the direct binding and Worker internals.

## Non-Goals

- Do not make the Node API asynchronous.
- Do not force Node.js Wasm users through a Worker proxy.
- Do not expose raw handles as public browser API.
- Do not support CommonJS, UMD, or non-module browser loaders.
- Do not implement implicit browser downloads through `saveToDisk(path)`.
- Do not copy entire Blob inputs into Wasm memory for Blob-backed reads.
