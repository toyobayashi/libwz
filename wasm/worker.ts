import { loadWzModule } from "./loader.js";

type OpenBlobMessage = {
  type: "openBlob";
  id: number;
  blob: Blob;
  name: string;
  gameVersion: number;
  mapleVersion: number;
};

type CloseBlobMessage = {
  type: "closeBlob";
  id: number;
};

type WzWorkerMessage = OpenBlobMessage | CloseBlobMessage;

type WzBinding = {
  openBlobSource(
    id: number,
    size: number,
    name: string,
    gameVersion: number,
    mapleVersion: number,
  ): bigint | null;
};

const blobs = new Map<number, Blob>();
const reader = new FileReaderSync();

self.addEventListener("message", (event: MessageEvent<WzWorkerMessage>) => {
  void handleMessage(event.data).catch((error: unknown) => {
    const message = error instanceof Error ? error.message : String(error);
    self.postMessage({ type: "error", id: event.data.id, message });
  });
});

async function handleMessage(message: WzWorkerMessage): Promise<void> {
  if (message.type === "closeBlob") {
    blobs.delete(message.id);
    return;
  }

  blobs.set(message.id, message.blob);

  const { binding, module } = await loadWzModule();
  globalThis.__libwzReadBlobRange = (
    id: number,
    offset: number,
    destination: number,
    length: number,
  ): number => {
    const source = blobs.get(id);
    if (!source) return -1;
    const start = Math.trunc(offset);
    if (start < 0 || start > source.size) return -1;
    const end = Math.min(start + length, source.size);
    const range = source.slice(start, end);
    const bytes = new Uint8Array(reader.readAsArrayBuffer(range));
    module.HEAPU8.set(bytes, destination);
    return bytes.byteLength;
  };

  const file = (binding as WzBinding).openBlobSource(
    message.id,
    message.blob.size,
    message.name,
    message.gameVersion,
    message.mapleVersion,
  );
  self.postMessage({ type: "opened", id: message.id, file });
}

declare global {
  class FileReaderSync {
    readAsArrayBuffer(blob: Blob): ArrayBuffer;
  }

  var __libwzReadBlobRange:
    | ((
        id: number,
        offset: number,
        destination: number,
        length: number,
      ) => number)
    | undefined;
}
