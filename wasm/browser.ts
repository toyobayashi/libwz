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
  | { type: "opened"; id: number; file: bigint | null }
  | { type: "error"; id: number; message: string };

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

  worker.addEventListener("message", (event: MessageEvent<WorkerResponse>) => {
    const response = event.data;
    const request = pending.get(response.id);
    if (!request) return;
    pending.delete(response.id);
    if (response.type === "error") {
      request.reject(new Error(response.message));
    } else {
      request.resolve({ id: response.id, file: response.file });
    }
  });

  return {
    worker,
    openBlob({ blob, name, gameVersion, mapleVersion }) {
      const id = nextId++;
      return new Promise<OpenedWzBlob>((resolve, reject) => {
        pending.set(id, { resolve, reject });
        worker.postMessage({
          type: "openBlob",
          id,
          blob,
          name,
          gameVersion,
          mapleVersion,
        });
      });
    },
    closeBlob(id) {
      worker.postMessage({ type: "closeBlob", id });
    },
    terminate() {
      worker.terminate();
      for (const request of pending.values()) {
        request.reject(new Error("libwz worker was terminated"));
      }
      pending.clear();
    },
  };
}
