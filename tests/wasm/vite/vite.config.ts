import { resolve } from 'node:path'
import { defineConfig } from 'vite'

const repoRoot = resolve(__dirname, '../../..')

export default defineConfig({
  build: {
    emptyOutDir: true,
    lib: {
      entry: resolve(__dirname, '../src/main.tsx'),
      fileName: 'main',
      formats: ['es']
    },
    outDir: resolve(__dirname, 'dist')
  },
  resolve: {
    alias: {
      libwz: resolve(repoRoot, 'dist/index.js')
    }
  },
  root: resolve(__dirname, '..'),
  server: {
    host: '127.0.0.1',
    port: 5173
  }
})
