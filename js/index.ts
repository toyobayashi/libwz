import { installNativeBinding } from './native-loader.js'
import { configureWasmBinding } from './wasm/runtime.js'
import { initializeWasm, loadWzModule } from './wasm/loader.js'
export type { DetectedMapleVersion } from './native-binding.js'
export {
  BinaryType,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
  WzBinaryProperty,
  WzCanvasProperty,
  WzConvexProperty,
  WzDirectory,
  WzDoubleProperty,
  WzFile,
  WzFloatProperty,
  WzImage,
  WzImageProperty,
  WzIntProperty,
  WzLongProperty,
  WzLuaProperty,
  WzNullProperty,
  WzObject,
  WzPngProperty,
  WzProperty,
  WzPropertyCollection,
  WzRawDataProperty,
  WzShortProperty,
  WzStringProperty,
  WzSubProperty,
  WzTool,
  WzUOLProperty,
  WzVectorProperty,
  WzVideoProperty
} from './node-wrapper.js'
export { getWzBindingType } from './binding-state.js'

export interface WzInitOptions {
  forceWasm?: boolean;
}

export function init (options?: WzInitOptions): Promise<void>
export function init (wasmUrl: string | URL, options?: WzInitOptions): Promise<void>
export async function init (
  wasmUrlOrOptions?: string | URL | WzInitOptions,
  options: WzInitOptions = {}
): Promise<void> {
  const request = parseInitArgs(wasmUrlOrOptions, options)
  const shouldUseNative = nativeReady && !request.needsWasm
  if (shouldUseNative) {
    await Promise.resolve()
    return
  }

  nativeReady = false
  const loaded = request.wasmUrl === undefined
    ? await loadWzModule()
    : await initializeWasm(request.wasmUrl)
  configureWasmBinding(loaded, request.options)
}

interface InitRequest {
  needsWasm: boolean;
  options: WzInitOptions;
  wasmUrl?: string | URL;
}

function parseInitArgs (
  wasmUrlOrOptions: string | URL | WzInitOptions | undefined,
  options: WzInitOptions
): InitRequest {
  const hasOptionsOnly = isInitOptions(wasmUrlOrOptions)
  const initOptions = hasOptionsOnly ? wasmUrlOrOptions : options
  const wasmUrl = hasOptionsOnly ? undefined : wasmUrlOrOptions
  return {
    needsWasm: wasmUrl !== undefined || initOptions.forceWasm === true,
    options: initOptions,
    wasmUrl
  }
}

function isInitOptions (value: string | URL | WzInitOptions | undefined): value is WzInitOptions {
  return typeof value === 'object' && value !== null && !(value instanceof URL)
}

let nativeReady = installNativeBinding()
