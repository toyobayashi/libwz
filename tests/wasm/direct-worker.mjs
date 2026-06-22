import { parentPort, workerData } from "node:worker_threads";
import { createWzApi, MapleVersion } from "../../dist/wasm/index.js";

class NodeFileReaderSync {
  readAsArrayBuffer(blob) {
    if (!blob.__libwzBytes) {
      throw new Error("test Blob range source is missing bytes");
    }
    const start = blob.__libwzStart ?? 0;
    const end = blob.__libwzEnd ?? blob.__libwzBytes.byteLength;
    return blob.__libwzBytes.buffer.slice(
      blob.__libwzBytes.byteOffset + start,
      blob.__libwzBytes.byteOffset + end,
    );
  }
}

class NodeRangeBlob extends Blob {
  constructor(bytes, start = 0, end = bytes.byteLength) {
    super([]);
    this.__libwzBytes = bytes;
    this.__libwzStart = start;
    this.__libwzEnd = end;
  }

  get size() {
    return this.__libwzEnd - this.__libwzStart;
  }

  slice(start = 0, end = this.size) {
    const nextStart = this.__libwzStart + Math.trunc(start);
    const nextEnd = this.__libwzStart + Math.min(Math.trunc(end), this.size);
    return new NodeRangeBlob(this.__libwzBytes, nextStart, nextEnd);
  }

  arrayBuffer() {
    throw new Error("full Blob read is forbidden");
  }
}

globalThis.FileReaderSync = NodeFileReaderSync;

try {
  const bytes = new Uint8Array(workerData.bytes);
  const blob = new NodeRangeBlob(bytes);
  const wz = await createWzApi();
  using file = wz.WzFile.fromBlob("TamingMob_GMS_87.wz", blob, MapleVersion.GMS);
  const status = file.parseWzFile();
  const root = file.getWzDirectory();
  parentPort.postMessage({ ok: true, status, name: root?.getName() });
} catch (error) {
  parentPort.postMessage({
    ok: false,
    message: error instanceof Error ? error.message : String(error),
  });
}
