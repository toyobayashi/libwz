export interface WzWorkerOptions {
  url?: URL | string;
  name?: string;
}

export interface OpenBlobOptions {
  blob: Blob;
  name: string;
  gameVersion: number;
  mapleVersion: number;
}

export interface WzWorkerClient {
  openBlob(options: OpenBlobOptions): Promise<OpenedWzBlob>;
  closeBlob(id: number): void;
  terminate(): void;
  readonly worker: Worker;
}

export interface OpenedWzBlob {
  id: number;
  file: bigint | null;
}

type WorkerResponse =
  | { id: number | string; ok: true; value: unknown }
  | { id: number | string; ok: false; error: { name: string; message: string } };

type RemoteRef = {
  kind: string;
  handle: string;
};

export function createWzWorker(options: WzWorkerOptions = {}): WzWorkerClient {
  const url = options.url ?? new URL("./worker.js", import.meta.url);
  const worker = new Worker(url, {
    name: options.name ?? "libwz-worker",
    type: "module",
  });

  let nextId = 1;
  const pending = new Map<
    number,
    {
      resolve: (file: OpenedWzBlob) => void;
      reject: (error: Error) => void;
    }
  >();
  const openedRefs = new Map<number, RemoteRef>();

  worker.addEventListener("message", (event: MessageEvent<WorkerResponse>) => {
    const response = event.data;
    if (typeof response.id !== "number") return;
    const request = pending.get(response.id);
    if (!request) return;
    pending.delete(response.id);
    if (!response.ok) {
      request.reject(new Error(response.error.message));
      return;
    }
    if (!isRemoteRef(response.value)) {
      request.reject(new TypeError("WzFile.fromBlob returned an invalid remote ref"));
      return;
    }
    openedRefs.set(response.id, response.value);
    request.resolve({ id: response.id, file: null });
  });

  return {
    worker,
    openBlob({ blob, name, gameVersion, mapleVersion }) {
      const id = nextId++;
      return new Promise<OpenedWzBlob>((resolve, reject) => {
        pending.set(id, { resolve, reject });
        worker.postMessage({
          id,
          method: "WzFile.fromBlob",
          args: [name, blob, gameVersion, mapleVersion],
        });
      });
    },
    closeBlob(id) {
      const ref = openedRefs.get(id);
      if (ref === undefined) return;
      openedRefs.delete(id);
      worker.postMessage({
        id,
        method: "dispose",
        args: [ref],
      });
    },
    terminate() {
      worker.terminate();
      for (const request of pending.values()) {
        request.reject(new Error("libwz worker was terminated"));
      }
      pending.clear();
      openedRefs.clear();
    },
  };
}

function isRemoteRef(value: unknown): value is RemoteRef {
  if (value === null || typeof value !== "object") return false;
  const ref = value as Partial<RemoteRef>;
  return typeof ref.kind === "string" && typeof ref.handle === "string";
}
