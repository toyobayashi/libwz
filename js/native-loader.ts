import { setWzBinding } from './binding-state.js'
import type { NativeBinding } from './native-binding.js'

interface NodePathModule {
  dirname(path: string): string;
  join(...parts: string[]): string;
}

interface NodeUrlModule {
  fileURLToPath(url: string): string;
}

type NodeRequire = (id: string) => unknown

interface ImportMetaWithDirname extends ImportMeta {
  dirname?: string;
}

interface NodeModuleBuiltin {
  createRequire(url: string): NodeRequire;
}

interface ProcessWithBuiltinModule {
  getBuiltinModule?(id: 'module'): NodeModuleBuiltin;
  getBuiltinModule?(id: 'path'): NodePathModule;
  getBuiltinModule?(id: 'url'): NodeUrlModule;
}

declare const process: ProcessWithBuiltinModule | undefined

export function installNativeBinding (): boolean {
  try {
    const native = loadNative()
    if (native === null) return false
    setWzBinding(native, {
      blobInput: false,
      pathInput: true,
      saveToDisk: true
    }, 'native')
    return true
  } catch {
    return false
  }
}

function loadNative (): NativeBinding | null {
  const moduleBuiltin = getBuiltinModule('module') as NodeModuleBuiltin | null
  const path = getBuiltinModule('path') as NodePathModule | null
  const url = getBuiltinModule('url') as NodeUrlModule | null
  if (moduleBuiltin === null || path === null || url === null) return null

  const nodeRequire = moduleBuiltin.createRequire(import.meta.url)
  const moduleDir = (import.meta as ImportMetaWithDirname).dirname
    ?? path.dirname(url.fileURLToPath(import.meta.url))
  for (const candidate of nativeCandidates(path, moduleDir)) {
    try {
      return nodeRequire(candidate) as NativeBinding
    } catch (err: unknown) {
      const { code } = (err as { code?: string })
      if (code !== 'MODULE_NOT_FOUND') throw err
    }
  }
  throw new Error('libwz native addon is not built. Run `npm run build`.')
}

function nativeCandidates (path: NodePathModule, moduleDir: string): string[] {
  return [
    path.join(moduleDir, '..', 'build', 'Release', 'wz.node'),
    path.join(moduleDir, '..', 'build', 'Release', 'libwz.node'),
    path.join(moduleDir, '..', 'build', 'Debug', 'wz.node'),
    path.join(moduleDir, '..', 'build', 'Debug', 'libwz.node')
  ]
}

function getBuiltinModule (id: string): unknown {
  if (typeof process !== 'object' || process === null) return null
  if (typeof process.getBuiltinModule !== 'function') return null
  try {
    return process.getBuiltinModule(id as 'module')
  } catch {
    return null
  }
}
