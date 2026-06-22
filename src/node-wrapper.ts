"use strict";

import type {
  ArrayBufferViewLike,
  DetectedMapleVersion,
  NativeBinding,
  NativeHandle,
  NativeObjectInfo,
  NullableNativeHandle,
} from "./native-binding.js";

export interface WzApiCapabilities {
  blobInput: boolean;
  pathInput: boolean;
  saveToDisk: boolean;
}

export interface BlobLike {
  readonly size: number;
  slice(start?: number, end?: number): BlobLike;
}

export interface FileLike extends BlobLike {
  readonly name: string;
}

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

export interface WzObject {
  nativePtr(): NativeHandle;
  _markBorrowed(): void;
  _markDisposed(): void;
  _markBorrowedInstance(): void;
  _markDisposedInstance(): void;
  ownsNative(): boolean;
  getObjectType(): ObjectTypeValue;
  getName(): string;
  getParent(): WzObject | null;
  getFullPath(): string;
  getWzFileParent(): WzFile | null;
  getTopMostWzDirectory(): WzObject | null;
  getTopMostWzImage(): WzObject | null;
  at(name: string): WzObject | null;
  setName(name: string): void;
  remove(): void;
  close(): void;
  [Symbol.dispose](): void;
}

export interface WzObjectConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzObject;
  readonly prototype: WzObject;
}

export interface WzFile extends WzObject {
  parseWzFile(): ParseStatusValue;
  saveToDisk(path: string): void;
  getName(): string;
  getFilePath(): string;
  getVersion(): number;
  getMapleVersion(): MapleVersionValue;
  getWzDirectory(): WzDirectory | null;
  is64BitWzFile(): boolean;
  isUnloaded(): boolean;
  getVersionHash(): number;
  getObjectFromPath(path: string, checkFirstDirectoryName?: boolean): WzObject | null;
  close(): void;
}

export interface WzFileConstructor extends Function {
  new(path: string, mapleVersion?: MapleVersionValue): WzFile;
  new(path: string, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  new(path: string, iv: ArrayBufferViewLike): WzFile;
  readonly prototype: WzFile;
  create(gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  fromBytes(name: string, bytes: Uint8Array, mapleVersion: MapleVersionValue): WzFile;
  fromBytes(name: string, bytes: Uint8Array, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  fromBytes(name: string, bytes: Uint8Array, iv: ArrayBufferViewLike): WzFile;
  fromBlob(name: string, blob: BlobLike, mapleVersion: MapleVersionValue): WzFile;
  fromBlob(name: string, blob: BlobLike, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  fromBlobWithIv(name: string, blob: BlobLike, iv: ArrayBufferViewLike): WzFile;
  fromFile(file: FileLike, mapleVersion: MapleVersionValue): WzFile;
  fromFile(file: FileLike, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
}

export interface WzDirectory extends WzObject {
  getName(): string;
  countImages(): number;
  wzImages(): Array<WzImage | null>;
  wzDirectories(): Array<WzDirectory | null>;
  getImage(index: number): WzImage | null;
  getImageByName(name: string): WzImage | null;
  getDirectory(index: number): WzDirectory | null;
  getDirectoryByName(name: string): WzDirectory | null;
  createDirectory(name: string): WzDirectory;
  createImage(name: string): WzImage;
  removeDirectory(child: WzDirectory): void;
  removeImage(child: WzImage): void;
  getBlockSize(): number;
  getChecksum(): number;
  getOffset(): bigint;
}

export interface WzDirectoryConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzDirectory;
  readonly prototype: WzDirectory;
}

export interface WzImage extends WzObject {
  getName(): string;
  isParsed(): boolean;
  isChanged(): boolean;
  getBlockSize(): number;
  getChecksum(): number;
  getOffset(): bigint;
  isLuaWzImage(): boolean;
  parseImage(): void;
  wzProperties(): WzPropertyCollection;
  getFromPath(path: string): WzImageProperty | null;
  addProperty(prop: WzImageProperty): void;
  removeProperty(prop: WzImageProperty): void;
  clearProperties(): void;
}

export interface WzImageConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzImage;
  readonly prototype: WzImage;
}

export interface WzPropertyCollection extends Array<WzImageProperty> {
  get(index: number): WzImageProperty | undefined;
}

export interface WzPropertyCollectionConstructor extends Function {
  new(arrayLength: number): WzPropertyCollection;
  new(...items: WzImageProperty[]): WzPropertyCollection;
  readonly prototype: WzPropertyCollection;
}

export interface WzImageProperty extends WzObject {
  getPropertyType(): PropertyTypeValue;
  getName(): string;
  wzProperties(): WzPropertyCollection;
  getChild(index: number): WzImageProperty | null;
  getChildByName(name: string): WzImageProperty | null;
  getFromPath(path: string): WzImageProperty | null;
  getLinkedWzImageProperty(): WzImageProperty | null;
  getInt(): number;
  getShort(): number;
  getLong(): bigint;
  getFloat(): number;
  getDouble(): number;
  getString(): string;
  getBytes(): Uint8Array;
  addProperty(child: WzImageProperty): void;
  removeProperty(child: WzImageProperty): void;
  clearProperties(): void;
  close(): void;
}

export interface WzImagePropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzImageProperty;
  readonly prototype: WzImageProperty;
}

export interface WzNullProperty extends WzImageProperty {}
export interface WzNullPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzNullProperty;
  readonly prototype: WzNullProperty;
}

export interface WzShortProperty extends WzImageProperty {
  getValue(): number;
  setValue(value: number): void;
}
export interface WzShortPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzShortProperty;
  readonly prototype: WzShortProperty;
}

export interface WzIntProperty extends WzImageProperty {
  getValue(): number;
  setValue(value: number): void;
}
export interface WzIntPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzIntProperty;
  readonly prototype: WzIntProperty;
}

export interface WzLongProperty extends WzImageProperty {
  getValue(): bigint;
  setValue(value: bigint): void;
}
export interface WzLongPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzLongProperty;
  readonly prototype: WzLongProperty;
}

export interface WzFloatProperty extends WzImageProperty {
  getValue(): number;
  setValue(value: number): void;
}
export interface WzFloatPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzFloatProperty;
  readonly prototype: WzFloatProperty;
}

export interface WzDoubleProperty extends WzImageProperty {
  getValue(): number;
  setValue(value: number): void;
}
export interface WzDoublePropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzDoubleProperty;
  readonly prototype: WzDoubleProperty;
}

export interface WzStringProperty extends WzImageProperty {
  getValue(): string;
  setValue(value: string): void;
}
export interface WzStringPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzStringProperty;
  readonly prototype: WzStringProperty;
}

export interface WzSubProperty extends WzImageProperty {}
export interface WzSubPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzSubProperty;
  readonly prototype: WzSubProperty;
}

export interface WzCanvasProperty extends WzImageProperty {
  getPngProperty(): WzPngProperty | null;
  containsInlinkProperty(): boolean;
  containsOutlinkProperty(): boolean;
  getLinkedWzImageProperty(): WzImageProperty | null;
}
export interface WzCanvasPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzCanvasProperty;
  readonly prototype: WzCanvasProperty;
}

export interface WzPngProperty extends WzImageProperty {
  getWidth(): number;
  getHeight(): number;
  getFormat(): number;
  isListWzUsed(): boolean;
  getImage(): Uint8Array;
  getCompressedBytes(): Uint8Array;
}
export interface WzPngPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzPngProperty;
  readonly prototype: WzPngProperty;
}

export interface WzVectorProperty extends WzImageProperty {
  getX(): number;
  getY(): number;
}
export interface WzVectorPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzVectorProperty;
  readonly prototype: WzVectorProperty;
}

export interface WzConvexProperty extends WzImageProperty {}
export interface WzConvexPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzConvexProperty;
  readonly prototype: WzConvexProperty;
}

export interface WzBinaryProperty extends WzImageProperty {
  getBytes(): Uint8Array;
  getWAVPlayback(): Uint8Array;
  getLength(): number;
  getFrequency(): number;
  getType(): BinaryTypeValue;
  isHeaderEncrypted(): boolean;
}
export interface WzBinaryPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzBinaryProperty;
  readonly prototype: WzBinaryProperty;
}

export interface WzRawDataProperty extends WzImageProperty {
  getBytes(): Uint8Array;
  getRawType(): number;
}
export interface WzRawDataPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzRawDataProperty;
  readonly prototype: WzRawDataProperty;
}

export interface WzVideoProperty extends WzImageProperty {
  getData(): Uint8Array;
}
export interface WzVideoPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzVideoProperty;
  readonly prototype: WzVideoProperty;
}

export interface WzUOLProperty extends WzImageProperty {
  getValue(): string;
  setValue(value: string): void;
  getLinkValue(): WzObject | null;
}
export interface WzUOLPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzUOLProperty;
  readonly prototype: WzUOLProperty;
}

export interface WzLuaProperty extends WzImageProperty {
  getData(): Uint8Array;
  getString(): string;
}
export interface WzLuaPropertyConstructor extends Function {
  new(ptr: NativeHandle, ownsNative?: boolean): WzLuaProperty;
  readonly prototype: WzLuaProperty;
}

export interface WzProperty {}

export interface WzPropertyConstructor extends Function {
  new(): WzProperty;
  readonly prototype: WzProperty;
  createNull(name: string): WzImageProperty;
  createShort(name: string, value: number): WzShortProperty;
  createInt(name: string, value: number): WzIntProperty;
  createLong(name: string, value: bigint): WzLongProperty;
  createFloat(name: string, value: number): WzFloatProperty;
  createDouble(name: string, value: number): WzDoubleProperty;
  createString(name: string, value: string): WzStringProperty;
  createSub(name: string): WzSubProperty;
  createVector(name: string, x: number, y: number): WzVectorProperty;
  createUol(name: string, value: string): WzUOLProperty;
}

export interface WzToolApi {
  readonly detectMapleVersion: (path: string) => DetectedMapleVersion;
  readonly getIvForVersion: (version: MapleVersionValue) => Uint8Array;
}

export interface WzApi {
  MapleVersion: typeof MapleVersion;
  ParseStatus: typeof ParseStatus;
  PropertyType: typeof PropertyType;
  ObjectType: typeof ObjectType;
  BinaryType: typeof BinaryType;
  WzObject: WzObjectConstructor;
  WzFile: WzFileConstructor;
  WzDirectory: WzDirectoryConstructor;
  WzImage: WzImageConstructor;
  WzPropertyCollection: WzPropertyCollectionConstructor;
  WzImageProperty: WzImagePropertyConstructor;
  WzNullProperty: WzNullPropertyConstructor;
  WzShortProperty: WzShortPropertyConstructor;
  WzIntProperty: WzIntPropertyConstructor;
  WzLongProperty: WzLongPropertyConstructor;
  WzFloatProperty: WzFloatPropertyConstructor;
  WzDoubleProperty: WzDoublePropertyConstructor;
  WzStringProperty: WzStringPropertyConstructor;
  WzSubProperty: WzSubPropertyConstructor;
  WzCanvasProperty: WzCanvasPropertyConstructor;
  WzPngProperty: WzPngPropertyConstructor;
  WzVectorProperty: WzVectorPropertyConstructor;
  WzConvexProperty: WzConvexPropertyConstructor;
  WzBinaryProperty: WzBinaryPropertyConstructor;
  WzRawDataProperty: WzRawDataPropertyConstructor;
  WzVideoProperty: WzVideoPropertyConstructor;
  WzUOLProperty: WzUOLPropertyConstructor;
  WzLuaProperty: WzLuaPropertyConstructor;
  WzProperty: WzPropertyConstructor;
  WzTool: WzToolApi;
}

export function createWzApiFromBinding(native: NativeBinding, capabilities: WzApiCapabilities): WzApi {
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

  class WzObject {
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

  class WzFile extends WzObject {
    private readonly disposeCallbacks: Array<() => void> = [];

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

    static fromBlobSource(name: string, id: number, size: number, mapleVersion: MapleVersionValue): WzFile;
    static fromBlobSource(name: string, id: number, size: number, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
    static fromBlobSource(name: string, id: number, size: number, iv: ArrayBufferViewLike): WzFile;
    static fromBlobSource(
      name: string,
      id: number,
      size: number,
      gameVersionOrMapleVersionOrIv: number | ArrayBufferViewLike,
      mapleVersion?: MapleVersionValue
    ): WzFile {
      if (!capabilities.blobInput) {
        throw new Error("Blob-backed WZ input is not supported in this runtime");
      }

      let ptr: NullableNativeHandle;
      if (ArrayBuffer.isView(gameVersionOrMapleVersionOrIv)) {
        const openBlobSourceWithIv = native.openBlobSourceWithIv;
        if (openBlobSourceWithIv === undefined) {
          throw new Error("Blob-backed WZ input with IV is not supported by this binding");
        }
        ptr = openBlobSourceWithIv(id, size, name, gameVersionOrMapleVersionOrIv);
      } else {
        const openBlobSource = native.openBlobSource;
        if (openBlobSource === undefined) {
          throw new Error("Blob-backed WZ input is not supported by this binding");
        }
        const gameVersion = mapleVersion === undefined ? -1 : gameVersionOrMapleVersionOrIv;
        const version = mapleVersion === undefined
          ? gameVersionOrMapleVersionOrIv as MapleVersionValue
          : mapleVersion;
        ptr = openBlobSource(id, size, name, gameVersion, version);
      }
      if (ptr === null) throw new Error("failed to open WZ file from Blob");
      return createWzFile(ptr, true);
    }

    static fromBlob(): WzFile {
      throw new Error("Blob-backed WZ input is not configured for this runtime");
    }

    static fromBlobWithIv(): WzFile {
      throw new Error("Blob-backed WZ input is not configured for this runtime");
    }

    static fromFile(): WzFile {
      throw new Error("File-backed WZ input is not configured for this runtime");
    }

    constructor(path: string, mapleVersion?: MapleVersionValue);
    constructor(path: string, gameVersion: number, mapleVersion: MapleVersionValue);
    constructor(path: string, iv: ArrayBufferViewLike);
    constructor(pathOrPtr: string | NativeHandle, gameVersionOrMapleVersion: number | ArrayBufferViewLike | boolean = MapleVersion.GMS, mapleVersion?: MapleVersionValue) {
      if (typeof pathOrPtr === "bigint") {
        super(pathOrPtr, gameVersionOrMapleVersion === true);
        return;
      }

      if (!capabilities.pathInput) {
        throw new Error("WZ file path input is not supported by this binding");
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

    addDisposeCallback(callback: () => void): void {
      this.disposeCallbacks.push(callback);
    }

    parseWzFile(): ParseStatusValue {
      return native.parseFile(this.nativePtr());
    }

    saveToDisk(path: string): void {
      if (!capabilities.saveToDisk) {
        throw new Error("saving WZ files to disk is not supported by this binding");
      }
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
        try {
          native.closeFile(ptr);
        } finally {
          invalidateHandles(ptrs);
          for (const callback of this.disposeCallbacks.splice(0)) callback();
        }
      }
    }
  }

  class WzDirectory extends WzObject {
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

  class WzImage extends WzObject {
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

  class WzPropertyCollection extends Array<WzImageProperty> {
    get(index: number): WzImageProperty | undefined {
      return this[index];
    }
  }

  class WzImageProperty extends WzObject {
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

  class WzNullProperty extends WzImageProperty {}

  class WzShortProperty extends WzImageProperty {
    getValue(): number {
      return native.shortValue(this.nativePtr());
    }

    setValue(value: number): void {
      native.shortSetValue(this.nativePtr(), value);
    }
  }

  class WzIntProperty extends WzImageProperty {
    getValue(): number {
      return native.intValue(this.nativePtr());
    }

    setValue(value: number): void {
      native.intSetValue(this.nativePtr(), value);
    }
  }

  class WzLongProperty extends WzImageProperty {
    getValue(): bigint {
      return native.longValue(this.nativePtr());
    }

    setValue(value: bigint): void {
      native.longSetValue(this.nativePtr(), value);
    }
  }

  class WzFloatProperty extends WzImageProperty {
    getValue(): number {
      return native.floatValue(this.nativePtr());
    }

    setValue(value: number): void {
      native.floatSetValue(this.nativePtr(), value);
    }
  }

  class WzDoubleProperty extends WzImageProperty {
    getValue(): number {
      return native.doubleValue(this.nativePtr());
    }

    setValue(value: number): void {
      native.doubleSetValue(this.nativePtr(), value);
    }
  }

  class WzStringProperty extends WzImageProperty {
    getValue(): string {
      return native.stringValue(this.nativePtr());
    }

    setValue(value: string): void {
      native.stringSetValue(this.nativePtr(), value);
    }
  }

  class WzSubProperty extends WzImageProperty {}

  class WzCanvasProperty extends WzImageProperty {
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

  class WzPngProperty extends WzImageProperty {
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

  class WzVectorProperty extends WzImageProperty {
    getX(): number {
      return native.vectorX(this.nativePtr());
    }

    getY(): number {
      return native.vectorY(this.nativePtr());
    }
  }

  class WzConvexProperty extends WzImageProperty {}

  class WzBinaryProperty extends WzImageProperty {
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

  class WzRawDataProperty extends WzImageProperty {
    getBytes(): Uint8Array {
      return native.rawData(this.nativePtr());
    }

    getRawType(): number {
      return native.rawType(this.nativePtr());
    }
  }

  class WzVideoProperty extends WzImageProperty {
    getData(): Uint8Array {
      return native.videoData(this.nativePtr());
    }
  }

  class WzUOLProperty extends WzImageProperty {
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

  class WzLuaProperty extends WzImageProperty {
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

  class WzProperty {
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

  const WzTool = Object.freeze({
    detectMapleVersion(path: string): DetectedMapleVersion {
      if (!capabilities.pathInput) {
        throw new Error("path-based WZ input is not supported in this runtime");
      }
      return native.detectMapleVersion(path);
    },
    getIvForVersion: native.ivForVersion,
  });


  return {
    MapleVersion,
    ParseStatus,
    PropertyType,
    ObjectType,
    BinaryType,
    WzObject,
    WzFile,
    WzDirectory,
    WzImage,
    WzPropertyCollection: WzPropertyCollection as unknown as WzPropertyCollectionConstructor,
    WzImageProperty,
    WzNullProperty,
    WzShortProperty,
    WzIntProperty,
    WzLongProperty,
    WzFloatProperty,
    WzDoubleProperty,
    WzStringProperty,
    WzSubProperty,
    WzCanvasProperty,
    WzPngProperty,
    WzVectorProperty,
    WzConvexProperty,
    WzBinaryProperty,
    WzRawDataProperty,
    WzVideoProperty,
    WzUOLProperty,
    WzLuaProperty,
    WzProperty,
    WzTool,
  };
}
