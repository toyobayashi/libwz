export type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray;
export type MapleVersionValue = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 99;
export interface BlobLike {
  readonly size: number;
  slice(start?: number, end?: number): BlobLike;
}
export interface FileLike extends BlobLike {
  readonly name: string;
}

export const MapleVersion: Readonly<{
  GMS: 0;
  EMS: 1;
  BMS: 2;
  CLASSIC: 3;
  GENERATE: 4;
  GETFROMZLZ: 5;
  CUSTOM: 6;
  UNKNOWN: 99;
}>;
export const ParseStatus: Readonly<{
  PATH_IS_NULL: -1;
  ERROR_GAME_VER_HASH: -2;
  FAILED_UNKNOWN: 0;
  SUCCESS: 1;
}>;
export const PropertyType: Readonly<Record<string, number>>;
export const ObjectType: Readonly<Record<string, number>>;
export const BinaryType: Readonly<Record<string, number>>;

export interface WzFile {
  parseWzFile(): number;
  getWzDirectory(): { getName(): string } | null;
  close(): void;
  [Symbol.dispose](): void;
}

export interface WzFileConstructor {
  new(path: string, mapleVersion?: MapleVersionValue): WzFile;
  new(path: string, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  new(path: string, iv: ArrayBufferViewLike): WzFile;
  fromBlob(name: string, blob: BlobLike, mapleVersion: MapleVersionValue): WzFile;
  fromBlob(name: string, blob: BlobLike, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
  fromBlobWithIv(name: string, blob: BlobLike, iv: ArrayBufferViewLike): WzFile;
  fromFile(file: FileLike, mapleVersion: MapleVersionValue): WzFile;
  fromFile(file: FileLike, gameVersion: number, mapleVersion: MapleVersionValue): WzFile;
}

export interface WzApi {
  MapleVersion: typeof MapleVersion;
  ParseStatus: typeof ParseStatus;
  PropertyType: typeof PropertyType;
  ObjectType: typeof ObjectType;
  BinaryType: typeof BinaryType;
  WzFile: WzFileConstructor;
  [name: string]: unknown;
}

export function createWzApiFromBinding(native: unknown, capabilities: {
  blobInput: boolean;
  pathInput: boolean;
  saveToDisk: boolean;
}): WzApi;
