import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import test from "node:test";
import { createRequire } from "node:module";
import { Worker } from "node:worker_threads";
import { fileURLToPath } from "node:url";
import {
  createWzApi,
  MapleVersion,
  ParseStatus,
} from "../../dist/wasm/index.js";
import {
  createWzWorker,
  MapleVersion as WorkerMapleVersion,
  ParseStatus as WorkerParseStatus,
} from "../../dist/wasm/browser-main.js";

const require = createRequire(import.meta.url);
const native = require("../../index.js");

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
    removeEventListener(type, listener) {
      if (type === "message") worker.off("message", listener);
      if (type === "error") worker.off("error", listener);
    },
    postMessage(message) {
      worker.postMessage(message);
    },
    terminate() {
      return worker.terminate();
    },
  };
}

function summarizeParsedRoot(file, status, parseStatus) {
  assert.equal(status, parseStatus.SUCCESS);
  const rootDirectory = file.getWzDirectory();
  assert.notEqual(rootDirectory, null);
  const images = rootDirectory.wzImages();
  return {
    status,
    rootName: rootDirectory.getName(),
    imageCount: images.length,
    firstImageName: images[0]?.getName() ?? null,
  };
}

async function summarizeRemoteRoot(file, parseStatus) {
  const status = await file.parseWzFile();
  assert.equal(status, parseStatus.SUCCESS);
  const rootDirectory = await file.getWzDirectory();
  assert.notEqual(rootDirectory, null);

  const images = await rootDirectory.getImages();
  const imageViaAlias = await rootDirectory.wzImages();
  assert.equal(imageViaAlias.length, images.length);
  const firstImage = images[0];
  assert.notEqual(firstImage, undefined);
  const firstImageByIndex = await rootDirectory.getImage(0);
  assert.notEqual(firstImageByIndex, null);
  const firstImageByName = await rootDirectory.getImageByName(await firstImage.getName());
  assert.notEqual(firstImageByName, null);
  assert.deepEqual(await rootDirectory.getDirectories(), await rootDirectory.wzDirectories());

  await firstImage.parseImage();
  const properties = await firstImage.getProperties();
  const propertiesViaAlias = await firstImage.wzProperties();
  assert.equal(propertiesViaAlias.length, properties.length);
  if (properties[0] !== undefined) {
    const firstProperty = properties[0];
    assert.equal(typeof await firstProperty.getName(), "string");
    assert.equal(typeof await firstProperty.getType(), "number");
    const propertyByPath = await firstImage.getFromPath(await firstProperty.getName());
    assert.notEqual(propertyByPath, null);
  }

  return {
    status,
    rootName: await rootDirectory.getName(),
    imageCount: images.length,
    firstImageName: await firstImage.getName(),
  };
}

test("native, direct wasm, and browser proxy expose matching root summaries", async () => {
  const expected = (() => {
    using file = new native.WzFile(sample, native.MapleVersion.GMS);
    return summarizeParsedRoot(file, file.parseWzFile(), native.ParseStatus);
  })();

  const wz = await createWzApi();
  const direct = (() => {
    using file = new wz.WzFile(sample, MapleVersion.GMS);
    return summarizeParsedRoot(file, file.parseWzFile(), ParseStatus);
  })();

  const bytes = fs.readFileSync(sample);
  await using proxy = await createWzWorker({ workerFactory: createNodeBrowserWorker });
  await using file = await proxy.WzFile.fromBlob(
    "TamingMob_GMS_87.wz",
    new Blob([bytes]),
    WorkerMapleVersion.GMS,
  );
  const remote = await summarizeRemoteRoot(file, WorkerParseStatus);

  assert.deepEqual(direct, expected);
  assert.deepEqual(remote, expected);
});

test("disposed browser proxy file invalidates child wrappers", async () => {
  const bytes = fs.readFileSync(sample);
  await using proxy = await createWzWorker({ workerFactory: createNodeBrowserWorker });
  const file = await proxy.WzFile.fromBlob(
    "TamingMob_GMS_87.wz",
    new Blob([bytes]),
    WorkerMapleVersion.GMS,
  );
  assert.equal(await file.parseWzFile(), WorkerParseStatus.SUCCESS);
  const rootDirectory = await file.getWzDirectory();
  assert.notEqual(rootDirectory, null);

  await file[Symbol.asyncDispose]();

  await assert.rejects(file.parseWzFile(), /disposed/i);
  await assert.rejects(rootDirectory.getImages(), /disposed/i);
});
