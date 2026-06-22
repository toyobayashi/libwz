import {
  BinaryType,
  createWzApiFromBinding,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
  type MapleVersionValue,
  type WzApi,
  type WzFile,
} from "./node-wrapper.js";
import type { NativeBinding } from "./native-binding.js";

export {
  BinaryType,
  MapleVersion,
  ObjectType,
  ParseStatus,
  PropertyType,
};
export type { WzApi };

export type WzBinding = NativeBinding;

export interface LoadedWzModule {
  binding: WzBinding;
  module: {
    HEAPU8: Uint8Array;
    FS?: {
      mkdirTree(path: string): void;
      mount(type: unknown, options: object, mountpoint: string): void;
    };
    NODEFS?: unknown;
    emnapiInit(options: { context: unknown; filename?: string }): unknown;
  };
}

export interface WzWasmSyncOptions {
  wasmPath?: string;
  wasmBytes?: Uint8Array | ArrayBuffer;
  wasmModule?: WebAssembly.Module;
}

export interface WzApiOptions {
  wasmUrl?: string | URL;
  mount?: NodeFilesystemMount;
}

export type WzApiSyncOptions = WzWasmSyncOptions & {
  mount?: NodeFilesystemMount;
};

export interface NodeFilesystemMount {
  hostPath: string;
  wasmPath: string;
}

type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray;

type BlobSourceRecord = {
  blob: Blob;
  module: LoadedWzModule["module"];
};

type FileReaderSyncConstructor = {
  new(): {
    readAsArrayBuffer(blob: Blob): ArrayBuffer;
  };
};

type WzFileConstructorWithBlobSource = WzApi["WzFile"] & {
  fromBlobSource(
    name: string,
    id: number,
    size: number,
    mapleVersion: MapleVersionValue,
  ): WzFile;
  fromBlobSource(
    name: string,
    id: number,
    size: number,
    gameVersion: number,
    mapleVersion: MapleVersionValue,
  ): WzFile;
  fromBlobSource(
    name: string,
    id: number,
    size: number,
    iv: ArrayBufferViewLike,
  ): WzFile;
};

type WzFileWithDisposeCallback = WzFile & {
  addDisposeCallback(callback: () => void): void;
};

let nextBlobId = 1;
const blobSources = new Map<number, BlobSourceRecord>();

export function createDirectApi(
  loaded: LoadedWzModule,
  options: WzApiOptions | WzApiSyncOptions,
): WzApi {
  configureNodeMount(loaded.module, options.mount);
  installBlobRangeReader();

  const isNode = isNodeRuntime();
  const binding = isNode ? normalizeNodePathBinding(loaded.binding) : loaded.binding;
  const api = createWzApiFromBinding(binding, {
    blobInput: true,
    pathInput: isNode,
    saveToDisk: isNode,
  });
  attachBlobFactories(api, loaded.module);
  return api;
}

function normalizeNodePathBinding(binding: WzBinding): WzBinding {
  return Object.assign(Object.create(binding) as WzBinding, {
    openFile(path: string, gameVersion: number, mapleVersion: number) {
      return binding.openFile(toWasmNodePath(path), gameVersion, mapleVersion);
    },
    openFileWithIv(path: string, iv: ArrayBufferViewLike) {
      return binding.openFileWithIv(toWasmNodePath(path), iv);
    },
    fileSaveToDisk(ptr: bigint, path: string) {
      return binding.fileSaveToDisk(ptr, toWasmNodePath(path));
    },
    detectMapleVersion(path: string) {
      return binding.detectMapleVersion(toWasmNodePath(path));
    },
  });
}

function toWasmNodePath(path: string): string {
  return path.replaceAll("\\", "/");
}

function attachBlobFactories(api: WzApi, module: LoadedWzModule["module"]): void {
  const WzFileCtor = api.WzFile as WzFileConstructorWithBlobSource;

  api.WzFile.fromBlob = (
    name: string,
    blob: Blob,
    gameVersionOrMapleVersion: number,
    mapleVersion?: MapleVersionValue,
  ): WzFile => openBlobSource(
    WzFileCtor,
    module,
    name,
    blob,
    gameVersionOrMapleVersion,
    mapleVersion,
  );

  api.WzFile.fromBlobWithIv = (
    name: string,
    blob: Blob,
    iv: ArrayBufferViewLike,
  ): WzFile => openBlobSource(WzFileCtor, module, name, blob, iv);

  api.WzFile.fromFile = (
    file: File,
    gameVersionOrMapleVersion: number,
    mapleVersion?: MapleVersionValue,
  ): WzFile => mapleVersion === undefined
    ? api.WzFile.fromBlob(file.name, file, gameVersionOrMapleVersion as MapleVersionValue)
    : api.WzFile.fromBlob(file.name, file, gameVersionOrMapleVersion, mapleVersion);
}

function openBlobSource(
  WzFileCtor: WzFileConstructorWithBlobSource,
  module: LoadedWzModule["module"],
  name: string,
  blob: Blob,
  gameVersionOrMapleVersionOrIv: number | ArrayBufferViewLike,
  mapleVersion?: MapleVersionValue,
): WzFile {
  assertBlobRangeReadsSupported();

  const id = nextBlobId++;
  blobSources.set(id, { blob, module });
  let file: WzFile;
  try {
    if (ArrayBuffer.isView(gameVersionOrMapleVersionOrIv)) {
      file = WzFileCtor.fromBlobSource(name, id, blob.size, gameVersionOrMapleVersionOrIv);
    } else if (mapleVersion === undefined) {
      file = WzFileCtor.fromBlobSource(
        name,
        id,
        blob.size,
        gameVersionOrMapleVersionOrIv as MapleVersionValue,
      );
    } else {
      file = WzFileCtor.fromBlobSource(
        name,
        id,
        blob.size,
        gameVersionOrMapleVersionOrIv,
        mapleVersion,
      );
    }
  } catch (error) {
    blobSources.delete(id);
    throw error;
  }

  (file as WzFileWithDisposeCallback).addDisposeCallback(() => {
    blobSources.delete(id);
  });
  return file;
}

function assertBlobRangeReadsSupported(): void {
  const FileReaderSyncCtor = getFileReaderSyncConstructor();
  if (typeof FileReaderSyncCtor !== "function") {
    throw new Error("FileReaderSync is required for Blob-backed WZ reads");
  }
}

function installBlobRangeReader(): void {
  globalThis.__libwzReadBlobRange = (
    id: number,
    offset: number,
    destination: number,
    length: number,
  ): number => {
    const source = blobSources.get(id);
    if (source === undefined) return -1;
    const start = Math.trunc(offset);
    if (start < 0 || start > source.blob.size) return -1;
    const end = Math.min(start + length, source.blob.size);
    const FileReaderSyncCtor = getFileReaderSyncConstructor();
    if (FileReaderSyncCtor === undefined) return -1;
    const reader = new FileReaderSyncCtor();
    const bytes = new Uint8Array(
      reader.readAsArrayBuffer(source.blob.slice(start, end)),
    );
    source.module.HEAPU8.set(bytes, destination);
    return bytes.byteLength;
  };
}

function getFileReaderSyncConstructor(): FileReaderSyncConstructor | undefined {
  return (globalThis as {
    FileReaderSync?: FileReaderSyncConstructor;
  }).FileReaderSync;
}

function configureNodeMount(
  module: LoadedWzModule["module"],
  mount: NodeFilesystemMount | undefined,
): void {
  if (mount === undefined || module.FS === undefined || module.NODEFS === undefined) {
    return;
  }
  module.FS.mkdirTree(mount.wasmPath);
  module.FS.mount(module.NODEFS, { root: mount.hostPath }, mount.wasmPath);
}

function isNodeRuntime(): boolean {
  const maybeProcess = (globalThis as {
    process?: { versions?: { node?: string } };
  }).process;
  return typeof maybeProcess?.versions?.node === "string";
}

declare global {
  var __libwzReadBlobRange:
    | ((
        id: number,
        offset: number,
        destination: number,
        length: number,
      ) => number)
    | undefined;
}
