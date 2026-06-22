import {
  BinaryType,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
  type MapleVersionValue,
} from "./node-wrapper.js";
import type { RemoteRef, RemoteRefKind, RpcRequest, RpcResponse } from "./rpc.js";

export {
  BinaryType,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
};

export interface WzWorkerOptions {
  wasmUrl?: string | URL;
  workerUrl?: string | URL;
  name?: string;
  workerFactory?: (url: string | URL) => Worker;
  onError?: (error: Error) => void;
}

export interface WzWorkerClient {
  readonly worker: Worker;
  readonly WzFile: RemoteWzFileConstructor;
  readonly MapleVersion: typeof MapleVersion;
  readonly ParseStatus: typeof ParseStatus;
  readonly PropertyType: typeof PropertyType;
  readonly ObjectType: typeof ObjectType;
  readonly BinaryType: typeof BinaryType;
  terminate(): void;
  [Symbol.asyncDispose](): Promise<void>;
}

export interface RemoteWzFileConstructor {
  fromBlob(
    name: string,
    blob: Blob,
    mapleVersion: MapleVersionValue,
  ): Promise<RemoteWzFile>;
  fromBlob(
    name: string,
    blob: Blob,
    gameVersion: number,
    mapleVersion: MapleVersionValue,
  ): Promise<RemoteWzFile>;
}

type PendingRpc = {
  resolve: (value: unknown) => void;
  reject: (error: Error) => void;
};

type MaybePromise = {
  then?: unknown;
};

type RemotePropertyTypeValue = number;

export function createWzWorker(options: WzWorkerOptions = {}): WzWorkerClient {
  const workerUrl = options.workerUrl ?? new URL("./worker.js", import.meta.url);
  const worker = options.workerFactory === undefined
    ? createDefaultWorker(workerUrl, options.name)
    : options.workerFactory(workerUrl);
  const rpc = new RpcClient(worker, options.onError);
  if (options.wasmUrl !== undefined) {
    rpc.configure(options.wasmUrl);
  }
  const WzFile = createRemoteWzFileConstructor(rpc);

  return {
    worker,
    WzFile,
    MapleVersion,
    ParseStatus,
    PropertyType,
    ObjectType,
    BinaryType,
    terminate() {
      rpc.terminate();
    },
    [Symbol.asyncDispose]() {
      return rpc.asyncDispose();
    },
  };
}

export class RemoteWzObject {
  protected disposed = false;

  constructor(
    protected readonly rpc: RpcClient,
    protected readonly ref: RemoteRef,
    protected readonly disposeTracker?: Set<RemoteWzObject>,
  ) {}

  async getName(): Promise<string> {
    this.assertAlive();
    return this.rpc.call("WzObject.getName", [this.ref]);
  }

  protected assertAlive(): void {
    if (this.disposed) {
      throw new Error(`Remote ${this.ref.kind} wrapper is disposed`);
    }
  }

  protected trackChild<T extends RemoteWzObject>(child: T): T {
    this.disposeTracker?.add(child);
    return child;
  }
}

export class RemoteWzDirectory extends RemoteWzObject {
  async getImages(): Promise<RemoteWzImage[]> {
    this.assertAlive();
    const value = await this.rpc.call("WzDirectory.getImages", [this.ref]);
    return this.wrapImageArray(value, "WzDirectory.getImages");
  }

  wzImages(): Promise<RemoteWzImage[]> {
    return this.getImages();
  }

  async getDirectories(): Promise<RemoteWzDirectory[]> {
    this.assertAlive();
    const value = await this.rpc.call("WzDirectory.getDirectories", [this.ref]);
    return this.wrapDirectoryArray(value, "WzDirectory.getDirectories");
  }

  wzDirectories(): Promise<RemoteWzDirectory[]> {
    return this.getDirectories();
  }

  async getImage(index: number): Promise<RemoteWzImage | null> {
    this.assertAlive();
    const value = await this.rpc.call("WzDirectory.getImage", [this.ref, index]);
    if (value === null) return null;
    if (!isRemoteRef(value, "image")) {
      throw new TypeError("WzDirectory.getImage returned an invalid remote ref");
    }
    return this.trackChild(new RemoteWzImage(this.rpc, value, this.disposeTracker));
  }

  async getImageByName(name: string): Promise<RemoteWzImage | null> {
    this.assertAlive();
    const value = await this.rpc.call("WzDirectory.getImageByName", [this.ref, name]);
    if (value === null) return null;
    if (!isRemoteRef(value, "image")) {
      throw new TypeError("WzDirectory.getImageByName returned an invalid remote ref");
    }
    return this.trackChild(new RemoteWzImage(this.rpc, value, this.disposeTracker));
  }

  async getDirectory(index: number): Promise<RemoteWzDirectory | null> {
    this.assertAlive();
    const value = await this.rpc.call("WzDirectory.getDirectory", [this.ref, index]);
    if (value === null) return null;
    if (!isRemoteRef(value, "directory")) {
      throw new TypeError("WzDirectory.getDirectory returned an invalid remote ref");
    }
    return this.trackChild(new RemoteWzDirectory(this.rpc, value, this.disposeTracker));
  }

  async getDirectoryByName(name: string): Promise<RemoteWzDirectory | null> {
    this.assertAlive();
    const value = await this.rpc.call("WzDirectory.getDirectoryByName", [this.ref, name]);
    if (value === null) return null;
    if (!isRemoteRef(value, "directory")) {
      throw new TypeError("WzDirectory.getDirectoryByName returned an invalid remote ref");
    }
    return this.trackChild(new RemoteWzDirectory(this.rpc, value, this.disposeTracker));
  }

  private wrapImageArray(value: unknown, method: string): RemoteWzImage[] {
    if (!Array.isArray(value)) {
      throw new TypeError(`${method} returned an invalid remote ref array`);
    }
    return value.map((item) => {
      if (!isRemoteRef(item, "image")) {
        throw new TypeError(`${method} returned an invalid remote ref`);
      }
      return this.trackChild(new RemoteWzImage(this.rpc, item, this.disposeTracker));
    });
  }

  private wrapDirectoryArray(value: unknown, method: string): RemoteWzDirectory[] {
    if (!Array.isArray(value)) {
      throw new TypeError(`${method} returned an invalid remote ref array`);
    }
    return value.map((item) => {
      if (!isRemoteRef(item, "directory")) {
        throw new TypeError(`${method} returned an invalid remote ref`);
      }
      return this.trackChild(new RemoteWzDirectory(this.rpc, item, this.disposeTracker));
    });
  }
}

export class RemoteWzImage extends RemoteWzObject {
  async parseImage(): Promise<void> {
    this.assertAlive();
    await this.rpc.call("WzImage.parseImage", [this.ref]);
  }

  async getProperties(): Promise<RemoteWzImageProperty[]> {
    this.assertAlive();
    const value = await this.rpc.call("WzImage.getProperties", [this.ref]);
    return this.wrapPropertyArray(value, "WzImage.getProperties");
  }

  wzProperties(): Promise<RemoteWzImageProperty[]> {
    return this.getProperties();
  }

  async getFromPath(path: string): Promise<RemoteWzImageProperty | null> {
    this.assertAlive();
    const value = await this.rpc.call("WzImage.getFromPath", [this.ref, path]);
    if (value === null) return null;
    if (!isRemoteRef(value, "property")) {
      throw new TypeError("WzImage.getFromPath returned an invalid remote ref");
    }
    return this.trackChild(
      new RemoteWzImageProperty(this.rpc, value, this.disposeTracker),
    );
  }

  private wrapPropertyArray(value: unknown, method: string): RemoteWzImageProperty[] {
    if (!Array.isArray(value)) {
      throw new TypeError(`${method} returned an invalid remote ref array`);
    }
    return value.map((item) => {
      if (!isRemoteRef(item, "property")) {
        throw new TypeError(`${method} returned an invalid remote ref`);
      }
      return this.trackChild(
        new RemoteWzImageProperty(this.rpc, item, this.disposeTracker),
      );
    });
  }
}

export class RemoteWzImageProperty extends RemoteWzObject {
  async getType(): Promise<RemotePropertyTypeValue> {
    this.assertAlive();
    return this.rpc.call("WzImageProperty.getType", [this.ref]);
  }

  getPropertyType(): Promise<RemotePropertyTypeValue> {
    return this.getType();
  }

  async getValue(): Promise<unknown> {
    this.assertAlive();
    return this.rpc.call("WzImageProperty.getValue", [this.ref]);
  }
}

export class RemoteWzFile extends RemoteWzObject {
  private readonly children = new Set<RemoteWzObject>();

  async parseWzFile(): Promise<number> {
    this.assertAlive();
    return this.rpc.call("WzFile.parseWzFile", [this.ref]);
  }

  async getWzDirectory(): Promise<RemoteWzDirectory | null> {
    this.assertAlive();
    const value = await this.rpc.call("WzFile.getWzDirectory", [this.ref]);
    if (value === null) return null;
    if (!isRemoteRef(value, "directory")) {
      throw new TypeError("WzFile.getWzDirectory returned an invalid remote ref");
    }
    const directory = new RemoteWzDirectory(this.rpc, value, this.children);
    this.children.add(directory);
    return directory;
  }

  async [Symbol.asyncDispose](): Promise<void> {
    if (this.disposed) return;
    markRemoteDisposed(this);
    for (const child of this.children) markRemoteDisposed(child);
    this.children.clear();

    if (this.rpc.isDisposed()) return;
    try {
      await this.rpc.call("dispose", [this.ref]);
    } catch (error) {
      if (!this.rpc.isDisposed()) throw error;
    }
  }
}

class RpcClient {
  private nextId = 1;
  private readonly pending = new Map<number | string, PendingRpc>();
  private ready: Promise<void> = Promise.resolve();
  private disposed = false;
  private disposeReason: Error | undefined;
  private readonly messageListener: (event: MessageEvent<unknown>) => void;
  private readonly errorListener: (event: Event) => void;
  private readonly messageErrorListener: (event: Event) => void;

  constructor(
    readonly worker: Worker,
    private readonly onError: ((error: Error) => void) | undefined,
  ) {
    this.messageListener = (event: MessageEvent<unknown>) => {
      this.handleMessage(event.data);
    };
    this.errorListener = (event: Event) => {
      this.fail(errorFromEvent(event), true);
    };
    this.messageErrorListener = (event: Event) => {
      this.fail(errorFromEvent(event), true);
    };

    worker.addEventListener("message", this.messageListener);
    worker.addEventListener("error", this.errorListener);
    worker.addEventListener("messageerror", this.messageErrorListener);
  }

  configure(wasmUrl: string | URL): void {
    const ready = this.callRaw<void>("configure", [{ wasmUrl: wasmUrl.toString() }])
      .then(() => undefined);
    ready.catch((error: unknown) => {
      this.fail(errorFromUnknown(error), true);
    });
    this.ready = ready;
  }

  call<T>(method: string, args: unknown[]): Promise<T> {
    return this.ready.then(() => this.callRaw<T>(method, args));
  }

  private callRaw<T>(method: string, args: unknown[]): Promise<T> {
    if (this.disposed) {
      return Promise.reject(
        this.disposeReason ?? new Error("libwz worker client is disposed"),
      );
    }

    const id = this.nextId++;
    const request: RpcRequest = { id, method, args };
    return new Promise<T>((resolve, reject) => {
      this.pending.set(id, {
        resolve: resolve as (value: unknown) => void,
        reject,
      });
      try {
        this.worker.postMessage(request);
      } catch (error) {
        this.pending.delete(id);
        reject(errorFromUnknown(error));
      }
    });
  }

  terminate(): void {
    void this.disposeWorker(new Error("libwz worker client is disposed"));
  }

  async asyncDispose(): Promise<void> {
    await this.disposeWorker(new Error("libwz worker client is disposed"));
  }

  isDisposed(): boolean {
    return this.disposed;
  }

  private handleMessage(message: unknown): void {
    if (!isRpcResponseEnvelope(message)) return;
    const pending = this.pending.get(message.id);
    if (pending === undefined) return;
    this.pending.delete(message.id);

    if (!isRpcResponse(message)) {
      pending.reject(new Error("Malformed RPC error response from libwz worker"));
    } else if (message.ok) {
      pending.resolve(message.value);
    } else {
      pending.reject(errorFromWire(message.error));
    }
  }

  private fail(error: Error, terminateWorker: boolean): void {
    if (this.disposed) return;
    this.disposed = true;
    this.disposeReason = error;
    this.notifyError(error);
    this.rejectPending(error);
    this.removeListeners();
    if (terminateWorker) {
      try {
        this.worker.terminate();
      } catch {
        // Termination is best-effort; pending calls have already been rejected.
      }
    }
  }

  private async disposeWorker(error: Error): Promise<void> {
    if (this.disposed) return;
    this.disposed = true;
    this.disposeReason = error;
    this.rejectPending(error);
    this.removeListeners();
    const result = this.worker.terminate() as unknown;
    if (isPromiseLike(result)) await result;
  }

  private removeListeners(): void {
    const worker = this.worker as Worker & {
      removeEventListener?: Worker["removeEventListener"];
    };
    if (typeof worker.removeEventListener !== "function") return;
    worker.removeEventListener("message", this.messageListener);
    worker.removeEventListener("error", this.errorListener);
    worker.removeEventListener("messageerror", this.messageErrorListener);
  }

  private rejectPending(error: Error): void {
    for (const pending of this.pending.values()) pending.reject(error);
    this.pending.clear();
  }

  private notifyError(error: Error): void {
    if (this.onError === undefined) return;
    try {
      this.onError(error);
    } catch {
      // User error callbacks should not mask the worker failure.
    }
  }
}

function markRemoteDisposed(object: RemoteWzObject): void {
  (object as unknown as { disposed: boolean }).disposed = true;
}

function createRemoteWzFileConstructor(rpc: RpcClient): RemoteWzFileConstructor {
  return {
    fromBlob(
      name: string,
      blob: Blob,
      gameVersionOrMapleVersion: number,
      mapleVersion?: MapleVersionValue,
    ): Promise<RemoteWzFile> {
      const args = mapleVersion === undefined
        ? [name, blob, gameVersionOrMapleVersion]
        : [name, blob, gameVersionOrMapleVersion, mapleVersion];
      return rpc.call("WzFile.fromBlob", args).then((value) => {
        if (!isRemoteRef(value, "file")) {
          throw new TypeError("WzFile.fromBlob returned an invalid remote ref");
        }
        return new RemoteWzFile(rpc, value);
      });
    },
  };
}

function createDefaultWorker(url: string | URL, name: string | undefined): Worker {
  const WorkerCtor = globalThis.Worker;
  if (typeof WorkerCtor !== "function") {
    throw new Error("Worker is not available; pass workerFactory to createWzWorker");
  }
  return new WorkerCtor(url, {
    name: name ?? "libwz-worker",
    type: "module",
  });
}

function isRemoteRef(value: unknown, expectedKind?: RemoteRefKind): value is RemoteRef {
  if (value === null || typeof value !== "object") return false;
  const ref = value as Partial<RemoteRef>;
  if (typeof ref.kind !== "string" || typeof ref.handle !== "string") {
    return false;
  }
  return expectedKind === undefined || ref.kind === expectedKind;
}

function isRpcResponseEnvelope(
  value: unknown,
): value is Pick<RpcResponse, "id"> {
  if (value === null || typeof value !== "object") return false;
  const response = value as Partial<RpcResponse>;
  return typeof response.id === "number" || typeof response.id === "string";
}

function isRpcResponse(value: unknown): value is RpcResponse {
  if (!isRpcResponseEnvelope(value)) return false;
  const response = value as Partial<RpcResponse>;
  if (response.ok === true) return true;
  if (response.ok !== false) return false;
  return isWireError(response.error);
}

function isWireError(value: unknown): value is { name: string; message: string } {
  if (value === null || typeof value !== "object") return false;
  const error = value as { name?: unknown; message?: unknown };
  return typeof error.name === "string" && typeof error.message === "string";
}

function errorFromWire(error: { name: string; message: string }): Error {
  const result = new Error(error.message);
  result.name = error.name;
  return result;
}

function errorFromEvent(event: Event): Error {
  if ("error" in event && event.error instanceof Error) {
    return event.error;
  }
  if ("message" in event && typeof event.message === "string") {
    return new Error(event.message);
  }
  return new Error("libwz worker failed");
}

function errorFromUnknown(error: unknown): Error {
  return error instanceof Error ? error : new Error(String(error));
}

function isPromiseLike(value: unknown): value is Promise<unknown> {
  return value !== null &&
    typeof value === "object" &&
    typeof (value as MaybePromise).then === "function";
}
