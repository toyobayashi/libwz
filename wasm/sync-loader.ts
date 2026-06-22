import { getDefaultContext } from "@emnapi/runtime";

import createLibwzModuleSync, {
  type EmscriptenModuleOptions,
} from "./libwz-sync.js";
import type {
  LoadedWzModule,
  WzBinding,
  WzWasmSyncOptions,
} from "./direct-api.js";

export function initializeWasmSync(options: WzWasmSyncOptions = {}): LoadedWzModule {
  const wasmInputCount = Number(options.wasmPath !== undefined) +
    Number(options.wasmBytes !== undefined) +
    Number(options.wasmModule !== undefined);
  if (wasmInputCount > 1) {
    throw new Error("Pass only one of wasmPath, wasmBytes, or wasmModule");
  }

  const moduleOptions: EmscriptenModuleOptions = {
    locateFile(path, prefix) {
      if (path.endsWith(".wasm") && options.wasmPath !== undefined) {
        return options.wasmPath;
      }
      return `${prefix}${path}`;
    },
    wasmBinary: options.wasmBytes,
  };

  if (options.wasmModule !== undefined) {
    moduleOptions.instantiateWasm = (imports, receiveInstance) => {
      if (options.wasmModule === undefined) return undefined;
      const instance = new WebAssembly.Instance(options.wasmModule, imports);
      receiveInstance(instance, options.wasmModule);
      return instance.exports;
    };
  }

  const module = createLibwzModuleSync(moduleOptions);
  if (typeof (module as { then?: unknown }).then === "function") {
    throw new Error("synchronous Wasm initialization did not complete synchronously");
  }

  return {
    binding: module.emnapiInit({
      context: getDefaultContext(),
      filename: "libwz.wasm",
    }) as WzBinding,
    module,
  };
}
