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

export type WasmRuntimeOptions = unknown

type ArrayBufferViewLike = Uint8Array | Int8Array | Uint8ClampedArray

type BlobLike = Blob & {
  slice(start?: number, end?: number): Blob;
}

type FileReaderSyncConstructor = new() => {
  readAsArrayBuffer(blob: Blob): ArrayBuffer;
}

export function configureWasmBinding (
  loaded: LoadedWzModule,
  _options: WasmRuntimeOptions
): void {
  const isNode = isNodeRuntime()
  const pathMapper = isNode ? createNodePathMapper(loaded.module) : undefined
  const binding = isNode ? normalizeNodePathBinding(loaded.binding, pathMapper) : loaded.binding
  setWzBinding(binding, {
    blobInput: true,
    pathInput: isNode,
    saveToDisk: isNode
  }, 'wasm')
  attachBlobFactories()
}

interface NodePathModule {
  parse(path: string): { root: string };
  resolve(path: string): string;
}

interface NodeProcess {
  cwd(): string;
  getBuiltinModule?(id: 'path'): NodePathModule | undefined;
}

interface NodePathMapper {
  toWasmPath(path: string): string;
}

function normalizeNodePathBinding (
  binding: NativeBinding,
  pathMapper: NodePathMapper | undefined
): NativeBinding {
  return Object.defineProperties(Object.create(binding) as NativeBinding, {
    openFile: {
      value (path: string, gameVersionOrIv: number | ArrayBufferViewLike, mapleVersion?: MapleVersion) {
        const wasmPath = toWasmNodePath(path, pathMapper)
        if (ArrayBuffer.isView(gameVersionOrIv)) return binding.openFile(wasmPath, gameVersionOrIv)
        if (mapleVersion === undefined) throw new TypeError('mapleVersion is required')
        return binding.openFile(wasmPath, gameVersionOrIv, mapleVersion)
      }
    },
    fileSaveToDisk: {
      value (ptr: bigint, path: string) {
        binding.fileSaveToDisk(ptr, toWasmNodePath(path, pathMapper))
      }
    },
    detectMapleVersion: {
      value (path: string) {
        return binding.detectMapleVersion(toWasmNodePath(path, pathMapper))
      }
    }
  })
}

function toWasmNodePath (
  path: string,
  pathMapper: NodePathMapper | undefined
): string {
  return pathMapper?.toWasmPath(path) ?? normalizePath(path)
}

function normalizePath (path: string): string {
  const normalizedPath = path.replaceAll('\\', '/')
  if (normalizedPath.length > 1 && normalizedPath.endsWith('/')) {
    return normalizedPath.replace(/\/+$/u, '')
  }
  return normalizedPath
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

function createNodePathMapper (module: LoadedWzModule['module']): NodePathMapper | undefined {
  if (module.FS === undefined || module.NODEFS === undefined) {
    return undefined
  }
  const nodePath = getNodePathModule()
  if (nodePath === undefined) return undefined

  const mountedRoots = new Map<string, string>()
  const mountRoot = '/mnt'
  return {
    toWasmPath (path: string): string {
      const hostPath = normalizePath(nodePath.resolve(path))
      const hostRoot = normalizePath(nodePath.parse(hostPath).root)
      let wasmRoot = mountedRoots.get(hostRoot)
      if (wasmRoot === undefined) {
        wasmRoot = hostRoot === '/'
          ? mountRoot
          : `${mountRoot}/${hostRoot.replaceAll(':', '').replace(/^\/+|\/+$/gu, '')}`
        module.FS?.mkdirTree(wasmRoot)
        module.FS?.mount(module.NODEFS, { root: hostRoot }, wasmRoot)
        mountedRoots.set(hostRoot, wasmRoot)
      }
      if (hostPath === hostRoot) return wasmRoot
      const suffix = hostPath.slice(hostRoot.length).replace(/^\/+/u, '')
      return suffix.length === 0 ? wasmRoot : `${wasmRoot}/${suffix}`
    }
  }
}

function isNodeRuntime (): boolean {
  const maybeProcess = (globalThis as {
    process?: { versions?: { node?: string } };
  }).process
  return typeof maybeProcess?.versions?.node === 'string'
}

function getNodePathModule (): NodePathModule | undefined {
  const maybeProcess = (globalThis as { process?: NodeProcess }).process
  if (typeof maybeProcess?.getBuiltinModule === 'function') {
    return maybeProcess.getBuiltinModule('path')
  }
  return undefined
}
