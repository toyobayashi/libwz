import type { NativeBinding } from './native-binding.js'

export interface WzApiCapabilities {
  blobInput: boolean;
  pathInput: boolean;
  saveToDisk: boolean;
}

export type WzBindingType = 'native' | 'wasm'

let currentNative: NativeBinding | undefined
let currentCapabilities: WzApiCapabilities | undefined
let currentType: WzBindingType | undefined

export function setWzBinding (
  native: NativeBinding,
  capabilities: WzApiCapabilities,
  type: WzBindingType
): void {
  currentNative = native
  currentCapabilities = capabilities
  currentType = type
}

export function getWzBinding (): NativeBinding {
  if (currentNative === undefined) {
    throw new Error('libwz binding is not initialized')
  }
  return currentNative
}

export function getWzCapabilities (): WzApiCapabilities {
  if (currentCapabilities === undefined) {
    throw new Error('libwz binding is not initialized')
  }
  return currentCapabilities
}

export function getWzBindingType (): WzBindingType {
  if (currentType === undefined) {
    throw new Error('libwz binding is not initialized')
  }
  return currentType
}
