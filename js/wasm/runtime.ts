import { setWzBinding } from '../binding-state.js'
import type { BlobReadRangeCallback, NativeBinding } from '../native-binding.js'
import {
  WzFile,
  type MapleVersion
} from '../node-wrapper.js'

export interface LoadedWzModule {
  binding: NativeBinding;
  module: {
    FS?: {
      mkdirTree(path: string): void;
      mount(type: unknown, options: object, mountpoint: string): void;
    };
    NODEFS?: unknown;
    emnapiInit(options: { context: unknown; filename?: string }): unknown;
  };
}

export interface WasmRuntimeOptions {
  mount?: NodeFilesystemMount;
}

export interface NodeFilesystemMount {
  hostPath: string;
  wasmPath: string;
}

type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray

type BlobLike = Blob & {
  slice(start?: number, end?: number): Blob;
}

type FileReaderSyncConstructor = new() => {
  readAsArrayBuffer(blob: Blob): ArrayBuffer;
}

export function configureWasmBinding (
  loaded: LoadedWzModule,
  options: WasmRuntimeOptions
): void {
  configureNodeMount(loaded.module, options.mount)

  const isNode = isNodeRuntime()
  const binding = isNode ? normalizeNodePathBinding(loaded.binding) : loaded.binding
  setWzBinding(binding, {
    blobInput: true,
    pathInput: isNode,
    saveToDisk: isNode
  }, 'wasm')
  attachBlobFactories()
}

function normalizeNodePathBinding (binding: NativeBinding): NativeBinding {
  return Object.assign(Object.create(binding) as NativeBinding, {
    openFile (path: string, gameVersion: number, mapleVersion: MapleVersion) {
      return binding.openFile(toWasmNodePath(path), gameVersion, mapleVersion)
    },
    openFileWithIv (path: string, iv: ArrayBufferViewLike) {
      return binding.openFileWithIv(toWasmNodePath(path), iv)
    },
    fileSaveToDisk (ptr: bigint, path: string) {
      binding.fileSaveToDisk(ptr, toWasmNodePath(path))
    },
    detectMapleVersion (path: string) {
      return binding.detectMapleVersion(toWasmNodePath(path))
    }
  })
}

function toWasmNodePath (path: string): string {
  return path.replaceAll('\\', '/')
}

function attachBlobFactories (): void {
  WzFile.fromBlob = (
    name: string,
    blob: BlobLike,
    gameVersionOrMapleVersion: number,
    mapleVersion?: MapleVersion
  ): WzFile => openBlobSource(
    name,
    blob,
    gameVersionOrMapleVersion,
    mapleVersion
  )

  WzFile.fromBlobWithIv = (
    name: string,
    blob: BlobLike,
    iv: ArrayBufferViewLike
  ): WzFile => openBlobSource(name, blob, iv)

  WzFile.fromFile = (
    file: File,
    gameVersionOrMapleVersion: number,
    mapleVersion?: MapleVersion
  ): WzFile => mapleVersion === undefined
    ? WzFile.fromBlob(file.name, file, gameVersionOrMapleVersion)
    : WzFile.fromBlob(file.name, file, gameVersionOrMapleVersion, mapleVersion)
}

function openBlobSource (
  name: string,
  blob: BlobLike,
  gameVersionOrMapleVersionOrIv: number | ArrayBufferViewLike,
  mapleVersion?: MapleVersion
): WzFile {
  const readRange = createBlobReadRange(blob)
  if (ArrayBuffer.isView(gameVersionOrMapleVersionOrIv)) {
    return WzFile.fromBlobSource(
      name,
      blob.size,
      gameVersionOrMapleVersionOrIv,
      readRange
    )
  }
  if (mapleVersion === undefined) {
    return WzFile.fromBlobSource(
      name,
      blob.size,
      gameVersionOrMapleVersionOrIv,
      readRange
    )
  }
  return WzFile.fromBlobSource(
    name,
    blob.size,
    gameVersionOrMapleVersionOrIv,
    mapleVersion,
    readRange
  )
}

function createBlobReadRange (blob: BlobLike): BlobReadRangeCallback {
  const FileReaderSyncCtor = getFileReaderSyncConstructor()
  if (typeof FileReaderSyncCtor !== 'function') {
    throw new Error('FileReaderSync is required for synchronous Blob-backed WZ reads')
  }
  const reader = new FileReaderSyncCtor()
  return (offset: number, length: number): Uint8Array => {
    const start = Math.trunc(offset)
    if (start < 0 || start > blob.size) {
      throw new Error('Blob read offset is out of range')
    }
    const end = Math.min(start + length, blob.size)
    const bytes = new Uint8Array(reader.readAsArrayBuffer(blob.slice(start, end)))
    if (bytes.byteLength !== length) {
      throw new Error('Blob read returned an unexpected number of bytes')
    }
    return bytes
  }
}

function getFileReaderSyncConstructor (): FileReaderSyncConstructor | undefined {
  return (globalThis as {
    FileReaderSync?: FileReaderSyncConstructor;
  }).FileReaderSync
}

function configureNodeMount (
  module: LoadedWzModule['module'],
  mount: NodeFilesystemMount | undefined
): void {
  if (mount === undefined || module.FS === undefined || module.NODEFS === undefined) {
    return
  }
  module.FS.mkdirTree(mount.wasmPath)
  module.FS.mount(module.NODEFS, { root: mount.hostPath }, mount.wasmPath)
}

function isNodeRuntime (): boolean {
  const maybeProcess = (globalThis as {
    process?: { versions?: { node?: string } };
  }).process
  return typeof maybeProcess?.versions?.node === 'string'
}
