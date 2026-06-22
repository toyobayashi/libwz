import { resolve } from "node:path";
export default {
  mode: "production",
  target: "web",
  entry: "./src/index.mjs",
  output: {
    clean: true,
    filename: "bundle.js",
    library: {
      type: "module",
    },
    path: resolve(import.meta.dirname, "dist"),
  },
  experiments: {
    outputModule: true,
  },
};
