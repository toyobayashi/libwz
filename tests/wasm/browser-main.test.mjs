import assert from "node:assert/strict";
import fs from "node:fs";
import test from "node:test";
import path from "node:path";
import { Worker } from "node:worker_threads";
import { fileURLToPath } from "node:url";
import {
  createWzWorker,
  MapleVersion,
  ParseStatus,
} from "../../dist/wasm/browser-main.js";

const root = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))));
const sample = path.join(
  root,
  "Harepacker-resurrected",
  "UnitTest_WzFile",
  "WzFiles",
  "Common",
  "TamingMob_GMS_87.wz",
);

function createNodeBrowserWorker(workerUrl = new URL("../../dist/wasm/worker.js", import.meta.url)) {
  const nodeHarness = `
    import { parentPort } from "node:worker_threads";

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
    const messageListeners = [];
    globalThis.self = {
      addEventListener(type, listener) {
        if (type === "message") messageListeners.push(listener);
      },
      postMessage(message) {
        parentPort.postMessage(message);
      }
    };
    parentPort.on("message", async (data) => {
      if (data?.method === "WzFile.fromBlob" && data.args?.[1] instanceof Blob) {
        const bytes = new Uint8Array(await data.args[1].arrayBuffer());
        data = { ...data, args: [...data.args] };
        data.args[1] = new NodeRangeBlob(bytes);
      }
      for (const listener of messageListeners) listener({ data });
    });
    await import(${JSON.stringify(workerUrl.href)});
  `;
  const worker = new Worker(
    new URL(`data:text/javascript,${encodeURIComponent(nodeHarness)}`),
    { type: "module" },
  );
  return {
    addEventListener(type, listener) {
      if (type === "message") worker.on("message", (data) => listener({ data }));
      if (type === "error") worker.on("error", listener);
    },
    postMessage(message) {
      worker.postMessage(message);
    },
    terminate() {
      void worker.terminate();
    },
  };
}

function createTrackingWorker() {
  const listeners = new Map();
  const worker = {
    terminated: 0,
    messages: [],
    addEventListener(type, listener) {
      const bucket = listeners.get(type) ?? new Set();
      bucket.add(listener);
      listeners.set(type, bucket);
    },
    removeEventListener(type, listener) {
      listeners.get(type)?.delete(listener);
    },
    postMessage(message) {
      this.messages.push(message);
    },
    terminate() {
      this.terminated += 1;
    },
    dispatch(type, data) {
      for (const listener of listeners.get(type) ?? []) listener({ data });
    },
    listenerCount(type) {
      return listeners.get(type)?.size ?? 0;
    },
  };
  return worker;
}

function createSilentWorker() {
  return {
    terminated: 0,
    messages: [],
    addEventListener() {},
    postMessage(message) {
      this.messages.push(message);
    },
    terminate() {
      this.terminated += 1;
    },
  };
}

test("browser main-thread proxy passes wasmUrl through to the Worker", async () => {
  const bytes = fs.readFileSync(sample);
  const wasmUrl = new URL("../../dist/wasm/libwz.wasm", import.meta.url);
  await using wz = await createWzWorker({
    wasmUrl,
    workerFactory: createNodeBrowserWorker,
  });
  await using file = await wz.WzFile.fromBlob(
    "TamingMob_GMS_87.wz",
    new Blob([bytes]),
    MapleVersion.GMS,
  );

  assert.equal(await file.parseWzFile(), ParseStatus.SUCCESS);
});

test("browser main-thread proxy parses Blob WZ files through a Worker", async () => {
  const bytes = fs.readFileSync(sample);
  await using wz = await createWzWorker({ workerFactory: createNodeBrowserWorker });
  await using file = await wz.WzFile.fromBlob(
    "TamingMob_GMS_87.wz",
    new Blob([bytes]),
    MapleVersion.GMS,
  );

  assert.equal(await file.parseWzFile(), ParseStatus.SUCCESS);
  const directory = await file.getWzDirectory();
  assert.notEqual(directory, null);
  assert.equal(await directory.getName(), "TamingMob_GMS_87.wz");
});

test("disposed remote file wrappers reject later calls", async () => {
  const bytes = fs.readFileSync(sample);
  await using wz = await createWzWorker({ workerFactory: createNodeBrowserWorker });
  const file = await wz.WzFile.fromBlob(
    "TamingMob_GMS_87.wz",
    new Blob([bytes]),
    MapleVersion.GMS,
  );

  await file[Symbol.asyncDispose]();
  await assert.rejects(file.parseWzFile(), /disposed/i);
  await file[Symbol.asyncDispose]();
});

test("client async dispose rejects pending calls and is idempotent", async () => {
  const worker = createSilentWorker();
  const wz = await createWzWorker({ workerFactory: () => worker });
  const pending = wz.WzFile.fromBlob("pending.wz", new Blob([]), MapleVersion.GMS);

  await wz[Symbol.asyncDispose]();
  await assert.rejects(pending, /disposed/i);
  await wz[Symbol.asyncDispose]();
  assert.equal(worker.terminated, 1);
});

test("malformed RPC error responses reject pending calls", async () => {
  const worker = createTrackingWorker();
  await using wz = await createWzWorker({ workerFactory: () => worker });
  const pending = wz.WzFile.fromBlob("broken.wz", new Blob([]), MapleVersion.GMS);

  await Promise.resolve();
  worker.dispatch("message", { id: worker.messages[0].id, ok: false });

  await assert.rejects(pending, /malformed RPC error response/i);
});

test("malformed matching-id RPC responses with invalid ok reject pending calls", async () => {
  const worker = createTrackingWorker();
  await using wz = await createWzWorker({ workerFactory: () => worker });
  const pending = wz.WzFile.fromBlob("broken.wz", new Blob([]), MapleVersion.GMS);

  await Promise.resolve();
  worker.dispatch("message", { id: worker.messages[0].id, ok: "nope" });

  await assert.rejects(
    Promise.race([
      pending,
      new Promise((_, reject) => {
        setTimeout(() => reject(new Error("pending RPC call was not rejected")), 50);
      }),
    ]),
    /malformed RPC error response/i,
  );
});

test("client dispose removes Worker event listeners when supported", async () => {
  const worker = createTrackingWorker();
  const wz = await createWzWorker({ workerFactory: () => worker });

  assert.equal(worker.listenerCount("message"), 1);
  assert.equal(worker.listenerCount("error"), 1);
  assert.equal(worker.listenerCount("messageerror"), 1);

  await wz[Symbol.asyncDispose]();

  assert.equal(worker.listenerCount("message"), 0);
  assert.equal(worker.listenerCount("error"), 0);
  assert.equal(worker.listenerCount("messageerror"), 0);
});
