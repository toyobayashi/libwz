import type {
  BinaryType,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType
} from './node-wrapper.js'

export type NativeHandle = bigint
export type NullableNativeHandle = NativeHandle | null
export type NativeIv = Uint8Array

export interface NativeObjectInfo {
  type: ObjectType;
  ptr: NativeHandle;
}

export interface DetectedMapleVersion {
  mapleVersion: MapleVersion;
  version: number;
}

export type NativePropertyValue = number | bigint | string

export interface NativeOpenFile {
  (path: string, gameVersion: number, mapleVersion: MapleVersion): NullableNativeHandle;
  (path: string, iv: NativeIv): NullableNativeHandle;
}

export interface NativeOpenMemory {
  (name: string, bytes: Uint8Array, gameVersion: number, mapleVersion: MapleVersion): NullableNativeHandle;
  (name: string, bytes: Uint8Array, iv: NativeIv): NullableNativeHandle;
}

export interface NativeOpenBlobSource {
  (size: number, name: string, gameVersion: number, mapleVersion: MapleVersion, readRange: BlobReadRangeCallback): NullableNativeHandle;
  (size: number, name: string, iv: NativeIv, readRange: BlobReadRangeCallback): NullableNativeHandle;
}

export interface NativeBinding {
  openFile: NativeOpenFile;
  openMemory: NativeOpenMemory;
  openBlobSource?: NativeOpenBlobSource;
  createFile(gameVersion: number, mapleVersion: MapleVersion): NullableNativeHandle;
  closeFile(ptr: NativeHandle): void;
  parseFile(ptr: NativeHandle): ParseStatus;
  fileSaveToDisk(ptr: NativeHandle, path: string): void;
  fileName(ptr: NativeHandle): string;
  filePath(ptr: NativeHandle): string;
  fileVersion(ptr: NativeHandle): number;
  fileMapleVersion(ptr: NativeHandle): MapleVersion;
  fileWzDirectory(ptr: NativeHandle): NullableNativeHandle;
  fileIs64Bit(ptr: NativeHandle): boolean;
  fileIsUnloaded(ptr: NativeHandle): boolean;
  fileVersionHash(ptr: NativeHandle): number;
  fileObjectFromPath(ptr: NativeHandle, path: string, checkFirstDirectoryName: boolean): NativeObjectInfo | null;

  objectType(ptr: NativeHandle): ObjectType;
  objectName(ptr: NativeHandle): string;
  objectParent(ptr: NativeHandle): NativeObjectInfo | null;
  objectFullPath(ptr: NativeHandle): string;
  objectWzFileParent(ptr: NativeHandle): NullableNativeHandle;
  objectTopMostDirectory(ptr: NativeHandle): NativeObjectInfo | null;
  objectTopMostImage(ptr: NativeHandle): NativeObjectInfo | null;
  objectAt(ptr: NativeHandle, name: string): NativeObjectInfo | null;
  objectSetName(ptr: NativeHandle, name: string): void;
  objectRemove(ptr: NativeHandle): void;

  dirName(ptr: NativeHandle): string;
  dirCountImagesTotal(ptr: NativeHandle): number;
  dirCountImages(ptr: NativeHandle): number;
  dirCountDirectories(ptr: NativeHandle): number;
  dirGetImage(ptr: NativeHandle, index: number): NullableNativeHandle;
  dirGetImageByName(ptr: NativeHandle, name: string): NullableNativeHandle;
  dirGetDirectory(ptr: NativeHandle, index: number): NullableNativeHandle;
  dirGetDirectoryByName(ptr: NativeHandle, name: string): NullableNativeHandle;
  dirCreateDirectory(ptr: NativeHandle, name: string): NullableNativeHandle;
  dirCreateImage(ptr: NativeHandle, name: string): NullableNativeHandle;
  dirRemoveDirectory(ptr: NativeHandle, child: NativeHandle): void;
  dirRemoveImage(ptr: NativeHandle, child: NativeHandle): void;
  dirBlockSize(ptr: NativeHandle): number;
  dirChecksum(ptr: NativeHandle): number;
  dirOffset(ptr: NativeHandle): bigint;

  imageName(ptr: NativeHandle): string;
  imageParsed(ptr: NativeHandle): boolean;
  imageChanged(ptr: NativeHandle): boolean;
  imageBlockSize(ptr: NativeHandle): number;
  imageChecksum(ptr: NativeHandle): number;
  imageOffset(ptr: NativeHandle): bigint;
  imageIsLua(ptr: NativeHandle): boolean;
  imageParse(ptr: NativeHandle): void;
  imageCountProperties(ptr: NativeHandle): number;
  imageGetProperty(ptr: NativeHandle, index: number): NullableNativeHandle;
  imageGetFromPath(ptr: NativeHandle, path: string): NullableNativeHandle;
  imageAddProperty(ptr: NativeHandle, prop: NativeHandle): void;
  imageRemoveProperty(ptr: NativeHandle, prop: NativeHandle): void;
  imageClearProperties(ptr: NativeHandle): void;

  propType(ptr: NativeHandle): PropertyType;
  propName(ptr: NativeHandle): string;
  propCountChildren(ptr: NativeHandle): number;
  propGetChild(ptr: NativeHandle, index: number): NullableNativeHandle;
  propGetChildByName(ptr: NativeHandle, name: string): NullableNativeHandle;
  propGetFromPath(ptr: NativeHandle, path: string): NullableNativeHandle;
  propLinked(ptr: NativeHandle): NullableNativeHandle;
  propGetInt(ptr: NativeHandle): number;
  propGetShort(ptr: NativeHandle): number;
  propGetLong(ptr: NativeHandle): bigint;
  propGetFloat(ptr: NativeHandle): number;
  propGetDouble(ptr: NativeHandle): number;
  propGetString(ptr: NativeHandle): string;
  propGetBytes(ptr: NativeHandle): Uint8Array;
  propIsVideo(ptr: NativeHandle): boolean;
  propertyValue(ptr: NativeHandle): NativePropertyValue;
  propertySetValue(ptr: NativeHandle, value: NativePropertyValue): void;
  propertyCreateNull(name: string): NullableNativeHandle;
  propertyCreateShort(name: string, value: number): NullableNativeHandle;
  propertyCreateInt(name: string, value: number): NullableNativeHandle;
  propertyCreateLong(name: string, value: bigint): NullableNativeHandle;
  propertyCreateFloat(name: string, value: number): NullableNativeHandle;
  propertyCreateDouble(name: string, value: number): NullableNativeHandle;
  propertyCreateString(name: string, value: string): NullableNativeHandle;
  propertyCreateSub(name: string): NullableNativeHandle;
  propertyCreateVector(name: string, x: number, y: number): NullableNativeHandle;
  propertyCreateUol(name: string, value: string): NullableNativeHandle;
  propertyFree(ptr: NativeHandle): void;
  propertyAddChild(ptr: NativeHandle, child: NativeHandle): void;
  propertyRemoveChild(ptr: NativeHandle, child: NativeHandle): void;
  propertyClearChildren(ptr: NativeHandle): void;

  canvasPng(ptr: NativeHandle): NullableNativeHandle;
  canvasContainsInlink(ptr: NativeHandle): boolean;
  canvasContainsOutlink(ptr: NativeHandle): boolean;
  canvasLinked(ptr: NativeHandle): NullableNativeHandle;
  canvasSaveToFile(ptr: NativeHandle, path: string): boolean;

  pngWidth(ptr: NativeHandle): number;
  pngHeight(ptr: NativeHandle): number;
  pngFormat(ptr: NativeHandle): number;
  pngListWzUsed(ptr: NativeHandle): boolean;
  pngImage(ptr: NativeHandle): Uint8Array;
  pngCompressedBytes(ptr: NativeHandle): Uint8Array;
  pngSaveToFile(ptr: NativeHandle, path: string): boolean;

  vectorX(ptr: NativeHandle): number;
  vectorY(ptr: NativeHandle): number;

  binaryData(ptr: NativeHandle): Uint8Array;
  binaryWav(ptr: NativeHandle): Uint8Array;
  binaryLength(ptr: NativeHandle): number;
  binaryFrequency(ptr: NativeHandle): number;
  binaryType(ptr: NativeHandle): BinaryType;
  binaryHeaderEncrypted(ptr: NativeHandle): boolean;
  binarySaveToFile(ptr: NativeHandle, path: string): boolean;

  rawData(ptr: NativeHandle): Uint8Array;
  rawType(ptr: NativeHandle): number;
  videoData(ptr: NativeHandle): Uint8Array;
  uolLinkValue(ptr: NativeHandle): NativeObjectInfo | null;
  luaData(ptr: NativeHandle): Uint8Array;
  luaString(ptr: NativeHandle): string;

  detectMapleVersion(path: string): DetectedMapleVersion;
  ivForVersion(version: MapleVersion): Uint8Array;
}

export type BlobReadRangeCallback = (
  offset: number,
  length: number,
  destination?: Uint8Array
) => Uint8Array | undefined
