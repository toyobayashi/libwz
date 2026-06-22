import { resolve } from "node:path";
import { defineConfig } from "vite";

export default defineConfig({
  build: {
    emptyOutDir: true,
    lib: {
      entry: resolve(__dirname, "src/main.ts"),
      fileName: "main",
      formats: ["es"],
    },
    outDir: "dist",
  },
});
