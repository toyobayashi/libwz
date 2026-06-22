import assert from "node:assert/strict";
import { mkdtemp, readFile, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import test from "node:test";
import { Worker } from "node:worker_threads";

function createRpcWorker(workerUrl = new URL("../../dist/wasm/worker.js", import.meta.url)) {
  const nodeHarness = `
    import { parentPort } from "node:worker_threads";
    globalThis.self = {
      addEventListener(type, listener) {
        if (type === "message") parentPort.on("message", (data) => listener({ data }));
      },
      postMessage(message) {
        parentPort.postMessage(message);
      }
    };
    await import(${JSON.stringify(workerUrl.href)});
  `;
  const worker = new Worker(
    new URL(`data:text/javascript,${encodeURIComponent(nodeHarness)}`),
    { type: "module" },
  );
  let nextId = 1;
  const pending = new Map();

  worker.on("message", (message) => {
    const request = pending.get(message.id);
    if (request === undefined) return;
    pending.delete(message.id);
    clearTimeout(request.timeout);
    request.resolve(message);
  });

  return {
    worker,
    postAndWait(message, responseId = message.id) {
      return new Promise((resolve, reject) => {
        const timeout = setTimeout(() => {
          pending.delete(responseId);
          reject(new Error(`Timed out waiting for RPC response id ${responseId}`));
        }, 1000);
        pending.set(responseId, { resolve, reject, timeout });
        worker.postMessage(message);
      });
    },
    request(method, args = []) {
      const id = nextId++;
      return new Promise((resolve, reject) => {
        const timeout = setTimeout(() => {
          pending.delete(id);
          reject(new Error(`Timed out waiting for RPC response id ${id}`));
        }, 1000);
        pending.set(id, { resolve, reject, timeout });
        worker.postMessage({ id, method, args });
      });
    },
  };
}

async function createWorkerWithStubbedApi(createWzApiSource) {
  const dir = await mkdtemp(join(tmpdir(), "libwz-worker-rpc-"));
  const workerPath = join(dir, "worker.js");
  const indexPath = join(dir, "index.js");
  const source = await readFile(new URL("../../dist/wasm/worker.js", import.meta.url), "utf8");
  await writeFile(workerPath, source);
  await writeFile(indexPath, `export const createWzApi = ${createWzApiSource};\n`);
  return createRpcWorker(pathToFileURL(workerPath));
}

test("worker RPC returns structured errors for unsupported methods", async () => {
  const { worker, request } = createRpcWorker();
  try {
    const response = await request("Unsupported.method");
    assert.deepEqual(response, {
      id: 1,
      ok: false,
      error: {
        name: "Error",
        message: "Unsupported RPC method: Unsupported.method",
      },
    });
  } finally {
    await worker.terminate();
  }
});

test("worker RPC rejects unsupported methods before wasm initialization", async () => {
  const { worker, request } = await createWorkerWithStubbedApi(`() => {
    throw new Error("createWzApi should not be called for unsupported methods");
  }`);
  try {
    const response = await request("Unsupported.method");
    assert.deepEqual(response, {
      id: 1,
      ok: false,
      error: {
        name: "Error",
        message: "Unsupported RPC method: Unsupported.method",
      },
    });
  } finally {
    await worker.terminate();
  }
});

test("worker RPC invalid request errors echo valid numeric ids", async () => {
  const { worker, postAndWait } = createRpcWorker();
  try {
    const response = await postAndWait({ id: 123, method: "WzObject.getName" }, 123);
    assert.deepEqual(response, {
      id: 123,
      ok: false,
      error: {
        name: "TypeError",
        message: "Invalid RPC request",
      },
    });
  } finally {
    await worker.terminate();
  }
});

test("worker RPC validates remote refs before dispatching object methods", async () => {
  const { worker, request } = createRpcWorker();
  try {
    const response = await request("WzObject.getName", [
      { kind: "directory", handle: "missing" },
    ]);
    assert.equal(response.id, 1);
    assert.equal(response.ok, false);
    assert.equal(response.error.name, "Error");
    assert.match(response.error.message, /Unknown remote directory handle: missing/);
  } finally {
    await worker.terminate();
  }
});
