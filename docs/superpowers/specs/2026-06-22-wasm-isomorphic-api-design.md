# WASM Isomorphic API Design

## Goal

Make the browser API feel like the existing Node API while respecting browser
asynchrony. Users should be able to learn one object model and use it in three
places:

- Node.js through the existing `libwz` entry.
- Browser main thread through an async Worker proxy.
- User-owned Web Workers through a direct Wasm API.

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

### Browser Main Thread Entry: `libwz/wasm`

The main-thread entry returns an async proxy client:

```ts
import { createWzWorker, MapleVersion } from "libwz/wasm";

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

### User Worker Entry: `libwz/wasm/direct`

Users who are already inside their own Worker should not be forced through a
second Worker. They can initialize Wasm directly and use a synchronous API that
matches Node:

```ts
import { createWzApi, MapleVersion } from "libwz/wasm/direct";

const wz = await createWzApi({ wasmUrl });

using file = wz.WzFile.fromBlob("Base.wz", blob, MapleVersion.GMS);
file.parseWzFile();
const root = file.getWzDirectory();
```

Rules:

- Initialization is async because Wasm loading is async.
- After initialization, object methods are synchronous.
- Direct objects support `Symbol.dispose`.
- `fromBlob()` is allowed because `FileReaderSync` is available in Workers.
- If direct APIs are used on a browser main thread and require synchronous Blob
  reads, they must throw a clear unsupported-environment error.

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

The direct Wasm API can reuse the same wrapper logic as Node where practical by
injecting a `NativeBinding`-compatible implementation. The async proxy API uses
similar wrappers, but every native call is sent through RPC and returns a
`Promise`.

## Construction APIs

The browser API should provide these file constructors:

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

`fromBlob` and `fromFile` use the Blob range reader and must not call full
`blob.arrayBuffer()`. `fromBytes` may copy bytes into Wasm memory because the
caller already owns the bytes.

## Disposal Semantics

### Node and Direct Worker APIs

Owning objects keep `Symbol.dispose`:

```ts
using file = wz.WzFile.fromBlob("Base.wz", blob, MapleVersion.GMS);
```

Disposal closes the underlying `wz_file` handle and invalidates known borrowed
wrappers, matching current Node behavior.

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
};
```

## Save APIs

Browser APIs must not make `saveToDisk(path)` trigger a download implicitly.
Direct and proxy browser APIs should report `saveToDisk` as unsupported unless
an Emscripten filesystem path is intentionally introduced later.

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

- Direct Worker API throws synchronously after initialization.
- Main-thread proxy API rejects promises with `Error` objects.
- Worker-side native errors include the C API last-error message.
- Unsupported environment errors should name the missing feature, such as
  `FileReaderSync is required for Blob-backed WZ reads`.

## Testing Strategy

Tests should prove API parity, not just module loading:

- A shared fixture test should open the same WZ file through Node `fromBytes`,
  direct Wasm `fromBlob`, and async proxy `fromBlob`.
- Assertions should compare parse status, root directory name, image counts,
  and at least one lazy image parse.
- A Blob test should override `blob.arrayBuffer()` to throw, proving the Blob
  path uses range reads instead of full reads.
- Async proxy tests should use `await using` and verify disposed wrappers reject
  follow-up calls.
- Direct Worker tests should use `using` and verify disposed wrappers throw.
- Bundler tests should import `libwz/wasm` from Vite and Webpack fixtures
  without `require`, UMD, or manual Wasm copying.

## Implementation Direction

1. Extract the current Node wrapper so it can accept an injected binding instead
   of closing over the Node native addon directly.
2. Build `wasm/direct.ts` on top of the injected binding returned by
   `initializeWasm()`.
3. Expand the Worker to expose the same binding operations over RPC.
4. Build async proxy wrappers with the same class and method names as the direct
   wrappers, returning promises and supporting `Symbol.asyncDispose`.
5. Keep raw N-API handles private to the direct binding and Worker internals.

## Non-Goals

- Do not make the Node API asynchronous.
- Do not expose raw handles as public browser API.
- Do not support CommonJS, UMD, or non-module browser loaders.
- Do not implement implicit browser downloads through `saveToDisk(path)`.
- Do not copy entire Blob inputs into Wasm memory for Blob-backed reads.
