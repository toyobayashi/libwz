export type NativeHandle = bigint;
export type NullableNativeHandle = NativeHandle | null;
export type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray;

export interface NativeObjectInfo {
  type: number;
  ptr: NativeHandle;
}

export interface DetectedMapleVersion {
  mapleVersion: number;
  version: number;
}

export interface NativeBinding {
  openFile(path: string, gameVersion: number, mapleVersion: number): NullableNativeHandle;
  openFileWithIv(path: string, iv: ArrayBufferViewLike): NullableNativeHandle;
  openMemory(name: string, bytes: Uint8Array, gameVersion: number, mapleVersion: number): NullableNativeHandle;
  openMemoryWithIv(name: string, bytes: Uint8Array, iv: ArrayBufferViewLike): NullableNativeHandle;
  openBlobSource?(id: number, size: number, name: string, gameVersion: number, mapleVersion: number): NullableNativeHandle;
  openBlobSourceWithIv?(id: number, size: number, name: string, iv: ArrayBufferViewLike): NullableNativeHandle;
  fileSaveToDisk(ptr: NativeHandle, path: string): void;
  detectMapleVersion(path: string): DetectedMapleVersion;
  [name: string]: unknown;
}
