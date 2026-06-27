import {
  BinaryType,
  ParseStatus,
  PropertyType,
  WzBinaryProperty,
  WzCanvasProperty,
  WzDoubleProperty,
  WzDirectory,
  WzFile,
  WzFloatProperty,
  WzImage,
  WzImageProperty,
  WzIntProperty,
  WzLongProperty,
  WzLuaProperty,
  WzPngProperty,
  WzRawDataProperty,
  WzShortProperty,
  WzStringProperty,
  WzUOLProperty,
  WzVideoProperty,
  WzVectorProperty,
  init
} from 'libwz'
import type { ExpandResult, ExportResult, MapleVersionOption, PreviewResult, TreeNodeSummary, WorkerRequest, WorkerResponse } from './messages'

interface HandleEntry {
  node: TreeNodeSummary;
  value: WzFile | WzDirectory | WzImage | WzImageProperty;
}

interface WorkerScope {
  addEventListener(type: 'message', listener: (event: MessageEvent<WorkerRequest>) => void): void;
  postMessage(message: WorkerResponse, transfer?: Transferable[]): void;
}

const handles = new Map<number, HandleEntry>()
const workerScope = self as unknown as WorkerScope
const maxPreviewPixels = 4096 * 4096
let nextNodeId = 0
let ready: Promise<void> | null = null

workerScope.addEventListener('message', (event: MessageEvent<WorkerRequest>) => {
  void handleMessage(event.data)
})

async function handleMessage (message: WorkerRequest): Promise<void> {
  try {
    const result = await dispatch(message)
    const response: WorkerResponse = { id: message.id, ok: true, result }
    workerScope.postMessage(response, transferables(result))
  } catch (error) {
    const response: WorkerResponse = {
      error: messageFromError(error),
      id: message.id,
      ok: false
    }
    workerScope.postMessage(response)
  }
}

function messageFromError (error: unknown): string {
  const message = error instanceof Error ? error.message : String(error)
  if (message.includes('Cannot enlarge memory')) {
    return `${message} This usually means the WZ was decoded with the wrong Maple version/IV or the image data is corrupt. Try another Version option before opening the file again.`
  }
  return message
}

async function dispatch (message: WorkerRequest): Promise<TreeNodeSummary | ExpandResult | ExportResult | PreviewResult> {
  await ensureReady()
  if (message.type === 'open') return await openFile(message.file, message.mapleVersion)
  if (message.type === 'expand') return expandNode(message.nodeId)
  if (message.type === 'export') return await exportNode(message.nodeId, message.format)
  if (message.type === 'preview') return await previewNode(message.nodeId)
  if (message.type === 'unload') return unloadFile(message.nodeId)
  return exhaustive(message)
}

async function ensureReady (): Promise<void> {
  ready ??= init()
  await ready
}

async function openFile (file: File, mapleVersion: MapleVersionOption): Promise<TreeNodeSummary> {
  const readRange = createFileReadRange(file)
  const wzFile = WzFile.fromBlobSource(file.name, file.size, mapleVersion, readRange)
  const status = wzFile.parseWzFile()
  if (status !== ParseStatus.SUCCESS) {
    wzFile.close()
    throw new Error(`Failed to parse WZ file: status ${status}`)
  }

  return remember(wzFile, null)
}

function createFileReadRange (file: File): (offset: number, length: number, destination?: Uint8Array) => Uint8Array | undefined {
  const reader = new FileReaderSync()
  return (offset: number, length: number, destination?: Uint8Array): Uint8Array | undefined => {
    const start = Math.trunc(offset)
    if (start < 0 || start > file.size) {
      throw new Error('File read offset is out of range')
    }
    const end = Math.min(start + Math.trunc(length), file.size)
    const bytes = new Uint8Array(reader.readAsArrayBuffer(file.slice(start, end)))
    if (bytes.byteLength !== length) {
      throw new Error('File read returned an unexpected number of bytes')
    }
    if (destination !== undefined) {
      destination.set(bytes)
      return
    }
    return bytes
  }
}

function unloadFile (nodeId: number): TreeNodeSummary {
  const entry = requireEntry(nodeId)
  if (!(entry.value instanceof WzFile)) {
    throw new Error('Only a WZ file root node can be unloaded')
  }
  const unloaded = {
    ...entry.node,
    actions: [],
    expandable: false,
    meta: 'unloaded'
  }
  entry.value.close()
  deleteHandleSubtree(nodeId)
  return unloaded
}

function expandNode (nodeId: number): ExpandResult {
  const entry = requireEntry(nodeId)
  const value = entry.value
  let children: TreeNodeSummary[]

  if (value instanceof WzFile) {
    const root = value.getWzDirectory()
    children = root === null ? [] : directoryChildren(root, nodeId)
  } else if (value instanceof WzDirectory) {
    children = directoryChildren(value, nodeId)
  } else if (value instanceof WzImage) {
    if (!value.isParsed()) value.parseImage()
    children = value.wzProperties().map((property: WzImageProperty) => remember(property, nodeId))
  } else if (value instanceof WzImageProperty) {
    children = value.wzProperties().map((property: WzImageProperty) => remember(property, nodeId))
  } else {
    children = []
  }

  entry.node = describeNode(value, nodeId, entry.node.parentId)
  return {
    children,
    node: entry.node
  }
}

async function exportNode (nodeId: number, format: 'png' | 'mp3'): Promise<ExportResult> {
  const entry = requireEntry(nodeId)

  if (format === 'png') {
    const png = getPngProperty(entry.value)
    if (png === null) throw new Error('Selected node does not contain PNG data')
    assertReasonablePngSize(png)
    return {
      bytes: await encodePng(png),
      mime: 'image/png',
      name: `${safeName(entry.node.name)}.png`
    }
  }

  if (!(entry.value instanceof WzBinaryProperty)) {
    throw new Error('Selected node is not a binary property')
  }

  return {
    bytes: entry.value.getBytes(),
    mime: 'audio/mpeg',
    name: `${safeName(entry.node.name)}.mp3`
  }
}

async function previewNode (nodeId: number): Promise<PreviewResult> {
  const entry = requireEntry(nodeId)
  const png = getPngProperty(entry.value)
  if (png !== null) {
    assertReasonablePngSize(png)
    const pixels = rgbaPixels(png)
    return {
      alpha: pixels.alpha,
      height: png.getHeight(),
      kind: 'image',
      name: entry.node.name,
      pixels: pixels.data,
      rgb: pixels.rgb,
      width: png.getWidth()
    }
  }

  if (entry.value instanceof WzBinaryProperty && entry.value.getType() === BinaryType.MP3) {
    return {
      bytes: entry.value.getBytes(),
      kind: 'audio',
      mime: 'audio/mpeg',
      name: entry.node.name
    }
  }

  throw new Error('Selected node has no previewable content')
}

function directoryChildren (directory: WzDirectory, parentId: number): TreeNodeSummary[] {
  return [
    ...directory.wzDirectories().map((dir: WzDirectory | null) => rememberRequired(dir, parentId)),
    ...directory.wzImages().map((image: WzImage | null) => rememberRequired(image, parentId))
  ]
}

function rememberRequired (
  value: WzDirectory | WzImage | null,
  parentId: number
): TreeNodeSummary {
  if (value === null) throw new Error('WZ tree contained an empty child')
  return remember(value, parentId)
}

function remember (
  value: WzFile | WzDirectory | WzImage | WzImageProperty,
  parentId: number | null
): TreeNodeSummary {
  nextNodeId += 1
  const node = describeNode(value, nextNodeId, parentId)
  handles.set(nextNodeId, { value, node })
  return node
}

function describeNode (
  value: WzFile | WzDirectory | WzImage | WzImageProperty,
  id: number,
  parentId: number | null
): TreeNodeSummary {
  if (value instanceof WzFile) {
    return node(id, parentId, value.getName(), displayPath(value), 'file', 'WZ file', true, ['unload'])
  }
  if (value instanceof WzDirectory) {
    return node(id, parentId, value.getName(), displayPath(value), 'directory', 'Directory', true, [])
  }
  if (value instanceof WzImage) {
    return node(id, parentId, value.getName(), displayPath(value), 'image', imageMeta(value), true, [])
  }
  return propertyNode(value, id, parentId)
}

function propertyNode (
  property: WzImageProperty,
  id: number,
  parentId: number | null
): TreeNodeSummary {
  const type = property.getPropertyType()
  const actions: TreeNodeSummary['actions'] = []
  const expandable = hasChildren(property)
  let meta = propertyTypeName(type)
  let kind: TreeNodeSummary['kind'] = 'property'

  if (type === PropertyType.CANVAS && property instanceof WzCanvasProperty) {
    kind = 'canvas'
    const png = property.getPngProperty()
    if (png !== null) {
      meta = `Canvas ${png.getWidth()}x${png.getHeight()}`
      actions.push('png')
    }
  } else if (type === PropertyType.PNG && property instanceof WzPngProperty) {
    kind = 'canvas'
    meta = `PNG ${property.getWidth()}x${property.getHeight()}`
    actions.push('png')
  } else if (type === PropertyType.SOUND && property instanceof WzBinaryProperty) {
    kind = 'binary'
    meta = binaryMeta(property)
    if (property.getType() === BinaryType.MP3) actions.push('mp3')
  } else if (type === PropertyType.STRING && property instanceof WzStringProperty) {
    meta = `String "${previewText(property.getValue())}"`
  } else if (type === PropertyType.UOL && property instanceof WzUOLProperty) {
    meta = `UOL -> ${property.getValue()}`
  } else if (type === PropertyType.VECTOR && property instanceof WzVectorProperty) {
    meta = `Vector (${property.getX()}, ${property.getY()})`
  } else {
    meta = propertyValueMeta(property)
  }

  return node(id, parentId, property.getName(), displayPath(property), kind, meta, expandable, actions)
}

function propertyValueMeta (property: WzImageProperty): string {
  const type = property.getPropertyType()
  if (property instanceof WzShortProperty) return `Short ${property.getValue()}`
  if (property instanceof WzIntProperty) return `Int ${property.getValue()}`
  if (property instanceof WzLongProperty) return `Long ${property.getValue().toString()}`
  if (property instanceof WzFloatProperty) return `Float ${formatNumber(property.getValue())}`
  if (property instanceof WzDoubleProperty) return `Double ${formatNumber(property.getValue())}`
  if (property instanceof WzLuaProperty) return `Lua "${previewText(property.getString())}"`
  if (property instanceof WzRawDataProperty) return `Raw type ${property.getRawType()}, ${property.getBytes().byteLength} bytes`
  if (property instanceof WzVideoProperty) return `Video, ${property.getData().byteLength} bytes`
  return propertyTypeName(type)
}

function formatNumber (value: number): string {
  return Number.isInteger(value) ? value.toString() : value.toPrecision(8).replace(/\.?0+$/, '')
}

function previewText (value: string): string {
  const normalized = value.replace(/\s+/g, ' ')
  return normalized.length > 80 ? `${normalized.slice(0, 77)}...` : normalized
}

function deleteHandleSubtree (nodeId: number): void {
  const ids = new Set<number>([nodeId])
  let changed = true
  while (changed) {
    changed = false
    for (const [id, entry] of handles) {
      if (!ids.has(id) && entry.node.parentId !== null && ids.has(entry.node.parentId)) {
        ids.add(id)
        changed = true
      }
    }
  }
  for (const id of ids) handles.delete(id)
}

function normalizePath (path: string): string {
  return path.replace(/\\/g, '/')
}

function displayPath (value: WzFile | WzDirectory | WzImage | WzImageProperty): string {
  const path = normalizePath(value.getFullPath())
  return path.length === 0 ? value.getName() : path
}

function node (
  id: number,
  parentId: number | null,
  name: string,
  fullPath: string,
  kind: TreeNodeSummary['kind'],
  meta: string,
  expandable: boolean,
  actions: TreeNodeSummary['actions']
): TreeNodeSummary {
  return {
    actions,
    expandable,
    fullPath,
    id,
    kind,
    meta,
    name,
    parentId
  }
}

function imageMeta (image: WzImage): string {
  const parsed = image.isParsed() ? 'parsed' : 'not parsed'
  return `Image, ${parsed}, ${image.getBlockSize()} bytes`
}

function binaryMeta (property: WzBinaryProperty): string {
  const type = property.getType() === BinaryType.MP3
    ? 'MP3'
    : property.getType() === BinaryType.WAV ? 'WAV' : 'Binary'
  return `${type}, ${property.getLength()} ms, ${property.getFrequency()} Hz`
}

function hasChildren (property: WzImageProperty): boolean {
  try {
    return property.wzProperties().length > 0
  } catch {
    return false
  }
}

function getPngProperty (value: HandleEntry['value']): WzPngProperty | null {
  if (value instanceof WzPngProperty) return value
  if (value instanceof WzCanvasProperty) return value.getPngProperty()
  return null
}

function requireEntry (nodeId: number): HandleEntry {
  const entry = handles.get(nodeId)
  if (entry === undefined) throw new Error(`Unknown node id: ${nodeId}`)
  return entry
}

function propertyTypeName (type: PropertyType): string {
  if (type === PropertyType.NULL) return 'Null'
  if (type === PropertyType.SHORT) return 'Short'
  if (type === PropertyType.INT) return 'Int'
  if (type === PropertyType.LONG) return 'Long'
  if (type === PropertyType.FLOAT) return 'Float'
  if (type === PropertyType.DOUBLE) return 'Double'
  if (type === PropertyType.STRING) return 'String'
  if (type === PropertyType.SUB) return 'SubProperty'
  if (type === PropertyType.CANVAS) return 'Canvas'
  if (type === PropertyType.VECTOR) return 'Vector'
  if (type === PropertyType.CONVEX) return 'Convex'
  if (type === PropertyType.SOUND) return 'Binary'
  if (type === PropertyType.RAW) return 'Raw'
  if (type === PropertyType.UOL) return 'UOL'
  if (type === PropertyType.LUA) return 'Lua'
  if (type === PropertyType.PNG) return 'PNG'
  return 'Property'
}

function rgbaPixels (png: WzPngProperty): {
  alpha: { max: number; nonZero: number };
  data: Uint8ClampedArray<ArrayBuffer>;
  rgb: { nonZero: number };
} {
  const bgra = png.getImage()
  const rgba = new Uint8ClampedArray(bgra.byteLength)
  let alphaMax = 0
  let alphaNonZero = 0
  let rgbNonZero = 0
  for (let i = 0; i < bgra.byteLength; i += 4) {
    rgba[i] = bgra[i + 2]
    rgba[i + 1] = bgra[i + 1]
    rgba[i + 2] = bgra[i]
    rgba[i + 3] = bgra[i + 3]
    if (rgba[i + 3] !== 0) alphaNonZero += 1
    if (rgba[i + 3] > alphaMax) alphaMax = rgba[i + 3]
    if (rgba[i] !== 0 || rgba[i + 1] !== 0 || rgba[i + 2] !== 0) rgbNonZero += 1
  }
  if (alphaNonZero === 0) {
    for (let i = 3; i < rgba.byteLength; i += 4) rgba[i] = 255
  }
  return {
    alpha: {
      max: alphaMax,
      nonZero: alphaNonZero
    },
    data: rgba,
    rgb: {
      nonZero: rgbNonZero
    }
  }
}

function assertReasonablePngSize (png: WzPngProperty): void {
  const width = png.getWidth()
  const height = png.getHeight()
  if (width <= 0 || height <= 0) {
    throw new Error(`Invalid image dimensions: ${width}x${height}`)
  }
  const pixels = width * height
  if (!Number.isSafeInteger(pixels) || pixels > maxPreviewPixels) {
    throw new Error(`Image is too large to preview/export in the browser: ${width}x${height}`)
  }
}

async function encodePng (png: WzPngProperty): Promise<Uint8Array> {
  const width = png.getWidth()
  const height = png.getHeight()
  const canvas = new OffscreenCanvas(width, height)
  const context = canvas.getContext('2d')
  if (context === null) throw new Error('Could not create PNG encoder canvas')
  context.putImageData(new ImageData(rgbaPixels(png).data, width, height), 0, 0)
  const blob = await canvas.convertToBlob({ type: 'image/png' })
  return new Uint8Array(await blob.arrayBuffer())
}

function safeName (name: string): string {
  return name.replace(/[^\w.-]+/g, '_') || 'wz-export'
}

function transferables (result: TreeNodeSummary | ExpandResult | ExportResult | PreviewResult): Transferable[] {
  if ('bytes' in result) return [result.bytes.buffer as ArrayBuffer]
  if ('pixels' in result) return [result.pixels.buffer]
  return []
}

function exhaustive (value: never): never {
  throw new Error(`Unexpected request: ${JSON.stringify(value)}`)
}
