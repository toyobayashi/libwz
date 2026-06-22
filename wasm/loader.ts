import { getDefaultContext } from "@emnapi/runtime";

import createLibwzModule, { type LibwzEmscriptenModule } from "./libwz.js";

export type WzBinding = Record<string, unknown>;

export interface LoadedWzModule {
  binding: WzBinding;
  module: LibwzEmscriptenModule;
}

let cached: Promise<LoadedWzModule> | undefined;

export function initializeWasm(wasmUrl: string | URL): Promise<LoadedWzModule> {
  const resolvedWasmUrl = wasmUrl.toString();
  cached ??= createLibwzModule({
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
  return cached;
}

export function loadWzModule(): Promise<LoadedWzModule> {
  return initializeWasm(new URL("./libwz.wasm", import.meta.url));
}
