# WASM Isomorphic API Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Provide Node-like `WzFile`, `WzDirectory`, `WzImage`, and property APIs for the Wasm build across Node.js direct usage, browser Worker direct usage, and browser main-thread async proxy usage.

**Architecture:** Extract the existing Node TypeScript object wrappers so they can be constructed from an injected `NativeBinding`. Build direct Wasm runtimes on that wrapper for Node.js and browser Workers, then build an async Worker proxy with the same class and method names over a small RPC protocol. Raw native handles remain private.

**Tech Stack:** TypeScript 5.9, ESM, Emscripten/emnapi, Node test runner, CMake, existing Node-API binding.

---

## File Structure

- Modify: `index.ts`
  - Keep the public native Node API unchanged.
  - Re-export wrappers created from the native addon through a shared wrapper factory.
- Create: `src/node-wrapper.ts`
  - Own all synchronous object classes and helper factories.
  - Accept an injected `NativeBinding`.
  - Export `createWzApiFromBinding(binding, capabilities)`.
- Create: `src/native-binding.ts`
  - Define `NativeBinding`, handle types, and enums shared by Node and Wasm TS code.
- Create: `wasm/index.ts`
  - Direct Wasm API using `initializeWasm()`, runtime host detection, Node path I/O, browser Worker Blob range reads, and synchronous wrappers.
- Modify: `wasm/loader.ts`
  - Export typed `WzBinding` instead of `Record<string, unknown>`.
  - Accept host hooks for Blob range and Node path access.
- Modify: `wasm/worker.ts`
  - Replace the current Blob-open-only message handler with generic RPC.
  - Keep `__libwzReadBlobRange` in the Worker.
- Create: `wasm/browser-main.ts`
  - Export async proxy wrappers and `createWzWorker()` for browser main-thread usage.
  - Support `Symbol.asyncDispose`.
- Modify: `wasm/tsconfig.json`
  - Include shared wrapper source or configure paths so wasm entries can import it.
- Modify: `package.json`
  - Point `./wasm` at the direct runtime.
  - Add `./wasm/browser-main` for the async Worker proxy.
  - Keep native `main` and root `require` entry unchanged.
- Create: `tests/wasm/direct.test.mjs`
  - Tests Node.js direct path input and browser Worker direct Blob behavior through the same `libwz/wasm` API.
- Create: `tests/wasm/browser-main.test.mjs`
  - Tests async proxy API and `Symbol.asyncDispose`.
- Create: `tests/wasm/parity.test.mjs`
  - Compares native Node, Node Wasm direct, browser Worker direct, and async proxy parse results.
- Create: `tests/wasm/vite/*`, `tests/wasm/webpack/*`
  - Verify bundler imports for `libwz/wasm` and `libwz/wasm/browser-main`.

## Task 1: Extract Shared Binding Types

**Files:**
- Create: `src/native-binding.ts`
- Modify: `index.ts`
- Test: `tests/smoke.test.js`

- [ ] **Step 1: Add baseline type-only build check**

Create `src/native-binding.ts` with the shared declarations currently embedded in `index.ts`. The first version should export only the minimum names and intentionally not be imported by `index.ts` yet:

```ts
export type NativeHandle = bigint;
export type NullableNativeHandle = NativeHandle | null;
export type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray;

export interface NativeObjectInfo {
  type: number;
  ptr: NativeHandle;
}
```

Run:

```bash
npm run build:ts
```

Expected: PASS. This step establishes the new shared file without behavior changes.

- [ ] **Step 2: Move the complete binding interface**

Move these declarations from `index.ts` into `src/native-binding.ts`:

```ts
export type NativeHandle = bigint;
export type NullableNativeHandle = NativeHandle | null;
export type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray;

export interface NativeObjectInfo {
  type: ObjectTypeValue;
  ptr: NativeHandle;
}

export interface DetectedMapleVersion {
  mapleVersion: MapleVersionValue;
  version: number;
}

export interface NativeBinding {
  openFile(path: string, gameVersion: number, mapleVersion: MapleVersionValue): NullableNativeHandle;
  openFileWithIv(path: string, iv: ArrayBufferViewLike): NullableNativeHandle;
  openMemory(name: string, bytes: Uint8Array, gameVersion: number, mapleVersion: MapleVersionValue): NullableNativeHandle;
  openMemoryWithIv(name: string, bytes: Uint8Array, iv: ArrayBufferViewLike): NullableNativeHandle;
  openBlobSource?(id: number, size: number, name: string, gameVersion: number, mapleVersion: MapleVersionValue): NullableNativeHandle;
  openBlobSourceWithIv?(id: number, size: number, name: string, iv: ArrayBufferViewLike): NullableNativeHandle;
  createFile(gameVersion: number, mapleVersion: MapleVersionValue): NullableNativeHandle;
  closeFile(ptr: NativeHandle): void;
  parseFile(ptr: NativeHandle): ParseStatusValue;
  fileSaveToDisk(ptr: NativeHandle, path: string): void;
  fileName(ptr: NativeHandle): string;
  filePath(ptr: NativeHandle): string;
  fileVersion(ptr: NativeHandle): number;
  fileMapleVersion(ptr: NativeHandle): MapleVersionValue;
  fileWzDirectory(ptr: NativeHandle): NullableNativeHandle;
  fileIs64Bit(ptr: NativeHandle): boolean;
  fileIsUnloaded(ptr: NativeHandle): boolean;
  fileVersionHash(ptr: NativeHandle): number;
  fileObjectFromPath(ptr: NativeHandle, path: string, checkFirstDirectoryName: boolean): NativeObjectInfo | null;
  objectType(ptr: NativeHandle): ObjectTypeValue;
  objectName(ptr: NativeHandle): string;
  objectParent(ptr: NativeHandle): NativeObjectInfo | null;
  objectFullPath(ptr: NativeHandle): string;
  objectWzFileParent(ptr: NativeHandle): NullableNativeHandle;
  objectTopMostDirectory(ptr: NativeHandle): NativeObjectInfo | null;
  objectTopMostImage(ptr: NativeHandle): NativeObjectInfo | null;
  objectAt(ptr: NativeHandle, name: string): NativeObjectInfo | null;
  objectSetName(ptr: NativeHandle, name: string): void;
  objectRemove(ptr: NativeHandle): void;
}
```

Keep the existing full method list from `index.ts`; do not truncate it to the snippet above. Import the moved types back into `index.ts`.

- [ ] **Step 3: Run native wrapper verification**

Run:

```bash
npm test
```

Expected: PASS with the existing native Node smoke tests.

- [ ] **Step 4: Commit**

```bash
git add index.ts src/native-binding.ts
git commit -m "refactor: share Node binding TypeScript types"
```

## Task 2: Extract Synchronous Wrapper Factory

**Files:**
- Create: `src/node-wrapper.ts`
- Modify: `index.ts`
- Test: `tests/smoke.test.js`

- [ ] **Step 1: Write failing import check**

Add a temporary test to `tests/smoke.test.js`:

```js
test("exports wrapper factory backed by native binding", () => {
  assert.equal(typeof wz.createWzApiFromBinding, "undefined");
});
```

Run:

```bash
npm test
```

Expected: PASS. This guards that the factory remains internal and is not exported from the root package.

- [ ] **Step 2: Move wrapper implementation**

Create `src/node-wrapper.ts` and move the existing classes and helper functions from `index.ts` into a factory:

```ts
import type { NativeBinding, NativeHandle, NativeObjectInfo } from "./native-binding";

export interface WzApiCapabilities {
  blobInput: boolean;
  pathInput: boolean;
  saveToDisk: boolean;
}

export function createWzApiFromBinding(native: NativeBinding, capabilities: WzApiCapabilities) {
  class WzObject {
    protected _ptr: NativeHandle;
    protected _owner: WzFile | null;

    constructor(ptr: NativeHandle, owner: WzFile | null = null) {
      this._ptr = ptr;
      this._owner = owner;
    }

    nativePtr(): NativeHandle {
      if (this._ptr === 0n) throw new Error("native object is disposed");
      return this._ptr;
    }

    getName(): string {
      return native.objectName(this.nativePtr());
    }
  }

  class WzFile extends WzObject {
    constructor(pathOrPtr: string | NativeHandle, gameVersionOrMapleVersion: number | Uint8Array, mapleVersion?: number) {
      if (typeof pathOrPtr === "bigint") {
        super(pathOrPtr, null);
        return;
      }
      if (!capabilities.pathInput) {
        throw new Error("path-based WZ input is not supported in this runtime");
      }
      const ptr = native.openFile(pathOrPtr, 0, Number(gameVersionOrMapleVersion));
      if (!ptr) throw new Error("failed to open WZ file");
      super(ptr, null);
    }

    parseWzFile(): number {
      return native.parseFile(this.nativePtr());
    }
  }

  return {
    WzObject,
    WzFile,
  };
}
```

Move the complete current implementation, including invalidation, concrete
property classes, `WzProperty`, `WzTool`, enums, and helper wrappers. Keep the
existing method bodies from `index.ts` unchanged except for replacing the closed
over `native` binding with the factory parameter.

- [ ] **Step 3: Rebuild root API from the factory**

In `index.ts`, load the native addon as before, call the factory, and export the returned classes and namespaces:

```ts
const native = loadNative();
const api = createWzApiFromBinding(native, {
  blobInput: false,
  pathInput: true,
  saveToDisk: true,
});

export const WzObject = api.WzObject;
export const WzFile = api.WzFile;
export const WzDirectory = api.WzDirectory;
export const WzImage = api.WzImage;
export const WzImageProperty = api.WzImageProperty;
export const WzProperty = api.WzProperty;
export const WzTool = api.WzTool;
```

- [ ] **Step 4: Run native tests**

Run:

```bash
npm test
```

Expected: PASS. No public root API behavior changes.

- [ ] **Step 5: Commit**

```bash
git add index.ts src/node-wrapper.ts tests/smoke.test.js
git commit -m "refactor: create injectable WZ TypeScript wrappers"
```

## Task 3: Add Direct Wasm Runtime for Node.js and Browser Workers

**Files:**
- Create: `wasm/index.ts`
- Modify: `wasm/loader.ts`
- Modify: `wasm/tsconfig.json`
- Modify: `package.json`
- Test: `tests/wasm/direct-worker.mjs`
- Test: `tests/wasm/direct.test.mjs`

- [ ] **Step 1: Write failing direct runtime tests**

Create `tests/wasm/direct-worker.mjs`:

```js
import { parentPort, workerData } from "node:worker_threads";
import { createWzApi, MapleVersion } from "../../dist/wasm/index.js";

try {
  const bytes = new Uint8Array(workerData.bytes);
  const blob = new Blob([bytes]);
  Object.defineProperty(blob, "arrayBuffer", {
    value() {
      throw new Error("full Blob read is forbidden");
    },
  });

  const wz = await createWzApi();
  using file = wz.WzFile.fromBlob("Character.wz", blob, MapleVersion.GMS);
  const status = file.parseWzFile();
  const root = file.getWzDirectory();
  parentPort.postMessage({ ok: true, status, name: root.getName() });
} catch (error) {
  parentPort.postMessage({ ok: false, message: error.message });
}
```

Create `tests/wasm/direct.test.mjs`:

```js
import assert from "node:assert/strict";
import fs from "node:fs";
import test from "node:test";
import path from "node:path";
import { Worker } from "node:worker_threads";
import { fileURLToPath } from "node:url";
import { createWzApi, MapleVersion, ParseStatus } from "../../dist/wasm/index.js";

const root = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))));
const sample = path.join(root, "Harepacker-resurrected", "UnitTest_WzFile", "WzFiles", "Character.wz");

test("direct wasm API opens path input in Node.js without worker proxy", async () => {
  const wz = await createWzApi();
  using file = new wz.WzFile(sample, MapleVersion.GMS);
  assert.equal(file.parseWzFile(), ParseStatus.SUCCESS);
  assert.equal(typeof file.getWzDirectory().getName(), "string");
});

test("browser Worker direct API opens Blob without full arrayBuffer", async () => {
  const bytes = fs.readFileSync(sample);
  const worker = new Worker(new URL("./direct-worker.mjs", import.meta.url), {
    workerData: { bytes: bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) },
  });
  const result = await new Promise((resolve, reject) => {
    worker.once("message", resolve);
    worker.once("error", reject);
  });
  assert.equal(result.ok, true, result.message);
  assert.equal(result.status, ParseStatus.SUCCESS);
});
```

Run:

```bash
npm run build:wasm
node tests/wasm/direct.test.mjs
```

Expected: FAIL because `dist/wasm/index.js` does not export `createWzApi`.

- [ ] **Step 2: Export the direct Wasm package entry**

Update `package.json` so `./wasm` points to the direct runtime:

```json
"./wasm": {
  "types": "./dist/wasm/index.d.ts",
  "import": "./dist/wasm/index.js"
}
```

The old `./wasm` browser proxy export will move to `./wasm/browser-main` in
Task 5.

- [ ] **Step 3: Implement `createWzApi()` with runtime host detection**

Create `wasm/index.ts`:

```ts
import { createWzApiFromBinding } from "../src/node-wrapper.js";
import { initializeWasm, loadWzModule, type WzBinding } from "./loader.js";

const blobs = new Map<number, Blob>();
let nextBlobId = 1;

export interface WzApiOptions {
  wasmUrl?: string | URL;
  mount?: {
    hostPath: string;
    wasmPath: string;
  };
}

export async function createWzApi(options: WzApiOptions = {}) {
  const loaded = options.wasmUrl
    ? await initializeWasm(options.wasmUrl)
    : await loadWzModule();
  const { binding, module } = loaded;

  const reader = new FileReaderSync();
  globalThis.__libwzReadBlobRange = (id, offset, destination, length) => {
    const blob = blobs.get(id);
    if (!blob) return -1;
    const start = Math.trunc(offset);
    const end = Math.min(start + length, blob.size);
    const bytes = new Uint8Array(reader.readAsArrayBuffer(blob.slice(start, end)));
    module.HEAPU8.set(bytes, destination);
    return bytes.byteLength;
  };

  const api = createWzApiFromBinding(binding as WzBinding, {
    blobInput: true,
    pathInput: false,
    saveToDisk: false,
  });

  api.WzFile.fromBlob = (name, blob, gameVersionOrMapleVersion, mapleVersion) => {
    const id = nextBlobId++;
    blobs.set(id, blob);
    const file = api.WzFile.fromBlobSource(name, id, blob.size, gameVersionOrMapleVersion, mapleVersion);
    file.addDisposeCallback(() => blobs.delete(id));
    return file;
  };

  return api;
}
```

Extend this implementation with Node.js host detection:

```ts
const isNode =
  typeof process === "object" &&
  !!process.versions?.node;

const capabilities = {
  blobInput: true,
  pathInput: isNode,
  saveToDisk: isNode,
};
```

When `isNode` is true, configure path access with `NODERAWFS` or `NODEFS`.
When `isNode` is false and `FileReaderSync` is unavailable, `fromBlob` and
`fromFile` must throw `FileReaderSync is required for Blob-backed WZ reads`.

Update `wasm/libwz.d.ts` so `LibwzEmscriptenModule` includes filesystem
runtime properties used by the Node host adapter:

```ts
export interface LibwzEmscriptenModule {
  HEAPU8: Uint8Array;
  FS?: {
    mkdirTree(path: string): void;
    mount(type: unknown, options: object, mountpoint: string): void;
  };
  NODEFS?: unknown;
  emnapiInit(options: { context: unknown; filename?: string }): unknown;
}
```

- [ ] **Step 4: Add wrapper helpers and Emscripten FS exports**

Add these internal helpers to `src/node-wrapper.ts`:

```ts
class WzFile extends WzObject {
  private disposeCallbacks: Array<() => void> = [];

  static fromBlobSource(
    name: string,
    id: number,
    size: number,
    gameVersionOrMapleVersion: number | MapleVersionValue,
    mapleVersion?: MapleVersionValue,
  ): WzFile {
    const version = mapleVersion ?? (gameVersionOrMapleVersion as MapleVersionValue);
    const gameVersion = mapleVersion === undefined ? 0 : Number(gameVersionOrMapleVersion);
    const openBlobSource = native.openBlobSource;
    if (!capabilities.blobInput || !openBlobSource) {
      throw new Error("Blob-backed WZ input is not supported in this runtime");
    }
    const ptr = openBlobSource(id, size, name, gameVersion, version);
    return new WzFile(requireHandle(ptr));
  }

  addDisposeCallback(callback: () => void): void {
    this.disposeCallbacks.push(callback);
  }

  protected disposeOwnedHandle(ptr: NativeHandle): void {
    native.closeFile(ptr);
    for (const callback of this.disposeCallbacks.splice(0)) callback();
  }
}
```

In `CMakeLists.txt`, add the runtime methods required by Node filesystem
access:

```cmake
"-sEXPORTED_RUNTIME_METHODS=['emnapiInit','HEAPU8','FS','NODEFS']"
"-sNODERAWFS=1"
```

- [ ] **Step 5: Run direct runtime verification**

Run:

```bash
npm run build:wasm
node tests/wasm/direct.test.mjs
```

Expected: PASS and no full Blob `arrayBuffer()` call.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt package.json src/node-wrapper.ts wasm/index.ts wasm/libwz.d.ts tests/wasm/direct-worker.mjs tests/wasm/direct.test.mjs
git commit -m "feat: add direct wasm API"
```

## Task 4: Expand Worker RPC Protocol

**Files:**
- Modify: `wasm/worker.ts`
- Create: `wasm/rpc.ts`
- Test: `tests/wasm/browser-main.test.mjs`

- [ ] **Step 1: Write failing async proxy test**

Create `tests/wasm/browser-main.test.mjs`:

```js
import assert from "node:assert/strict";
import fs from "node:fs";
import test from "node:test";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createWzWorker, MapleVersion, ParseStatus } from "../../dist/wasm/browser-main.js";

const root = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))));
const sample = path.join(root, "Harepacker-resurrected", "UnitTest_WzFile", "WzFiles", "Character.wz");

test("async proxy mirrors WzFile parse and async disposal", async () => {
  await using wz = await createWzWorker();
  const blob = new Blob([fs.readFileSync(sample)]);
  await using file = await wz.WzFile.fromBlob("Character.wz", blob, MapleVersion.GMS);
  assert.equal(await file.parseWzFile(), ParseStatus.SUCCESS);
  const rootDir = await file.getWzDirectory();
  assert.equal(typeof await rootDir.getName(), "string");
});
```

Run:

```bash
npm run build:wasm
node tests/wasm/browser-main.test.mjs
```

Expected: FAIL because async proxy wrappers do not exist.

- [ ] **Step 2: Add shared RPC types**

Create `wasm/rpc.ts`:

```ts
export type RpcRequest = {
  id: number;
  method: string;
  args: unknown[];
};

export type RpcResponse =
  | { id: number; ok: true; value: unknown }
  | { id: number; ok: false; error: { name: string; message: string } };

export type RemoteRef = {
  kind: "file" | "directory" | "image" | "property" | "object";
  handle: string;
  type?: number;
  ownerFile?: string;
};
```

- [ ] **Step 3: Implement Worker-side dispatch**

In `wasm/worker.ts`, initialize the direct browser Worker API and dispatch generic requests:

```ts
import { createWzApi } from "./index.js";
import type { RpcRequest, RpcResponse } from "./rpc.js";

let apiPromise = createWzApi();

self.addEventListener("message", (event: MessageEvent<RpcRequest>) => {
  void handleRequest(event.data);
});

async function handleRequest(request: RpcRequest): Promise<void> {
  try {
    const api = await apiPromise;
    const value = await dispatch(api, request.method, request.args);
    post({ id: request.id, ok: true, value });
  } catch (error) {
    post({
      id: request.id,
      ok: false,
      error: {
        name: error instanceof Error ? error.name : "Error",
        message: error instanceof Error ? error.message : String(error),
      },
    });
  }
}

function post(response: RpcResponse): void {
  self.postMessage(response);
}
```

Implement `dispatch()` for the first method set:

```ts
function dispatch(api, method: string, args: unknown[]) {
  switch (method) {
    case "WzFile.fromBlob":
      return serializeRef(api.WzFile.fromBlob(args[0], args[1], args[2], args[3]));
    case "WzFile.parseWzFile":
      return fileFrom(args[0]).parseWzFile();
    case "WzFile.getWzDirectory":
      return serializeRef(fileFrom(args[0]).getWzDirectory());
    case "WzObject.getName":
      return objectFrom(args[0]).getName();
    case "dispose":
      return disposeRef(args[0]);
    default:
      throw new Error(`Unsupported RPC method: ${method}`);
  }
}
```

- [ ] **Step 4: Run proxy test to confirm it still fails at client**

Run:

```bash
npm run build:wasm
node tests/wasm/browser-main.test.mjs
```

Expected: FAIL because browser client wrappers are not implemented yet, but the Worker compiles.

- [ ] **Step 5: Commit**

```bash
git add wasm/rpc.ts wasm/worker.ts
git commit -m "feat: add wasm worker RPC dispatch"
```

## Task 5: Implement Browser Main-Thread Async Proxy

**Files:**
- Modify: `package.json`
- Create: `wasm/browser-main.ts`
- Test: `tests/wasm/browser-main.test.mjs`

- [ ] **Step 1: Implement RPC client transport**

In `wasm/browser-main.ts`, replace Blob-open-only code with a generic request helper:

```ts
class RpcClient {
  private nextId = 1;
  private pending = new Map<number, { resolve(value: unknown): void; reject(error: Error): void }>();

  constructor(readonly worker: Worker) {
    worker.addEventListener("message", (event: MessageEvent<RpcResponse>) => {
      const response = event.data;
      const pending = this.pending.get(response.id);
      if (!pending) return;
      this.pending.delete(response.id);
      if (response.ok) pending.resolve(response.value);
      else pending.reject(new Error(response.error.message));
    });
  }

  call(method: string, args: unknown[] = []): Promise<unknown> {
    const id = this.nextId++;
    this.worker.postMessage({ id, method, args });
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
    });
  }
}
```

- [ ] **Step 2: Implement async wrapper classes**

Add proxy wrappers with Node-like names:

```ts
class RemoteWzObject {
  protected disposed = false;
  constructor(protected rpc: RpcClient, protected ref: RemoteRef) {}

  protected ensureLive(): void {
    if (this.disposed) throw new Error("remote object is disposed");
  }

  async getName(): Promise<string> {
    this.ensureLive();
    return this.rpc.call("WzObject.getName", [this.ref]) as Promise<string>;
  }
}

class RemoteWzFile extends RemoteWzObject {
  async parseWzFile(): Promise<number> {
    this.ensureLive();
    return this.rpc.call("WzFile.parseWzFile", [this.ref]) as Promise<number>;
  }

  async getWzDirectory(): Promise<RemoteWzDirectory> {
    this.ensureLive();
    const ref = await this.rpc.call("WzFile.getWzDirectory", [this.ref]) as RemoteRef;
    return new RemoteWzDirectory(this.rpc, ref);
  }

  async [Symbol.asyncDispose](): Promise<void> {
    if (this.disposed) return;
    this.disposed = true;
    await this.rpc.call("dispose", [this.ref]);
  }
}
```

Add the initial class set needed by tests: `RemoteWzFile`, `RemoteWzDirectory`, `RemoteWzImage`.

- [ ] **Step 3: Return a Node-shaped async API from `createWzWorker()`**

Add the browser-main export to `package.json`:

```json
"./wasm/browser-main": {
  "types": "./dist/wasm/browser-main.d.ts",
  "import": "./dist/wasm/browser-main.js"
}
```

Return:

```ts
return {
  worker,
  WzFile: {
    async fromBlob(name, blob, gameVersionOrMapleVersion, mapleVersion) {
      const ref = await rpc.call("WzFile.fromBlob", [name, blob, gameVersionOrMapleVersion, mapleVersion]) as RemoteRef;
      return new RemoteWzFile(rpc, ref);
    },
  },
  async [Symbol.asyncDispose]() {
    worker.terminate();
  },
  terminate() {
    worker.terminate();
  },
};
```

- [ ] **Step 4: Run proxy verification**

Run:

```bash
npm run build:wasm
node tests/wasm/browser-main.test.mjs
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add package.json wasm/browser-main.ts tests/wasm/browser-main.test.mjs
git commit -m "feat: add async wasm worker proxy API"
```

## Task 6: Fill Out API Parity Surface

**Files:**
- Modify: `src/node-wrapper.ts`
- Modify: `wasm/worker.ts`
- Modify: `wasm/browser-main.ts`
- Test: `tests/wasm/parity.test.mjs`

- [ ] **Step 1: Write parity test**

Create `tests/wasm/parity.test.mjs`:

```js
import assert from "node:assert/strict";
import fs from "node:fs";
import test from "node:test";
import path from "node:path";
import { fileURLToPath } from "node:url";
import * as nativeWz from "../../index.js";
import { createWzApi } from "../../dist/wasm/index.js";
import { createWzWorker } from "../../dist/wasm/browser-main.js";

const repo = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))));
const sample = path.join(repo, "Harepacker-resurrected", "UnitTest_WzFile", "WzFiles", "Character.wz");

function readNativeSummary() {
  using file = new nativeWz.WzFile(sample, nativeWz.MapleVersion.GMS);
  const status = file.parseWzFile();
  const root = file.getWzDirectory();
  return { status, name: root.getName(), images: root.getImages().length };
}

test("wasm direct and proxy match native summary", async () => {
  const expected = readNativeSummary();

  const wasmNode = await createWzApi();
  using wasmFile = new wasmNode.WzFile(sample, wasmNode.MapleVersion.GMS);
  assert.deepEqual({
    status: wasmFile.parseWzFile(),
    name: wasmFile.getWzDirectory().getName(),
    images: wasmFile.getWzDirectory().getImages().length,
  }, expected);

  await using proxy = await createWzWorker();
  await using proxyFile = await proxy.WzFile.fromBlob("Character.wz", new Blob([fs.readFileSync(sample)]), proxy.MapleVersion.GMS);
  assert.deepEqual({
    status: await proxyFile.parseWzFile(),
    name: await (await proxyFile.getWzDirectory()).getName(),
    images: (await (await proxyFile.getWzDirectory()).getImages()).length,
  }, expected);
});
```

Run:

```bash
npm run build:wasm
node tests/wasm/parity.test.mjs
```

Expected: FAIL until directory image-list RPC methods are implemented.

- [ ] **Step 2: Add method groups in batches**

Implement RPC and async wrappers for these groups:

```ts
// WzDirectory
getImages(): Promise<RemoteWzImage[]>;
getDirectories(): Promise<RemoteWzDirectory[]>;
getImage(index: number): Promise<RemoteWzImage | null>;
getImageByName(name: string): Promise<RemoteWzImage | null>;

// WzImage
parseImage(): Promise<void>;
getProperties(): Promise<RemoteWzImageProperty[]>;
getFromPath(path: string): Promise<RemoteWzImageProperty | null>;

// WzImageProperty scalar getters
getType(): Promise<number>;
getName(): Promise<string>;
getValue(): Promise<number | bigint | string | Uint8Array>;
```

After each group, run:

```bash
npm run build:wasm
node tests/wasm/parity.test.mjs
```

Expected: progress from missing-method failures to PASS.

- [ ] **Step 3: Verify disposal invalidation**

Extend `tests/wasm/parity.test.mjs`:

```js
test("async disposed proxy rejects later calls", async () => {
  await using proxy = await createWzWorker();
  const file = await proxy.WzFile.fromBlob("Character.wz", new Blob([fs.readFileSync(sample)]), proxy.MapleVersion.GMS);
  await file[Symbol.asyncDispose]();
  await assert.rejects(() => file.getName(), /disposed/i);
});
```

Run:

```bash
npm run build:wasm
node tests/wasm/parity.test.mjs
```

Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add src/node-wrapper.ts wasm/worker.ts wasm/browser-main.ts tests/wasm/parity.test.mjs
git commit -m "feat: expand wasm API parity surface"
```

## Task 7: Bundler Fixture Verification

**Files:**
- Create: `tests/wasm/vite/package.json`
- Create: `tests/wasm/vite/vite.config.ts`
- Create: `tests/wasm/vite/src/main.ts`
- Create: `tests/wasm/webpack/package.json`
- Create: `tests/wasm/webpack/webpack.config.mjs`
- Create: `tests/wasm/webpack/src/index.mjs`
- Modify: `package.json`

- [ ] **Step 1: Add Vite fixture**

Create `tests/wasm/vite/src/main.ts`:

```ts
import { createWzWorker, MapleVersion } from "libwz/wasm/browser-main";

export async function smoke(blob: Blob) {
  await using wz = await createWzWorker();
  await using file = await wz.WzFile.fromBlob("fixture.wz", blob, MapleVersion.GMS);
  return file.parseWzFile();
}
```

Create `tests/wasm/vite/package.json`:

```json
{
  "type": "module",
  "scripts": {
    "build": "vite build"
  },
  "dependencies": {
    "libwz": "file:../../..",
    "vite": "^7.0.0"
  },
  "devDependencies": {}
}
```

Create `tests/wasm/vite/vite.config.ts`:

```ts
import { defineConfig } from "vite";

export default defineConfig({
  build: {
    lib: {
      entry: "src/main.ts",
      formats: ["es"],
      fileName: "main",
    },
  },
});
```

- [ ] **Step 2: Add Webpack fixture**

Create `tests/wasm/webpack/src/index.mjs`:

```js
import { createWzWorker, MapleVersion } from "libwz/wasm/browser-main";

export async function smoke(blob) {
  await using wz = await createWzWorker();
  await using file = await wz.WzFile.fromBlob("fixture.wz", blob, MapleVersion.GMS);
  return file.parseWzFile();
}
```

Create `tests/wasm/webpack/package.json`:

```json
{
  "type": "module",
  "scripts": {
    "build": "webpack --config webpack.config.mjs"
  },
  "dependencies": {
    "libwz": "file:../../..",
    "webpack": "^5.100.0",
    "webpack-cli": "^6.0.0"
  }
}
```

Create `tests/wasm/webpack/webpack.config.mjs`:

```js
export default {
  mode: "production",
  entry: "./src/index.mjs",
  experiments: {
    asyncWebAssembly: true,
  },
  output: {
    module: true,
    library: { type: "module" },
  },
};
```

- [ ] **Step 3: Add root script**

Add to `package.json`:

```json
"test:wasm:bundlers": "npm --prefix tests/wasm/vite install && npm --prefix tests/wasm/vite run build && npm --prefix tests/wasm/webpack install && npm --prefix tests/wasm/webpack run build"
```

- [ ] **Step 4: Run bundler verification**

Run:

```bash
npm run build:wasm
npm run test:wasm:bundlers
```

Expected: PASS. No fixture uses `require`, UMD, or manual Wasm copying.

- [ ] **Step 5: Commit**

```bash
git add package.json tests/wasm/vite tests/wasm/webpack
git commit -m "test: verify wasm ESM bundler integration"
```

## Task 8: Final Verification and Docs

**Files:**
- Modify: `docs/superpowers/progress/2026-06-22-wasm-blob-data-source-progress.md`
- Modify: `README.md`

- [ ] **Step 1: Run complete verification**

Run:

```bash
npm test
npm run test:wasm
node tests/wasm/direct.test.mjs
node tests/wasm/browser-main.test.mjs
node tests/wasm/parity.test.mjs
npm pack --dry-run
```

When any C++ files are changed during implementation, also run:

```bash
clang-format -i include/wz/*.h include/wz/**/*.h src/*.cpp src/**/*.cpp tests/*.cpp
cpplint --recursive --config .cpplint src include tests
cmake -B build -DBUILD_TESTS=ON -DBUILD_JNI=OFF
cmake --build build --target wz_test -j2
ctest --test-dir build --output-on-failure
```

Expected: all commands pass.

- [ ] **Step 2: Update progress doc**

Append a section describing:

```md
### Isomorphic Wasm API

- Added shared synchronous wrapper factory.
- Added direct Wasm API through `libwz/wasm` for Node.js and browser Workers.
- Added browser main-thread async Worker proxy through `libwz/wasm/browser-main`.
- Added `Symbol.asyncDispose` support to the async proxy.
- Verified parity against native Node APIs.
```

- [ ] **Step 3: Commit docs**

```bash
git add docs/superpowers/progress/2026-06-22-wasm-blob-data-source-progress.md README.md
git commit -m "docs: record isomorphic wasm API progress"
```

## Final Review Checklist

- [ ] Root `libwz` API remains synchronous and native-addon backed.
- [ ] Node.js Wasm users can use `libwz/wasm` without Worker proxy.
- [ ] Browser Worker users can use `libwz/wasm` without a nested Worker.
- [ ] Browser main-thread users can use `libwz/wasm/browser-main` async proxy.
- [ ] Async proxy owning objects support `Symbol.asyncDispose`.
- [ ] Direct owning objects support `Symbol.dispose`.
- [ ] Blob-backed reads do not call full `blob.arrayBuffer()`.
- [ ] Raw native handles are not public browser API.
- [ ] `npm pack --dry-run` includes all `dist/wasm` files needed by exports.
