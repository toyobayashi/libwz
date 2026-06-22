export interface EmscriptenModuleOptions {
  locateFile?: (path: string, prefix: string) => string;
}

export interface LibwzEmscriptenModule {
  HEAPU8: Uint8Array;
  emnapiInit(options: { context: unknown; filename?: string }): unknown;
}

export default function createLibwzModule(
  options?: EmscriptenModuleOptions,
): Promise<LibwzEmscriptenModule>;
