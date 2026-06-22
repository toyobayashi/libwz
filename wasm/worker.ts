import { createWzApi } from "./index.js";
import type { RemoteRef, RemoteRefKind, RpcRequest, RpcResponse } from "./rpc.js";
import type { BlobLike, MapleVersionValue, WzApi, WzFile } from "./node-wrapper.js";

type WorkerScope = {
  addEventListener(
    type: "message",
    listener: (event: MessageEvent<unknown>) => void,
  ): void;
  postMessage(message: RpcResponse): void;
};

type RegistryEntry = {
  kind: RemoteRefKind;
  object: RemoteObject;
  type?: string;
  ownerFile?: string;
};

type RemoteObject = {
  getName?: () => string;
  close?: () => void;
  [Symbol.dispose]?: () => void;
};

const workerScope = self as unknown as WorkerScope;
const registry = new Map<string, RegistryEntry>();
let nextHandle = 1;
let apiPromise: Promise<WzApi> | undefined;
let configuredWasmUrl: string | undefined;

workerScope.addEventListener("message", (event: MessageEvent<unknown>) => {
  const request = event.data;
  if (!isRpcRequest(request)) {
    workerScope.postMessage({
      id: rpcIdFrom(request),
      ok: false,
      error: {
        name: "TypeError",
        message: "Invalid RPC request",
      },
    });
    return;
  }

  void handleRequest(request);
});

async function handleRequest(request: RpcRequest): Promise<void> {
  try {
    if (!isSupportedMethod(request.method)) {
      throw new Error(`Unsupported RPC method: ${request.method}`);
    }
    if (request.method === "configure") {
      configure(request.args);
      workerScope.postMessage({ id: request.id, ok: true, value: undefined });
      return;
    }
    const api = await getApi();
    const value = dispatch(api, request.method, request.args);
    workerScope.postMessage({ id: request.id, ok: true, value });
  } catch (error) {
    workerScope.postMessage({
      id: request.id,
      ok: false,
      error: errorToWire(error),
    });
  }
}

function getApi(): Promise<WzApi> {
  apiPromise ??= configuredWasmUrl === undefined
    ? createWzApi()
    : createWzApi({ wasmUrl: configuredWasmUrl });
  return apiPromise;
}

function isSupportedMethod(method: string): boolean {
  switch (method) {
    case "configure":
    case "WzFile.fromBlob":
    case "WzFile.parseWzFile":
    case "WzFile.getWzDirectory":
    case "WzObject.getName":
    case "dispose":
      return true;
    default:
      return false;
  }
}

function configure(args: unknown[]): void {
  if (apiPromise !== undefined) {
    throw new Error("Cannot configure libwz worker after wasm initialization");
  }
  if (args.length !== 1 || !isWorkerConfig(args[0])) {
    throw new TypeError("configure requires a wasmUrl string");
  }
  configuredWasmUrl = args[0].wasmUrl;
}

function dispatch(api: WzApi, method: string, args: unknown[]): unknown {
  switch (method) {
    case "WzFile.fromBlob":
      return fromBlob(api, args);
    case "WzFile.parseWzFile":
      return fileFrom(args[0]).parseWzFile();
    case "WzFile.getWzDirectory":
      return getWzDirectory(args[0]);
    case "WzObject.getName":
      return objectFrom(args[0]).getName();
    case "dispose":
      disposeRef(args[0]);
      return undefined;
    default:
      throw new Error(`Unsupported RPC method: ${method}`);
  }
}

function fromBlob(api: WzApi, args: unknown[]): RemoteRef {
  const [name, blob, gameVersionOrMapleVersion, mapleVersion] = args;
  if (typeof name !== "string") {
    throw new TypeError("WzFile.fromBlob requires a string name");
  }
  if (!isBlobLike(blob)) {
    throw new TypeError("WzFile.fromBlob requires a Blob-like source");
  }
  if (typeof gameVersionOrMapleVersion !== "number") {
    throw new TypeError("WzFile.fromBlob requires a numeric game or Maple version");
  }
  if (mapleVersion !== undefined && typeof mapleVersion !== "number") {
    throw new TypeError("WzFile.fromBlob requires a numeric Maple version");
  }

  const file = mapleVersion === undefined
    ? api.WzFile.fromBlob(name, blob, gameVersionOrMapleVersion as MapleVersionValue)
    : api.WzFile.fromBlob(
      name,
      blob,
      gameVersionOrMapleVersion,
      mapleVersion as MapleVersionValue,
    );
  return registerRef("file", file, { type: "WzFile" });
}

function getWzDirectory(value: unknown): RemoteRef | null {
  const fileRef = requireRemoteRef(value, "file");
  const file = fileFromRef(fileRef);
  const directory = file.getWzDirectory();
  if (directory === null) return null;
  return registerRef("directory", directory, {
    type: "WzDirectory",
    ownerFile: fileRef.handle,
  });
}

function fileFrom(value: unknown): WzFile {
  return fileFromRef(requireRemoteRef(value, "file"));
}

function fileFromRef(ref: RemoteRef): WzFile {
  return entryFrom(ref, "file").object as unknown as WzFile;
}

function objectFrom(value: unknown): RemoteObject & { getName(): string } {
  const object = entryFrom(requireRemoteRef(value)).object;
  if (typeof object.getName !== "function") {
    throw new TypeError("Remote object does not support getName");
  }
  return object as RemoteObject & { getName(): string };
}

function entryFrom(ref: RemoteRef, expectedKind?: RemoteRefKind): RegistryEntry {
  if (expectedKind !== undefined && ref.kind !== expectedKind) {
    throw new TypeError(`Expected remote ${expectedKind} ref, got ${ref.kind}`);
  }

  const entry = registry.get(ref.handle);
  if (entry === undefined) {
    throw new Error(`Unknown remote ${ref.kind} handle: ${ref.handle}`);
  }
  if (entry.kind !== ref.kind) {
    throw new TypeError(
      `Remote handle ${ref.handle} is ${entry.kind}, not ${ref.kind}`,
    );
  }
  return entry;
}

function registerRef(
  kind: RemoteRefKind,
  object: RemoteObject,
  metadata: { type?: string; ownerFile?: string } = {},
): RemoteRef {
  const handle = String(nextHandle++);
  registry.set(handle, { kind, object, ...metadata });
  return { kind, handle, ...metadata };
}

function disposeRef(value: unknown): void {
  const ref = requireRemoteRef(value);
  const entry = entryFrom(ref);
  try {
    entry.object.close?.();
    entry.object[Symbol.dispose]?.();
  } finally {
    registry.delete(ref.handle);
    if (entry.kind === "file") {
      for (const [handle, child] of registry) {
        if (child.ownerFile === ref.handle) registry.delete(handle);
      }
    }
  }
}

function requireRemoteRef(value: unknown, expectedKind?: RemoteRefKind): RemoteRef {
  if (!isRemoteRef(value)) {
    throw new TypeError("Expected remote ref with string kind and handle");
  }
  if (expectedKind !== undefined && value.kind !== expectedKind) {
    throw new TypeError(`Expected remote ${expectedKind} ref, got ${value.kind}`);
  }
  return value;
}

function isRpcRequest(value: unknown): value is RpcRequest {
  if (value === null || typeof value !== "object") return false;
  const request = value as Partial<RpcRequest>;
  return isRpcId(request.id) &&
    typeof request.method === "string" &&
    Array.isArray(request.args);
}

function rpcIdFrom(value: unknown): number | string {
  if (value !== null && typeof value === "object") {
    const request = value as Partial<RpcRequest>;
    if (isRpcId(request.id)) return request.id;
  }
  return 0;
}

function isRpcId(value: unknown): value is number | string {
  return typeof value === "number" || typeof value === "string";
}

function isRemoteRef(value: unknown): value is RemoteRef {
  if (value === null || typeof value !== "object") return false;
  const ref = value as Partial<RemoteRef>;
  return typeof ref.kind === "string" && typeof ref.handle === "string";
}

function isWorkerConfig(value: unknown): value is { wasmUrl: string } {
  if (value === null || typeof value !== "object") return false;
  return typeof (value as { wasmUrl?: unknown }).wasmUrl === "string";
}

function isBlobLike(value: unknown): value is BlobLike {
  if (value === null || typeof value !== "object") return false;
  const blob = value as Partial<BlobLike>;
  return typeof blob.size === "number" && typeof blob.slice === "function";
}

function errorToWire(error: unknown): { name: string; message: string } {
  if (error instanceof Error) {
    return { name: error.name, message: error.message };
  }
  return { name: "Error", message: String(error) };
}
