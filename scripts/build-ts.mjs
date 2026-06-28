import typescript from '@rollup/plugin-typescript'
import { spawnSync } from 'node:child_process'
import { existsSync, mkdirSync, readFileSync, readdirSync, renameSync, rmSync, writeFileSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { rollup } from 'rollup'
import dts from 'rollup-plugin-dts'

const dist = 'dist'
const dtsDir = 'build/ts-dts'
const tempTag = `${process.pid}-${Date.now()}`

mkdirSync(dist, { recursive: true })
promoteLegacyWasmArtifacts()
rmSync(dtsDir, { force: true, recursive: true })
rmSync(join(dist, 'wasm'), { force: true, recursive: true })

await bundleJs({
  external: [
    '@emnapi/runtime',
    './libwz.js'
  ],
  input: 'js/index.ts',
  output: join(dist, 'index.js'),
  tsconfig: 'tsconfig.json'
})
patchFlattenedWasmImports(join(dist, 'index.js'))

const tsc = spawnSync(
  process.execPath,
  [
    'node_modules/typescript/bin/tsc',
    '-p',
    'tsconfig.json',
    '--emitDeclarationOnly',
    '--declaration',
    '--declarationMap',
    'false',
    '--outDir',
    dtsDir
  ],
  { stdio: 'inherit' }
)
if (tsc.status !== 0) process.exit(tsc.status ?? 1)

const dtsInput = join(dtsDir, 'index.d.ts')
if (!existsSync(dtsInput)) {
  throw new Error(`Declaration entry was not generated: ${dtsInput}`)
}

await bundleDts(dtsInput, join(dist, 'index.d.ts'))

rmSync(dtsDir, { force: true, recursive: true })
for (const entry of readdirSync(dist)) {
  if (entry === 'index.js' || entry === 'index.d.ts' || entry === 'libwz.js' || entry === 'libwz.wasm') continue
  if (entry.endsWith('.js') || entry.endsWith('.d.ts') || entry.endsWith('.map')) {
    rmSync(join(dist, entry), { force: true, recursive: true })
  }
}

async function bundleJs ({
  external,
  input,
  output,
  tsconfig
}) {
  const tempOutput = tempPath(output, '.tmp.js')
  mkdirSync(dirname(output), { recursive: true })
  const bundle = await rollup({
    external,
    input,
    plugins: [
      typescript({
        compilerOptions: {
          declaration: false,
          declarationMap: false,
          outDir: undefined
        },
        tsconfig
      })
    ]
  })
  await bundle.write({
    file: tempOutput,
    format: 'es'
  })
  await bundle.close()
  renameSync(tempOutput, output)
}

async function bundleDts (input, output) {
  const tempOutput = tempPath(output, '.tmp.d.ts')
  mkdirSync(dirname(output), { recursive: true })
  const bundle = await rollup({
    external: ['@emnapi/runtime', './libwz.js'],
    input,
    plugins: [dts()]
  })
  await bundle.write({
    file: tempOutput,
    format: 'es'
  })
  await bundle.close()
  renameSync(tempOutput, output)
}

function tempPath (output, suffix) {
  return join(dirname(output), `${output.split('/').pop()}.${tempTag}${suffix}`)
}

function promoteLegacyWasmArtifacts () {
  const legacyWasmDist = join(dist, 'wasm')
  for (const file of ['libwz.js', 'libwz.wasm']) {
    const from = join(legacyWasmDist, file)
    const to = join(dist, file)
    if (existsSync(from) && !existsSync(to)) renameSync(from, to)
  }
}

function patchFlattenedWasmImports (file) {
  const original = readFileSync(file, 'utf8')
  const patched = original.replaceAll('./wasm/libwz.js', './libwz.js')
  if (patched !== original) writeFileSync(file, patched)
}
