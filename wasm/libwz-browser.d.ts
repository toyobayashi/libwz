export {
  type EmscriptenModuleOptions,
  type LibwzEmscriptenModule,
};

import type {
  EmscriptenModuleOptions,
  LibwzEmscriptenModule,
} from "./libwz.js";

export default function createLibwzModule(
  options?: EmscriptenModuleOptions,
): Promise<LibwzEmscriptenModule>;
