"use strict";

declare const __dirname: string;
declare function require(id: string): unknown;

type NativeHandle = bigint;
type NullableNativeHandle = NativeHandle | null;
type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray;

interface NativeObjectInfo {
  type: ObjectTypeValue;
  ptr: NativeHandle;
}

interface NativeBinding {
  openFile(path: string, gameVersion: number, mapleVersion: MapleVersionValue): NullableNativeHandle;
  openFileWithIv(path: string, iv: ArrayBufferViewLike): NullableNativeHandle;
  closeFile(ptr: NativeHandle): void;
  parseFile(ptr: NativeHandle): ParseStatusValue;
  fileName(ptr: NativeHandle): string;
  filePath(ptr: NativeHandle): string;
  fileVersion(ptr: NativeHandle): number;
  fileMapleVersion(ptr: NativeHandle): MapleVersionValue;
  fileWzDirectory(ptr: NativeHandle): NullableNativeHandle;
  fileIs64Bit(ptr: NativeHandle): boolean;
  fileIsUnloaded(ptr: NativeHandle): boolean;
  fileVersionHash(ptr: NativeHandle): number;
  fileObjectFromPath(ptr: NativeHandle, path: string, checkFirstDirectoryName: boolean): NativeObjectInfo | null;

  objectType(ptr: NativeHandle): ObjectTypeValue;
  objectName(ptr: NativeHandle): string;
  objectParent(ptr: NativeHandle): NativeObjectInfo | null;
  objectFullPath(ptr: NativeHandle): string;
  objectWzFileParent(ptr: NativeHandle): NullableNativeHandle;
  objectTopMostDirectory(ptr: NativeHandle): NativeObjectInfo | null;
  objectTopMostImage(ptr: NativeHandle): NativeObjectInfo | null;
  objectAt(ptr: NativeHandle, name: string): NativeObjectInfo | null;

  dirName(ptr: NativeHandle): string;
  dirCountImagesTotal(ptr: NativeHandle): number;
  dirCountImages(ptr: NativeHandle): number;
  dirCountDirectories(ptr: NativeHandle): number;
  dirGetImage(ptr: NativeHandle, index: number): NullableNativeHandle;
  dirGetImageByName(ptr: NativeHandle, name: string): NullableNativeHandle;
  dirGetDirectory(ptr: NativeHandle, index: number): NullableNativeHandle;
  dirGetDirectoryByName(ptr: NativeHandle, name: string): NullableNativeHandle;
  dirBlockSize(ptr: NativeHandle): number;
  dirChecksum(ptr: NativeHandle): number;
  dirOffset(ptr: NativeHandle): number;

  imageName(ptr: NativeHandle): string;
  imageParsed(ptr: NativeHandle): boolean;
  imageChanged(ptr: NativeHandle): boolean;
  imageBlockSize(ptr: NativeHandle): number;
  imageChecksum(ptr: NativeHandle): number;
  imageOffset(ptr: NativeHandle): number;
  imageIsLua(ptr: NativeHandle): boolean;
  imageParse(ptr: NativeHandle): void;
  imageCountProperties(ptr: NativeHandle): number;
  imageGetProperty(ptr: NativeHandle, index: number): NullableNativeHandle;
  imageGetFromPath(ptr: NativeHandle, path: string): NullableNativeHandle;

  propType(ptr: NativeHandle): PropertyTypeValue;
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

  shortValue(ptr: NativeHandle): number;
  intValue(ptr: NativeHandle): number;
  intSetValue(ptr: NativeHandle, value: number): void;
  longValue(ptr: NativeHandle): bigint;
  floatValue(ptr: NativeHandle): number;
  doubleValue(ptr: NativeHandle): number;
  stringValue(ptr: NativeHandle): string;

  canvasPng(ptr: NativeHandle): NullableNativeHandle;
  canvasContainsInlink(ptr: NativeHandle): boolean;
  canvasContainsOutlink(ptr: NativeHandle): boolean;
  canvasLinked(ptr: NativeHandle): NullableNativeHandle;

  pngWidth(ptr: NativeHandle): number;
  pngHeight(ptr: NativeHandle): number;
  pngFormat(ptr: NativeHandle): number;
  pngListWzUsed(ptr: NativeHandle): boolean;
  pngImage(ptr: NativeHandle): Uint8Array;
  pngCompressedBytes(ptr: NativeHandle): Uint8Array;

  vectorX(ptr: NativeHandle): number;
  vectorY(ptr: NativeHandle): number;

  binaryData(ptr: NativeHandle): Uint8Array;
  binaryWav(ptr: NativeHandle): Uint8Array;
  binaryLength(ptr: NativeHandle): number;
  binaryFrequency(ptr: NativeHandle): number;
  binaryType(ptr: NativeHandle): BinaryTypeValue;
  binaryHeaderEncrypted(ptr: NativeHandle): boolean;

  rawData(ptr: NativeHandle): Uint8Array;
  rawType(ptr: NativeHandle): number;
  videoData(ptr: NativeHandle): Uint8Array;
  uolValue(ptr: NativeHandle): string;
  uolLinkValue(ptr: NativeHandle): NativeObjectInfo | null;
  luaData(ptr: NativeHandle): Uint8Array;
  luaString(ptr: NativeHandle): string;

  detectMapleVersion(path: string): MapleVersionValue;
  ivForVersion(version: MapleVersionValue): Uint8Array;
}

interface NodePathModule {
  join(...parts: string[]): string;
}

function loadNative(): NativeBinding {
  const path = require("node:path") as NodePathModule;
  const candidates = [
    path.join(__dirname, "build", "Release", "wz.node"),
    path.join(__dirname, "build", "Release", "libwz.node"),
    path.join(__dirname, "build", "Debug", "wz.node"),
    path.join(__dirname, "build", "Debug", "libwz.node"),
  ];
  for (const candidate of candidates) {
    try {
      return require(candidate) as NativeBinding;
    } catch (err: unknown) {
      const code = (err as { code?: string }).code;
      if (code !== "MODULE_NOT_FOUND") throw err;
    }
  }
  throw new Error("libwz native addon is not built. Run `npm run build`.");
}

const native = loadNative();

export const MapleVersion = Object.freeze({
  GMS: 0,
  EMS: 1,
  BMS: 2,
  CLASSIC: 3,
  GENERATE: 4,
  GETFROMZLZ: 5,
  CUSTOM: 6,
  UNKNOWN: 99,
} as const);
export type MapleVersionValue = (typeof MapleVersion)[keyof typeof MapleVersion];

export const ParseStatus = Object.freeze({
  PATH_IS_NULL: -1,
  ERROR_GAME_VER_HASH: -2,
  FAILED_UNKNOWN: 0,
  SUCCESS: 1,
} as const);
export type ParseStatusValue = (typeof ParseStatus)[keyof typeof ParseStatus];

export const PropertyType = Object.freeze({
  NULL: 0,
  SHORT: 1,
  INT: 2,
  LONG: 3,
  FLOAT: 4,
  DOUBLE: 5,
  STRING: 6,
  SUB: 7,
  CANVAS: 8,
  VECTOR: 9,
  CONVEX: 10,
  SOUND: 11,
  RAW: 12,
  UOL: 13,
  LUA: 14,
  PNG: 15,
} as const);
export type PropertyTypeValue = (typeof PropertyType)[keyof typeof PropertyType];

export const ObjectType = Object.freeze({
  FILE: 0,
  IMAGE: 1,
  DIRECTORY: 2,
  PROPERTY: 3,
  LIST: 4,
} as const);
export type ObjectTypeValue = (typeof ObjectType)[keyof typeof ObjectType];

export const BinaryType = Object.freeze({
  RAW: 0,
  MP3: 1,
  WAV: 2,
} as const);
export type BinaryTypeValue = (typeof BinaryType)[keyof typeof BinaryType];

function assertPtr(ptr: unknown): asserts ptr is NativeHandle {
  if (typeof ptr !== "bigint" || ptr === 0n) {
    throw new TypeError("native pointer must be a non-zero bigint");
  }
}

function isPresent<T>(value: T | null): value is T {
  return value !== null;
}

function createWzFile(ptr: NativeHandle, ownsNative = false): WzFile {
  const NativeWzFile = WzFile as unknown as {
    new(ptr: NativeHandle, ownsNative?: boolean): WzFile;
  };
  return new NativeWzFile(ptr, ownsNative);
}

export class WzObject {
  protected _ptr: NativeHandle;
  protected readonly _ownsNative: boolean;

  constructor(ptr: NativeHandle, ownsNative = false) {
    if (new.target === WzObject) {
      throw new TypeError("WzObject is abstract");
    }
    assertPtr(ptr);
    this._ptr = ptr;
    this._ownsNative = ownsNative;
  }

  nativePtr(): NativeHandle {
    this._assertAlive();
    return this._ptr;
  }

  protected _assertAlive(): void {
    if (this._ptr === 0n) throw new Error("native object is disposed");
  }

  ownsNative(): boolean {
    return this._ownsNative;
  }

  getObjectType(): ObjectTypeValue {
    return native.objectType(this.nativePtr());
  }

  getName(): string {
    return native.objectName(this.nativePtr());
  }

  getParent(): WzObject | null {
    return wrapObjectInfo(native.objectParent(this.nativePtr()));
  }

  getFullPath(): string {
    return native.objectFullPath(this.nativePtr());
  }

  getWzFileParent(): WzFile | null {
    const ptr = native.objectWzFileParent(this.nativePtr());
    return ptr === null ? null : createWzFile(ptr);
  }

  getTopMostWzDirectory(): WzObject | null {
    return wrapObjectInfo(native.objectTopMostDirectory(this.nativePtr()));
  }

  getTopMostWzImage(): WzObject | null {
    return wrapObjectInfo(native.objectTopMostImage(this.nativePtr()));
  }

  at(name: string): WzObject | null {
    return wrapObjectInfo(native.objectAt(this.nativePtr(), name));
  }

  close(): void {}

  [Symbol.dispose](): void {
    this.close();
  }
}

export class WzFile extends WzObject {
  constructor(path: string, mapleVersion?: MapleVersionValue);
  constructor(path: string, gameVersion: number, mapleVersion: MapleVersionValue);
  constructor(path: string, iv: ArrayBufferViewLike);
  constructor(pathOrPtr: string | NativeHandle, gameVersionOrMapleVersion: number | ArrayBufferViewLike | boolean = MapleVersion.GMS, mapleVersion?: MapleVersionValue) {
    if (typeof pathOrPtr === "bigint") {
      super(pathOrPtr, gameVersionOrMapleVersion === true);
      return;
    }

    let ptr: NullableNativeHandle;
    if (ArrayBuffer.isView(gameVersionOrMapleVersion)) {
      ptr = native.openFileWithIv(pathOrPtr, gameVersionOrMapleVersion as ArrayBufferViewLike);
    } else {
      const gameVersion = mapleVersion === undefined ? -1 : gameVersionOrMapleVersion as number;
      const version = mapleVersion === undefined
        ? gameVersionOrMapleVersion as MapleVersionValue
        : mapleVersion;
      ptr = native.openFile(pathOrPtr, gameVersion, version);
    }
    if (ptr === null) throw new Error("failed to open WZ file");
    super(ptr, true);
  }

  parseWzFile(): ParseStatusValue {
    return native.parseFile(this.nativePtr());
  }

  getName(): string {
    return native.fileName(this.nativePtr());
  }

  getFilePath(): string {
    return native.filePath(this.nativePtr());
  }

  getVersion(): number {
    return native.fileVersion(this.nativePtr());
  }

  getMapleVersion(): MapleVersionValue {
    return native.fileMapleVersion(this.nativePtr());
  }

  getWzDirectory(): WzDirectory | null {
    const ptr = native.fileWzDirectory(this.nativePtr());
    return ptr === null ? null : new WzDirectory(ptr);
  }

  is64BitWzFile(): boolean {
    return native.fileIs64Bit(this.nativePtr());
  }

  isUnloaded(): boolean {
    return native.fileIsUnloaded(this.nativePtr());
  }

  getVersionHash(): number {
    return native.fileVersionHash(this.nativePtr());
  }

  getObjectFromPath(path: string, checkFirstDirectoryName = true): WzObject | null {
    return wrapObjectInfo(native.fileObjectFromPath(
      this.nativePtr(),
      path,
      checkFirstDirectoryName
    ));
  }

  close(): void {
    if (this._ownsNative && this._ptr !== 0n) {
      native.closeFile(this._ptr);
      this._ptr = 0n;
    }
  }
}

export class WzDirectory extends WzObject {
  getName(): string {
    return native.dirName(this.nativePtr());
  }

  countImages(): number {
    return native.dirCountImagesTotal(this.nativePtr());
  }

  wzImages(): Array<WzImage | null> {
    const count = native.dirCountImages(this.nativePtr());
    return Array.from({ length: count }, (_, i) => this.getImage(i));
  }

  wzDirectories(): Array<WzDirectory | null> {
    const count = native.dirCountDirectories(this.nativePtr());
    return Array.from({ length: count }, (_, i) => this.getDirectory(i));
  }

  getImage(index: number): WzImage | null {
    const ptr = native.dirGetImage(this.nativePtr(), index);
    return ptr === null ? null : new WzImage(ptr);
  }

  getImageByName(name: string): WzImage | null {
    const ptr = native.dirGetImageByName(this.nativePtr(), name);
    return ptr === null ? null : new WzImage(ptr);
  }

  getDirectory(index: number): WzDirectory | null {
    const ptr = native.dirGetDirectory(this.nativePtr(), index);
    return ptr === null ? null : new WzDirectory(ptr);
  }

  getDirectoryByName(name: string): WzDirectory | null {
    const ptr = native.dirGetDirectoryByName(this.nativePtr(), name);
    return ptr === null ? null : new WzDirectory(ptr);
  }

  getBlockSize(): number {
    return native.dirBlockSize(this.nativePtr());
  }

  getChecksum(): number {
    return native.dirChecksum(this.nativePtr());
  }

  getOffset(): number {
    return native.dirOffset(this.nativePtr());
  }
}

export class WzImage extends WzObject {
  getName(): string {
    return native.imageName(this.nativePtr());
  }

  isParsed(): boolean {
    return native.imageParsed(this.nativePtr());
  }

  isChanged(): boolean {
    return native.imageChanged(this.nativePtr());
  }

  getBlockSize(): number {
    return native.imageBlockSize(this.nativePtr());
  }

  getChecksum(): number {
    return native.imageChecksum(this.nativePtr());
  }

  getOffset(): number {
    return native.imageOffset(this.nativePtr());
  }

  isLuaWzImage(): boolean {
    return native.imageIsLua(this.nativePtr());
  }

  parseImage(): void {
    native.imageParse(this.nativePtr());
  }

  wzProperties(): WzPropertyCollection {
    const count = native.imageCountProperties(this.nativePtr());
    return new WzPropertyCollection(...Array.from({ length: count }, (_, i) => {
      const ptr = native.imageGetProperty(this.nativePtr(), i);
      return ptr === null ? null : wrapProperty(ptr);
    }).filter(isPresent));
  }

  getFromPath(path: string): WzImageProperty | null {
    const ptr = native.imageGetFromPath(this.nativePtr(), path);
    return ptr === null ? null : wrapProperty(ptr);
  }
}

export class WzPropertyCollection extends Array<WzImageProperty> {
  get(index: number): WzImageProperty | undefined {
    return this[index];
  }
}

export class WzImageProperty extends WzObject {
  getPropertyType(): PropertyTypeValue {
    return native.propType(this.nativePtr());
  }

  getName(): string {
    return native.propName(this.nativePtr());
  }

  wzProperties(): WzPropertyCollection {
    const count = native.propCountChildren(this.nativePtr());
    return new WzPropertyCollection(
      ...Array.from({ length: count }, (_, i) => this.getChild(i)).filter(isPresent)
    );
  }

  getChild(index: number): WzImageProperty | null {
    const ptr = native.propGetChild(this.nativePtr(), index);
    return ptr === null ? null : wrapProperty(ptr);
  }

  getChildByName(name: string): WzImageProperty | null {
    const ptr = native.propGetChildByName(this.nativePtr(), name);
    return ptr === null ? null : wrapProperty(ptr);
  }

  getFromPath(path: string): WzImageProperty | null {
    const ptr = native.propGetFromPath(this.nativePtr(), path);
    return ptr === null ? null : wrapProperty(ptr);
  }

  getLinkedWzImageProperty(): WzImageProperty | null {
    const ptr = native.propLinked(this.nativePtr());
    return ptr === null ? null : wrapProperty(ptr);
  }

  getInt(): number {
    return native.propGetInt(this.nativePtr());
  }

  getShort(): number {
    return native.propGetShort(this.nativePtr());
  }

  getLong(): bigint {
    return native.propGetLong(this.nativePtr());
  }

  getFloat(): number {
    return native.propGetFloat(this.nativePtr());
  }

  getDouble(): number {
    return native.propGetDouble(this.nativePtr());
  }

  getString(): string {
    return native.propGetString(this.nativePtr());
  }

  getBytes(): Uint8Array {
    return native.propGetBytes(this.nativePtr());
  }
}

export class WzNullProperty extends WzImageProperty {}

export class WzShortProperty extends WzImageProperty {
  getValue(): number {
    return native.shortValue(this.nativePtr());
  }
}

export class WzIntProperty extends WzImageProperty {
  getValue(): number {
    return native.intValue(this.nativePtr());
  }

  setValue(value: number): void {
    native.intSetValue(this.nativePtr(), value);
  }
}

export class WzLongProperty extends WzImageProperty {
  getValue(): bigint {
    return native.longValue(this.nativePtr());
  }
}

export class WzFloatProperty extends WzImageProperty {
  getValue(): number {
    return native.floatValue(this.nativePtr());
  }
}

export class WzDoubleProperty extends WzImageProperty {
  getValue(): number {
    return native.doubleValue(this.nativePtr());
  }
}

export class WzStringProperty extends WzImageProperty {
  getValue(): string {
    return native.stringValue(this.nativePtr());
  }
}

export class WzSubProperty extends WzImageProperty {}

export class WzCanvasProperty extends WzImageProperty {
  getPngProperty(): WzPngProperty | null {
    const ptr = native.canvasPng(this.nativePtr());
    return ptr === null ? null : new WzPngProperty(ptr);
  }

  containsInlinkProperty(): boolean {
    return native.canvasContainsInlink(this.nativePtr());
  }

  containsOutlinkProperty(): boolean {
    return native.canvasContainsOutlink(this.nativePtr());
  }

  getLinkedWzImageProperty(): WzImageProperty | null {
    const ptr = native.canvasLinked(this.nativePtr());
    return ptr === null ? null : wrapProperty(ptr);
  }
}

export class WzPngProperty extends WzImageProperty {
  getWidth(): number {
    return native.pngWidth(this.nativePtr());
  }

  getHeight(): number {
    return native.pngHeight(this.nativePtr());
  }

  getFormat(): number {
    return native.pngFormat(this.nativePtr());
  }

  isListWzUsed(): boolean {
    return native.pngListWzUsed(this.nativePtr());
  }

  getImage(): Uint8Array {
    return native.pngImage(this.nativePtr());
  }

  getCompressedBytes(): Uint8Array {
    return native.pngCompressedBytes(this.nativePtr());
  }
}

export class WzVectorProperty extends WzImageProperty {
  getX(): number {
    return native.vectorX(this.nativePtr());
  }

  getY(): number {
    return native.vectorY(this.nativePtr());
  }
}

export class WzConvexProperty extends WzImageProperty {}

export class WzBinaryProperty extends WzImageProperty {
  getBytes(): Uint8Array {
    return native.binaryData(this.nativePtr());
  }

  getWAVPlayback(): Uint8Array {
    return native.binaryWav(this.nativePtr());
  }

  getLength(): number {
    return native.binaryLength(this.nativePtr());
  }

  getFrequency(): number {
    return native.binaryFrequency(this.nativePtr());
  }

  getType(): BinaryTypeValue {
    return native.binaryType(this.nativePtr());
  }

  isHeaderEncrypted(): boolean {
    return native.binaryHeaderEncrypted(this.nativePtr());
  }
}

export class WzRawDataProperty extends WzImageProperty {
  getBytes(): Uint8Array {
    return native.rawData(this.nativePtr());
  }

  getRawType(): number {
    return native.rawType(this.nativePtr());
  }
}

export class WzVideoProperty extends WzImageProperty {
  getData(): Uint8Array {
    return native.videoData(this.nativePtr());
  }
}

export class WzUOLProperty extends WzImageProperty {
  getValue(): string {
    return native.uolValue(this.nativePtr());
  }

  getLinkValue(): WzObject | null {
    return wrapObjectInfo(native.uolLinkValue(this.nativePtr()));
  }
}

export class WzLuaProperty extends WzImageProperty {
  getData(): Uint8Array {
    return native.luaData(this.nativePtr());
  }

  getString(): string {
    return native.luaString(this.nativePtr());
  }
}

function wrapObjectInfo(info: NativeObjectInfo | null): WzObject | null {
  if (info === null) return null;
  return wrapObject(info.type, info.ptr);
}

function wrapObject(type: ObjectTypeValue, ptr: NativeHandle): WzObject | null {
  if (type === ObjectType.FILE) return createWzFile(ptr);
  if (type === ObjectType.IMAGE) return new WzImage(ptr);
  if (type === ObjectType.DIRECTORY) return new WzDirectory(ptr);
  if (type === ObjectType.PROPERTY) return wrapProperty(ptr);
  return null;
}

function wrapProperty(ptr: NativeHandle): WzImageProperty {
  const type = native.propType(ptr);
  if (type === PropertyType.NULL) return new WzNullProperty(ptr);
  if (type === PropertyType.SHORT) return new WzShortProperty(ptr);
  if (type === PropertyType.INT) return new WzIntProperty(ptr);
  if (type === PropertyType.LONG) return new WzLongProperty(ptr);
  if (type === PropertyType.FLOAT) return new WzFloatProperty(ptr);
  if (type === PropertyType.DOUBLE) return new WzDoubleProperty(ptr);
  if (type === PropertyType.STRING) return new WzStringProperty(ptr);
  if (type === PropertyType.SUB) return new WzSubProperty(ptr);
  if (type === PropertyType.CANVAS) return new WzCanvasProperty(ptr);
  if (type === PropertyType.VECTOR) return new WzVectorProperty(ptr);
  if (type === PropertyType.CONVEX) return new WzConvexProperty(ptr);
  if (type === PropertyType.SOUND) return new WzBinaryProperty(ptr);
  if (type === PropertyType.RAW) {
    return native.propIsVideo(ptr)
      ? new WzVideoProperty(ptr)
      : new WzRawDataProperty(ptr);
  }
  if (type === PropertyType.UOL) return new WzUOLProperty(ptr);
  if (type === PropertyType.LUA) return new WzLuaProperty(ptr);
  if (type === PropertyType.PNG) return new WzPngProperty(ptr);
  return new WzImageProperty(ptr);
}

export const WzTool = Object.freeze({
  detectMapleVersion: native.detectMapleVersion,
  getIvForVersion: native.ivForVersion,
});
