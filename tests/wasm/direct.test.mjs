import assert from "node:assert/strict";
import fs from "node:fs";
import test from "node:test";
import path from "node:path";
import { Worker } from "node:worker_threads";
import { fileURLToPath } from "node:url";
import {
  createWzApi,
  createWzApiSync,
  MapleVersion,
  ParseStatus,
} from "../../dist/wasm/index.js";

const root = path.dirname(path.dirname(path.dirname(fileURLToPath(import.meta.url))));
const sample = path.join(
  root,
  "Harepacker-resurrected",
  "UnitTest_WzFile",
  "WzFiles",
  "Common",
  "TamingMob_GMS_87.wz",
);

test("sync wasm glue uses Node 18-compatible static createRequire import", () => {
  const syncGluePath = path.join(root, "dist", "wasm", "libwz-sync.js");
  const syncGlue = fs.readFileSync(syncGluePath, "utf8");
  assert.match(syncGlue, /^import \{ createRequire \} from 'node:module';\r?\n/);
  assert.doesNotMatch(syncGlue, /process\.getBuiltinModule/);
  assert.doesNotMatch(syncGlue, /await import\('node:module'\)/);
  assert.match(syncGlue, /\bfunction Module\(moduleArg = \{\}\)/);
  assert.doesNotMatch(syncGlue, /\basync function Module\(moduleArg = \{\}\)/);
  assert.doesNotMatch(syncGlue, /libwz-sync\.wasm/);
  assert.equal(fs.existsSync(path.join(root, "dist", "wasm", "libwz-sync.wasm")), false);
});

test("browser wasm entry does not import Node-only sync glue", () => {
  const distWasm = path.join(root, "dist", "wasm");
  const browserEntry = fs.readFileSync(path.join(distWasm, "browser-index.js"), "utf8");
  const browserLoader = fs.readFileSync(path.join(distWasm, "browser-loader.js"), "utf8");
  const browserGlue = fs.readFileSync(path.join(distWasm, "libwz-browser.js"), "utf8");

  assert.doesNotMatch(browserEntry, /libwz-sync|sync-loader|node:module/);
  assert.doesNotMatch(browserLoader, /libwz-sync|sync-loader|node:module/);
  assert.doesNotMatch(browserGlue, /node:module|require\('node:/);
  assert.equal(fs.existsSync(path.join(distWasm, "libwz-browser.wasm")), false);
});

test("direct wasm API opens path input in Node.js without worker proxy", async () => {
  const before = process._getActiveHandles().filter((handle) =>
    handle.constructor?.name === "Worker",
  ).length;
  const wz = await createWzApi();
  using file = new wz.WzFile(sample, MapleVersion.GMS);
  assert.equal(file.parseWzFile(), ParseStatus.SUCCESS);
  assert.equal(file.getWzDirectory()?.getName(), "TamingMob_GMS_87.wz");
  const after = process._getActiveHandles().filter((handle) =>
    handle.constructor?.name === "Worker",
  ).length;
  assert.equal(after, before);
});

test("direct wasm API can initialize synchronously in Node.js", () => {
  const wasmPath = path.join(root, "dist", "wasm", "libwz.wasm");
  const wz = createWzApiSync({ wasmPath });
  using file = new wz.WzFile(sample, MapleVersion.GMS);
  assert.equal(file.parseWzFile(), ParseStatus.SUCCESS);

  const wasmBytes = fs.readFileSync(wasmPath);
  const wzFromBytes = createWzApiSync({ wasmBytes });
  using fileFromBytes = new wzFromBytes.WzFile(sample, MapleVersion.GMS);
  assert.equal(fileFromBytes.parseWzFile(), ParseStatus.SUCCESS);

  const wasmModule = new WebAssembly.Module(wasmBytes);
  const wzFromModule = createWzApiSync({ wasmModule });
  using fileFromModule = new wzFromModule.WzFile(sample, MapleVersion.GMS);
  assert.equal(fileFromModule.parseWzFile(), ParseStatus.SUCCESS);
});

test("browser Worker direct API opens Blob without full arrayBuffer", async () => {
  const bytes = fs.readFileSync(sample);
  const worker = new Worker(new URL("./direct-worker.mjs", import.meta.url), {
    workerData: {
      bytes: bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
    },
  });
  const result = await new Promise((resolve, reject) => {
    worker.once("message", resolve);
    worker.once("error", reject);
  });
  await worker.terminate();
  assert.equal(result.ok, true, result.message);
  assert.equal(result.status, ParseStatus.SUCCESS);
  assert.equal(result.name, "TamingMob_GMS_87.wz");
});
