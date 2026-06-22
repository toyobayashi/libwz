export interface EmscriptenModuleOptions {
  locateFile?: (path: string, prefix: string) => string;
  wasmBinary?: Uint8Array | ArrayBuffer;
  wasmModule?: WebAssembly.Module;
  instantiateWasm?: (
    imports: WebAssembly.Imports,
    receiveInstance: (
      instance: WebAssembly.Instance,
      module: WebAssembly.Module,
    ) => void,
  ) => WebAssembly.Exports | undefined;
}

export interface LibwzEmscriptenModule {
  HEAPU8: Uint8Array;
  FS?: {
    mkdirTree(path: string): void;
    mount(type: unknown, options: object, mountpoint: string): void;
  };
  NODEFS?: unknown;
  NODERAWFS?: unknown;
  emnapiInit(options: { context: unknown; filename?: string }): unknown;
}

export default function createLibwzModule(
  options?: EmscriptenModuleOptions,
): Promise<LibwzEmscriptenModule>;
