import assert from "node:assert/strict";
import { createWzWorker } from "../../dist/wasm/browser.js";
import { loadWzModule } from "../../dist/wasm/loader.js";

assert.equal(typeof createWzWorker, "function");

const { module } = await loadWzModule();
assert.ok(module.HEAPU8 instanceof Uint8Array);
