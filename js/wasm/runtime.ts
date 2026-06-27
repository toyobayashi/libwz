import { setWzBinding } from '../binding-state.js'
import type { NativeBinding, NativeIv } from '../native-binding.js'
import type {
  MapleVersion
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

const nodeMountedRoots = new WeakMap<LoadedWzModule['module'], Map<string, string>>()

function normalizeNodePathBinding (
  binding: NativeBinding,
  pathMapper: NodePathMapper | undefined
): NativeBinding {
  return Object.defineProperties(Object.create(binding) as NativeBinding, {
    openFile: {
      value (path: string, gameVersionOrIv: number | NativeIv, mapleVersion?: MapleVersion) {
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
    canvasSaveToFile: {
      value (ptr: bigint, path: string) {
        return binding.canvasSaveToFile(ptr, toWasmNodePath(path, pathMapper))
      }
    },
    pngSaveToFile: {
      value (ptr: bigint, path: string) {
        return binding.pngSaveToFile(ptr, toWasmNodePath(path, pathMapper))
      }
    },
    binarySaveToFile: {
      value (ptr: bigint, path: string) {
        return binding.binarySaveToFile(ptr, toWasmNodePath(path, pathMapper))
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

function createNodePathMapper (module: LoadedWzModule['module']): NodePathMapper | undefined {
  if (module.FS === undefined || module.NODEFS === undefined) {
    return undefined
  }
  const nodePath = getNodePathModule()
  if (nodePath === undefined) return undefined

  let mountedRoots = nodeMountedRoots.get(module)
  if (mountedRoots === undefined) {
    mountedRoots = new Map<string, string>()
    nodeMountedRoots.set(module, mountedRoots)
  }
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
