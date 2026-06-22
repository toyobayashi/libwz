import type { ArrayBufferViewLike, DetectedMapleVersion, NativeBinding, NativeHandle } from "./native-binding.js";
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
export declare const MapleVersion: Readonly<{
    readonly GMS: 0;
    readonly EMS: 1;
    readonly BMS: 2;
    readonly CLASSIC: 3;
    readonly GENERATE: 4;
    readonly GETFROMZLZ: 5;
    readonly CUSTOM: 6;
    readonly UNKNOWN: 99;
}>;
export type MapleVersionValue = (typeof MapleVersion)[keyof typeof MapleVersion];
export declare const ParseStatus: Readonly<{
    readonly PATH_IS_NULL: -1;
    readonly ERROR_GAME_VER_HASH: -2;
    readonly FAILED_UNKNOWN: 0;
    readonly SUCCESS: 1;
}>;
export type ParseStatusValue = (typeof ParseStatus)[keyof typeof ParseStatus];
export declare const PropertyType: Readonly<{
    readonly NULL: 0;
    readonly SHORT: 1;
    readonly INT: 2;
    readonly LONG: 3;
    readonly FLOAT: 4;
    readonly DOUBLE: 5;
    readonly STRING: 6;
    readonly SUB: 7;
    readonly CANVAS: 8;
    readonly VECTOR: 9;
    readonly CONVEX: 10;
    readonly SOUND: 11;
    readonly RAW: 12;
    readonly UOL: 13;
    readonly LUA: 14;
    readonly PNG: 15;
}>;
export type PropertyTypeValue = (typeof PropertyType)[keyof typeof PropertyType];
export declare const ObjectType: Readonly<{
    readonly FILE: 0;
    readonly IMAGE: 1;
    readonly DIRECTORY: 2;
    readonly PROPERTY: 3;
    readonly LIST: 4;
}>;
export type ObjectTypeValue = (typeof ObjectType)[keyof typeof ObjectType];
export declare const BinaryType: Readonly<{
    readonly RAW: 0;
    readonly MP3: 1;
    readonly WAV: 2;
}>;
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
    new (ptr: NativeHandle, ownsNative?: boolean): WzObject;
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
    new (path: string, mapleVersion?: MapleVersionValue): WzFile;
    new (path: string, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
    new (path: string, iv: ArrayBufferViewLike): WzFile;
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
    new (ptr: NativeHandle, ownsNative?: boolean): WzDirectory;
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
    new (ptr: NativeHandle, ownsNative?: boolean): WzImage;
    readonly prototype: WzImage;
}
export interface WzPropertyCollection extends Array<WzImageProperty> {
    get(index: number): WzImageProperty | undefined;
}
export interface WzPropertyCollectionConstructor extends Function {
    new (arrayLength: number): WzPropertyCollection;
    new (...items: WzImageProperty[]): WzPropertyCollection;
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
    new (ptr: NativeHandle, ownsNative?: boolean): WzImageProperty;
    readonly prototype: WzImageProperty;
}
export interface WzNullProperty extends WzImageProperty {
}
export interface WzNullPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzNullProperty;
    readonly prototype: WzNullProperty;
}
export interface WzShortProperty extends WzImageProperty {
    getValue(): number;
    setValue(value: number): void;
}
export interface WzShortPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzShortProperty;
    readonly prototype: WzShortProperty;
}
export interface WzIntProperty extends WzImageProperty {
    getValue(): number;
    setValue(value: number): void;
}
export interface WzIntPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzIntProperty;
    readonly prototype: WzIntProperty;
}
export interface WzLongProperty extends WzImageProperty {
    getValue(): bigint;
    setValue(value: bigint): void;
}
export interface WzLongPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzLongProperty;
    readonly prototype: WzLongProperty;
}
export interface WzFloatProperty extends WzImageProperty {
    getValue(): number;
    setValue(value: number): void;
}
export interface WzFloatPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzFloatProperty;
    readonly prototype: WzFloatProperty;
}
export interface WzDoubleProperty extends WzImageProperty {
    getValue(): number;
    setValue(value: number): void;
}
export interface WzDoublePropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzDoubleProperty;
    readonly prototype: WzDoubleProperty;
}
export interface WzStringProperty extends WzImageProperty {
    getValue(): string;
    setValue(value: string): void;
}
export interface WzStringPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzStringProperty;
    readonly prototype: WzStringProperty;
}
export interface WzSubProperty extends WzImageProperty {
}
export interface WzSubPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzSubProperty;
    readonly prototype: WzSubProperty;
}
export interface WzCanvasProperty extends WzImageProperty {
    getPngProperty(): WzPngProperty | null;
    containsInlinkProperty(): boolean;
    containsOutlinkProperty(): boolean;
    getLinkedWzImageProperty(): WzImageProperty | null;
}
export interface WzCanvasPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzCanvasProperty;
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
    new (ptr: NativeHandle, ownsNative?: boolean): WzPngProperty;
    readonly prototype: WzPngProperty;
}
export interface WzVectorProperty extends WzImageProperty {
    getX(): number;
    getY(): number;
}
export interface WzVectorPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzVectorProperty;
    readonly prototype: WzVectorProperty;
}
export interface WzConvexProperty extends WzImageProperty {
}
export interface WzConvexPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzConvexProperty;
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
    new (ptr: NativeHandle, ownsNative?: boolean): WzBinaryProperty;
    readonly prototype: WzBinaryProperty;
}
export interface WzRawDataProperty extends WzImageProperty {
    getBytes(): Uint8Array;
    getRawType(): number;
}
export interface WzRawDataPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzRawDataProperty;
    readonly prototype: WzRawDataProperty;
}
export interface WzVideoProperty extends WzImageProperty {
    getData(): Uint8Array;
}
export interface WzVideoPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzVideoProperty;
    readonly prototype: WzVideoProperty;
}
export interface WzUOLProperty extends WzImageProperty {
    getValue(): string;
    setValue(value: string): void;
    getLinkValue(): WzObject | null;
}
export interface WzUOLPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzUOLProperty;
    readonly prototype: WzUOLProperty;
}
export interface WzLuaProperty extends WzImageProperty {
    getData(): Uint8Array;
    getString(): string;
}
export interface WzLuaPropertyConstructor extends Function {
    new (ptr: NativeHandle, ownsNative?: boolean): WzLuaProperty;
    readonly prototype: WzLuaProperty;
}
export interface WzProperty {
}
export interface WzPropertyConstructor extends Function {
    new (): WzProperty;
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
export declare function createWzApiFromBinding(native: NativeBinding, capabilities: WzApiCapabilities): WzApi;
