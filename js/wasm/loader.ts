import type { LoadedWzModule } from './runtime.js'
import type { NativeBinding } from '../native-binding.js'

interface LibwzEmscriptenModule {
  FS?: LoadedWzModule['module']['FS'];
  HEAPU8: Uint8Array;
  NODEFS?: unknown;
  emnapiInit(options: { context: unknown; filename?: string }): unknown;
}

type CreateLibwzModule = (options: {
  locateFile(path: string, prefix: string): string;
}) => Promise<LibwzEmscriptenModule>

interface EmnapiRuntimeModule {
  getDefaultContext(): unknown;
}

const asyncCache = new Map<string, Promise<LoadedWzModule>>()

export async function initializeWasm (wasmUrl: string | URL): Promise<LoadedWzModule> {
  const resolvedWasmUrl = wasmUrl.toString()
  const cached = asyncCache.get(resolvedWasmUrl)
  if (cached !== undefined) return await cached

  const loaded = loadWasmDependencies().then(async ({ createLibwzModule, getDefaultContext }) => {
    const module = await createLibwzModule({
      locateFile (path, prefix) {
        if (path.endsWith('.wasm')) {
          return resolvedWasmUrl
        }
        return `${prefix}${path}`
      }
    })
    return {
      binding: module.emnapiInit({
        context: getDefaultContext(),
        filename: 'libwz.wasm'
      }) as NativeBinding,
      module
    }
  })
  asyncCache.set(resolvedWasmUrl, loaded)
  return await loaded
}

export async function loadWzModule (): Promise<LoadedWzModule> {
  return await initializeWasm(defaultWasmUrl())
}

function defaultWasmUrl (): URL {
  return new URL('./libwz.wasm', import.meta.url)
}

async function loadWasmDependencies (): Promise<{
  createLibwzModule: CreateLibwzModule;
  getDefaultContext: EmnapiRuntimeModule['getDefaultContext'];
}> {
  const [
    runtime,
    generated
  ] = await Promise.all([
    import('@emnapi/runtime') as Promise<EmnapiRuntimeModule>,
    importGeneratedLibwzModule()
  ])
  return {
    createLibwzModule: generated,
    getDefaultContext: () => runtime.getDefaultContext()
  }
}

async function importGeneratedLibwzModule (): Promise<CreateLibwzModule> {
  // @ts-expect-error Emscripten generates this module during build:wasm.
  const generated = await import('./libwz.js') as { default: unknown }
  return generated.default as CreateLibwzModule
}
