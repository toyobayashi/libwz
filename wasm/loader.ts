import { getDefaultContext } from "@emnapi/runtime";

import createLibwzModule, {
  type EmscriptenModuleOptions,
  type LibwzEmscriptenModule,
} from "./libwz.js";
import createLibwzModuleSync from "./libwz-sync.js";
import type { NativeBinding } from "./native-binding.js";

export type WzBinding = NativeBinding;

export interface LoadedWzModule {
  binding: WzBinding;
  module: LibwzEmscriptenModule;
}

export interface WzWasmSyncOptions {
  wasmPath?: string;
  wasmBytes?: Uint8Array | ArrayBuffer;
  wasmModule?: WebAssembly.Module;
}

const asyncCache = new Map<string, Promise<LoadedWzModule>>();

export function initializeWasm(wasmUrl: string | URL): Promise<LoadedWzModule> {
  const resolvedWasmUrl = wasmUrl.toString();
  const cached = asyncCache.get(resolvedWasmUrl);
  if (cached !== undefined) return cached;

  const loaded = createLibwzModule({
    locateFile(path, prefix) {
      if (path.endsWith(".wasm")) {
        return resolvedWasmUrl;
      }
      return `${prefix}${path}`;
    },
  }).then((module) => ({
    binding: module.emnapiInit({
      context: getDefaultContext(),
      filename: "libwz.wasm",
    }) as WzBinding,
    module,
  }));
  asyncCache.set(resolvedWasmUrl, loaded);
  return loaded;
}

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

export function loadWzModule(): Promise<LoadedWzModule> {
  return initializeWasm(new URL("./libwz.wasm", import.meta.url));
}
