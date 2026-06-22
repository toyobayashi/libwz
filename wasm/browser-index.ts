import {
  BinaryType,
  createDirectApi,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
  type WzApi,
  type WzApiOptions,
  type WzApiSyncOptions,
  type WzWasmSyncOptions,
} from "./direct-api.js";
import { initializeWasm, loadWzModule } from "./browser-loader.js";

export {
  BinaryType,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
};
export type { WzApi, WzApiOptions, WzApiSyncOptions, WzWasmSyncOptions };

export async function createWzApi(options: WzApiOptions = {}): Promise<WzApi> {
  const loaded = options.wasmUrl === undefined
    ? await loadWzModule()
    : await initializeWasm(options.wasmUrl);
  return createDirectApi(loaded, options);
}

export function createWzApiSync(_options: WzApiSyncOptions = {}): WzApi {
  throw new Error(
    "createWzApiSync is only supported by the Node.js libwz/wasm entry; " +
      "use createWzApi or libwz/wasm/browser-main in browser bundles.",
  );
}
