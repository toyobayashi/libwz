import { getDefaultContext } from "@emnapi/runtime";

import createLibwzModule from "./libwz-browser.js";
import type { LoadedWzModule, WzBinding } from "./direct-api.js";

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

export function loadWzModule(): Promise<LoadedWzModule> {
  return initializeWasm(new URL("./libwz.wasm", import.meta.url));
}
