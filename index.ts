"use strict";

import type {
  ArrayBufferViewLike,
  NativeBinding,
  NativeHandle,
  NativeObjectInfo,
  NullableNativeHandle,
} from "./src/native-binding";
export type { DetectedMapleVersion } from "./src/native-binding";

declare const __dirname: string;
declare function require(id: string): unknown;

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

const handleRegistry = new Map<NativeHandle, Set<WzObject>>();

function registerHandle(ptr: NativeHandle, obj: WzObject): void {
  let entries = handleRegistry.get(ptr);
  if (entries === undefined) {
    entries = new Set<WzObject>();
    handleRegistry.set(ptr, entries);
  }
  entries.add(obj);
}

function unregisterHandle(ptr: NativeHandle, obj: WzObject): void {
  const entries = handleRegistry.get(ptr);
  if (entries === undefined) return;
  entries.delete(obj);
  if (entries.size === 0) handleRegistry.delete(ptr);
}

function markHandleBorrowed(ptr: NativeHandle): void {
  const entries = handleRegistry.get(ptr);
  if (entries === undefined) return;
  for (const entry of entries) entry._markBorrowedInstance();
}

function invalidateHandle(ptr: NativeHandle): void {
  const entries = handleRegistry.get(ptr);
  if (entries === undefined) return;
  handleRegistry.delete(ptr);
  for (const entry of entries) entry._markDisposedInstance();
}

function invalidateHandles(ptrs: Iterable<NativeHandle>): void {
  for (const ptr of ptrs) invalidateHandle(ptr);
}

function registeredObjectsSnapshot(): WzObject[] {
  const objects = new Set<WzObject>();
  for (const entries of handleRegistry.values()) {
    for (const entry of entries) objects.add(entry);
  }
  return Array.from(objects);
}

function livePtr(obj: WzObject): NativeHandle | null {
  try {
    return obj.nativePtr();
  } catch {
    return null;
  }
}

function collectKnownFilePtrs(file: WzFile): Set<NativeHandle> {
  const filePtr = file.nativePtr();
  const ptrs = new Set<NativeHandle>([filePtr]);
  for (const obj of registeredObjectsSnapshot()) {
    const ptr = livePtr(obj);
    if (ptr === null || ptr === filePtr) continue;
    try {
      if (native.objectWzFileParent(ptr) === filePtr) ptrs.add(ptr);
    } catch {
      // Detached properties and already-invalid objects are not file-owned.
    }
  }
  return ptrs;
}

function isDescendantPath(path: string, rootPath: string): boolean {
  return path === rootPath ||
    path.startsWith(`${rootPath}/`) ||
    path.startsWith(`${rootPath}\\`);
}

function collectKnownSubtreePtrs(root: WzObject): Set<NativeHandle> {
  const rootPtr = root.nativePtr();
  const rootPath = native.objectFullPath(rootPtr);
  const ptrs = new Set<NativeHandle>([rootPtr]);
  for (const obj of registeredObjectsSnapshot()) {
    const ptr = livePtr(obj);
    if (ptr === null || ptr === rootPtr) continue;
    try {
      if (isDescendantPath(native.objectFullPath(ptr), rootPath)) {
        ptrs.add(ptr);
      }
    } catch {
      // Some detached properties do not have a tree path.
    }
  }
  return ptrs;
}

function collectPropertySubtreePtrs(prop: WzImageProperty, out = new Set<NativeHandle>()): Set<NativeHandle> {
  const ptr = prop.nativePtr();
  if (out.has(ptr)) return out;
  out.add(ptr);
  for (const child of prop.wzProperties()) collectPropertySubtreePtrs(child, out);
  return out;
}

function collectImagePropertyPtrs(image: WzImage): Set<NativeHandle> {
  const ptrs = new Set<NativeHandle>();
  for (const prop of image.wzProperties()) collectPropertySubtreePtrs(prop, ptrs);
  return ptrs;
}

function collectPropertyChildrenPtrs(prop: WzImageProperty): Set<NativeHandle> {
  const ptrs = new Set<NativeHandle>();
  for (const child of prop.wzProperties()) collectPropertySubtreePtrs(child, ptrs);
  return ptrs;
}

export class WzObject {
  protected _ptr: NativeHandle;
  protected _ownsNative: boolean;

  constructor(ptr: NativeHandle, ownsNative = false) {
    if (new.target === WzObject) {
      throw new TypeError("WzObject is abstract");
    }
    assertPtr(ptr);
    this._ptr = ptr;
    this._ownsNative = ownsNative;
    registerHandle(ptr, this);
  }

  nativePtr(): NativeHandle {
    this._assertAlive();
    return this._ptr;
  }

  protected _assertAlive(): void {
    if (this._ptr === 0n) throw new Error("native object is disposed");
  }

  _markBorrowed(): void {
    this._assertAlive();
    markHandleBorrowed(this._ptr);
  }

  _markDisposed(): void {
    this._assertAlive();
    invalidateHandle(this._ptr);
  }

  _markBorrowedInstance(): void {
    this._ownsNative = false;
  }

  _markDisposedInstance(): void {
    this._ptr = 0n;
    this._ownsNative = false;
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

  setName(name: string): void {
    native.objectSetName(this.nativePtr(), name);
  }

  remove(): void {
    const ptrs = collectKnownSubtreePtrs(this);
    native.objectRemove(this.nativePtr());
    invalidateHandles(ptrs);
  }

  close(): void {}

  [Symbol.dispose](): void {
    this.close();
  }
}

export class WzFile extends WzObject {
  static create(gameVersion: number, mapleVersion: MapleVersionValue): WzFile {
    const ptr = native.createFile(gameVersion, mapleVersion);
    if (ptr === null) throw new Error("failed to create WZ file");
    return createWzFile(ptr, true);
  }

  static fromBytes(name: string, bytes: Uint8Array, mapleVersion: MapleVersionValue): WzFile;
  static fromBytes(name: string, bytes: Uint8Array, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  static fromBytes(name: string, bytes: Uint8Array, iv: ArrayBufferViewLike): WzFile;
  static fromBytes(
    name: string,
    bytes: Uint8Array,
    gameVersionOrMapleVersionOrIv: number | ArrayBufferViewLike,
    mapleVersion?: MapleVersionValue
  ): WzFile {
    let ptr: NullableNativeHandle;
    if (ArrayBuffer.isView(gameVersionOrMapleVersionOrIv)) {
      ptr = native.openMemoryWithIv(name, bytes, gameVersionOrMapleVersionOrIv);
    } else {
      const gameVersion = mapleVersion === undefined ? -1 : gameVersionOrMapleVersionOrIv;
      const version = mapleVersion === undefined
        ? gameVersionOrMapleVersionOrIv as MapleVersionValue
        : mapleVersion;
      ptr = native.openMemory(name, bytes, gameVersion, version);
    }
    if (ptr === null) throw new Error("failed to open WZ file from bytes");
    return createWzFile(ptr, true);
  }

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

  saveToDisk(path: string): void {
    native.fileSaveToDisk(this.nativePtr(), path);
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
      const ptr = this._ptr;
      const ptrs = collectKnownFilePtrs(this);
      native.closeFile(ptr);
      invalidateHandles(ptrs);
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

  createDirectory(name: string): WzDirectory {
    const ptr = native.dirCreateDirectory(this.nativePtr(), name);
    if (ptr === null) throw new Error("failed to create WZ directory");
    return new WzDirectory(ptr);
  }

  createImage(name: string): WzImage {
    const ptr = native.dirCreateImage(this.nativePtr(), name);
    if (ptr === null) throw new Error("failed to create WZ image");
    return new WzImage(ptr);
  }

  removeDirectory(child: WzDirectory): void {
    const ptrs = collectKnownSubtreePtrs(child);
    native.dirRemoveDirectory(this.nativePtr(), child.nativePtr());
    invalidateHandles(ptrs);
  }

  removeImage(child: WzImage): void {
    const ptrs = collectKnownSubtreePtrs(child);
    native.dirRemoveImage(this.nativePtr(), child.nativePtr());
    invalidateHandles(ptrs);
  }

  getBlockSize(): number {
    return native.dirBlockSize(this.nativePtr());
  }

  getChecksum(): number {
    return native.dirChecksum(this.nativePtr());
  }

  getOffset(): bigint {
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

  getOffset(): bigint {
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

  addProperty(prop: WzImageProperty): void {
    native.imageAddProperty(this.nativePtr(), prop.nativePtr());
    prop._markBorrowed();
  }

  removeProperty(prop: WzImageProperty): void {
    const ptrs = collectPropertySubtreePtrs(prop);
    native.imageRemoveProperty(this.nativePtr(), prop.nativePtr());
    invalidateHandles(ptrs);
  }

  clearProperties(): void {
    const ptrs = collectImagePropertyPtrs(this);
    native.imageClearProperties(this.nativePtr());
    invalidateHandles(ptrs);
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

  addProperty(child: WzImageProperty): void {
    native.propertyAddChild(this.nativePtr(), child.nativePtr());
    child._markBorrowed();
  }

  removeProperty(child: WzImageProperty): void {
    const ptrs = collectPropertySubtreePtrs(child);
    native.propertyRemoveChild(this.nativePtr(), child.nativePtr());
    invalidateHandles(ptrs);
  }

  clearProperties(): void {
    const ptrs = collectPropertyChildrenPtrs(this);
    native.propertyClearChildren(this.nativePtr());
    invalidateHandles(ptrs);
  }

  close(): void {
    if (this._ownsNative && this._ptr !== 0n) {
      const ptr = this._ptr;
      native.propertyFree(ptr);
      invalidateHandle(ptr);
    } else if (this._ptr !== 0n) {
      unregisterHandle(this._ptr, this);
      this._ptr = 0n;
    }
  }
}

export class WzNullProperty extends WzImageProperty {}

export class WzShortProperty extends WzImageProperty {
  getValue(): number {
    return native.shortValue(this.nativePtr());
  }

  setValue(value: number): void {
    native.shortSetValue(this.nativePtr(), value);
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

  setValue(value: bigint): void {
    native.longSetValue(this.nativePtr(), value);
  }
}

export class WzFloatProperty extends WzImageProperty {
  getValue(): number {
    return native.floatValue(this.nativePtr());
  }

  setValue(value: number): void {
    native.floatSetValue(this.nativePtr(), value);
  }
}

export class WzDoubleProperty extends WzImageProperty {
  getValue(): number {
    return native.doubleValue(this.nativePtr());
  }

  setValue(value: number): void {
    native.doubleSetValue(this.nativePtr(), value);
  }
}

export class WzStringProperty extends WzImageProperty {
  getValue(): string {
    return native.stringValue(this.nativePtr());
  }

  setValue(value: string): void {
    native.stringSetValue(this.nativePtr(), value);
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

  setValue(value: string): void {
    native.uolSetValue(this.nativePtr(), value);
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

function requireProperty(ptr: NullableNativeHandle): WzImageProperty {
  if (ptr === null) throw new Error("failed to create WZ property");
  return wrapProperty(ptr, true);
}

export class WzProperty {
  static createNull(name: string): WzImageProperty {
    return requireProperty(native.propertyCreateNull(name));
  }

  static createShort(name: string, value: number): WzShortProperty {
    return requireProperty(native.propertyCreateShort(name, value)) as WzShortProperty;
  }

  static createInt(name: string, value: number): WzIntProperty {
    return requireProperty(native.propertyCreateInt(name, value)) as WzIntProperty;
  }

  static createLong(name: string, value: bigint): WzLongProperty {
    return requireProperty(native.propertyCreateLong(name, value)) as WzLongProperty;
  }

  static createFloat(name: string, value: number): WzFloatProperty {
    return requireProperty(native.propertyCreateFloat(name, value)) as WzFloatProperty;
  }

  static createDouble(name: string, value: number): WzDoubleProperty {
    return requireProperty(native.propertyCreateDouble(name, value)) as WzDoubleProperty;
  }

  static createString(name: string, value: string): WzStringProperty {
    return requireProperty(native.propertyCreateString(name, value)) as WzStringProperty;
  }

  static createSub(name: string): WzSubProperty {
    return requireProperty(native.propertyCreateSub(name)) as WzSubProperty;
  }

  static createVector(name: string, x: number, y: number): WzVectorProperty {
    return requireProperty(native.propertyCreateVector(name, x, y)) as WzVectorProperty;
  }

  static createUol(name: string, value: string): WzUOLProperty {
    return requireProperty(native.propertyCreateUol(name, value)) as WzUOLProperty;
  }
}

function wrapProperty(ptr: NativeHandle, ownsNative = false): WzImageProperty {
  const type = native.propType(ptr);
  if (type === PropertyType.NULL) return new WzNullProperty(ptr, ownsNative);
  if (type === PropertyType.SHORT) return new WzShortProperty(ptr, ownsNative);
  if (type === PropertyType.INT) return new WzIntProperty(ptr, ownsNative);
  if (type === PropertyType.LONG) return new WzLongProperty(ptr, ownsNative);
  if (type === PropertyType.FLOAT) return new WzFloatProperty(ptr, ownsNative);
  if (type === PropertyType.DOUBLE) return new WzDoubleProperty(ptr, ownsNative);
  if (type === PropertyType.STRING) return new WzStringProperty(ptr, ownsNative);
  if (type === PropertyType.SUB) return new WzSubProperty(ptr, ownsNative);
  if (type === PropertyType.CANVAS) return new WzCanvasProperty(ptr, ownsNative);
  if (type === PropertyType.VECTOR) return new WzVectorProperty(ptr, ownsNative);
  if (type === PropertyType.CONVEX) return new WzConvexProperty(ptr, ownsNative);
  if (type === PropertyType.SOUND) return new WzBinaryProperty(ptr, ownsNative);
  if (type === PropertyType.RAW) {
    return native.propIsVideo(ptr)
      ? new WzVideoProperty(ptr, ownsNative)
      : new WzRawDataProperty(ptr, ownsNative);
  }
  if (type === PropertyType.UOL) return new WzUOLProperty(ptr, ownsNative);
  if (type === PropertyType.LUA) return new WzLuaProperty(ptr, ownsNative);
  if (type === PropertyType.PNG) return new WzPngProperty(ptr, ownsNative);
  return new WzImageProperty(ptr, ownsNative);
}

export const WzTool = Object.freeze({
  detectMapleVersion: native.detectMapleVersion,
  getIvForVersion: native.ivForVersion,
});
