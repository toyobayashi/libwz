import React, { useMemo, useRef, useState } from 'react'
import { createRoot } from 'react-dom/client'
// @ts-expect-error Bundlers load this stylesheet as text.
import cssText from './style.css?raw'
import type {
  ExpandResult,
  ExportResult,
  PreviewResult,
  MapleVersionOption,
  TreeNodeSummary,
  WorkerRequest,
  WorkerRequestPayload,
  WorkerResponse
} from './messages'

interface NodeState extends TreeNodeSummary {
  children: TreeNodeSummary[];
  error?: string;
  loaded: boolean;
  loading: boolean;
}

interface PendingRequest<T> {
  reject(error: Error): void;
  resolve(value: T): void;
}

type WorkerResult = TreeNodeSummary | ExpandResult | ExportResult | PreviewResult

type PreviewState
  = | { kind: 'empty' }
  | { kind: 'message'; text: string }
  | {
    alpha: { max: number; nonZero: number };
    height: number;
    kind: 'image';
    name: string;
    pixels: Uint8ClampedArray<ArrayBuffer>;
    rgb: { nonZero: number };
    width: number;
  }
  | { kind: 'audio'; mime: string; name: string; url: string }

const mapleVersionOptions: Array<{ label: string; value: MapleVersionOption }> = [
  { label: 'GMS (old)', value: 0 },
  { label: 'MSEA / EMS / Taiwan (old)', value: 1 },
  { label: 'BMS / GMS / MapleSEA / JMS / KMS', value: 2 },
  { label: 'Custom encryption key', value: 6 }
]

document.head.append(styleElement(cssText))

function App (): React.JSX.Element {
  const worker = useMemo(() => new Worker(new URL('./worker.ts', import.meta.url), { type: 'module' }), [])
  const nextRequestId = useRef(0)
  const pending = useRef(new Map<number, PendingRequest<WorkerResult>>())
  const [status, setStatus] = useState('Choose a .wz file to inspect it in a worker.')
  const [errorStatus, setErrorStatus] = useState(false)
  const [expanded, setExpanded] = useState<ReadonlySet<number>>(new Set())
  const [nodes, setNodes] = useState<ReadonlyMap<number, NodeState>>(new Map())
  const [selectedNodeId, setSelectedNodeId] = useState<number | null>(null)
  const [mapleVersion, setMapleVersion] = useState<MapleVersionOption>(0)
  const [preview, setPreview] = useState<PreviewState>({ kind: 'empty' })
  const canvasRef = useRef<HTMLCanvasElement | null>(null)

  React.useEffect(() => {
    const onMessage = (event: MessageEvent<WorkerResponse>): void => {
      const response = event.data
      const request = pending.current.get(response.id)
      if (request === undefined) return
      pending.current.delete(response.id)
      if (response.ok) {
        request.resolve(response.result)
      } else {
        request.reject(new Error(response.error))
      }
    }

    worker.addEventListener('message', onMessage)
    return () => {
      worker.removeEventListener('message', onMessage)
      worker.terminate()
    }
  }, [worker])

  React.useEffect(() => {
    if (preview.kind !== 'image') return
    const canvas = canvasRef.current
    if (canvas === null) return
    canvas.width = preview.width
    canvas.height = preview.height
    const context = canvas.getContext('2d')
    if (context === null) return
    context.putImageData(new ImageData(preview.pixels, preview.width, preview.height), 0, 0)
  }, [preview])

  React.useEffect(() => () => {
    if (preview.kind === 'audio') URL.revokeObjectURL(preview.url)
  }, [preview])

  const callWorker = async <T extends WorkerResult>(request: WorkerRequestPayload): Promise<T> => {
    nextRequestId.current += 1
    const id = nextRequestId.current
    worker.postMessage({ id, ...request } satisfies WorkerRequest)
    return await new Promise<T>((resolve, reject) => {
      pending.current.set(id, {
        reject,
        resolve: resolve as (value: WorkerResult) => void
      })
    })
  }

  const rememberNodes = (incoming: TreeNodeSummary[]): void => {
    setNodes((current) => {
      const next = new Map(current)
      for (const node of incoming) {
        next.set(node.id, {
          children: [],
          loaded: false,
          loading: false,
          ...node
        })
      }
      return next
    })
  }

  const openFile = async (file: File): Promise<void> => {
    setStatus(`Loading ${file.name}...`)
    setErrorStatus(false)

    try {
      const root = await callWorker<TreeNodeSummary>({ file, mapleVersion, type: 'open' })
      const expandedRoot = await callWorker<ExpandResult>({ nodeId: root.id, type: 'expand' })
      rememberNodes([{ ...expandedRoot.node, expandable: expandedRoot.children.length > 0 }, ...expandedRoot.children])
      setNodes((current) => updateNode(current, root.id, {
        ...expandedRoot.node,
        children: expandedRoot.children,
        loaded: true
      }))
      setExpanded((current) => new Set([...current, root.id]))
      setSelectedNodeId(root.id)
      setStatus(`${file.name} opened.`)
    } catch (error) {
      showError(error)
    }
  }

  const openFiles = async (files: FileList): Promise<void> => {
    const tasks = Array.from(files).reduce(
      async (previous, file) => {
        await previous
        await openFile(file)
      },
      Promise.resolve()
    )
    await tasks
  }

  const unloadFile = async (nodeId: number): Promise<void> => {
    try {
      const node = nodes.get(nodeId)
      await callWorker<TreeNodeSummary>({ nodeId, type: 'unload' })
      const removedIds = subtreeIds(nodes, nodeId)
      setNodes((current) => removeNodeSubtree(current, nodeId))
      setExpanded((current) => {
        const next = new Set(current)
        for (const id of removedIds) next.delete(id)
        return next
      })
      if (selectedNodeId !== null && removedIds.has(selectedNodeId)) {
        setSelectedNodeId(null)
        setPreview((current) => {
          if (current.kind === 'audio') URL.revokeObjectURL(current.url)
          return { kind: 'empty' }
        })
      }
      setStatus(`${node?.name ?? 'WZ file'} unloaded.`)
      setErrorStatus(false)
    } catch (error) {
      showError(error)
    }
  }

  const expandNode = async (nodeId: number): Promise<void> => {
    const node = nodes.get(nodeId)
    if (node !== undefined && (node.loading || node.loaded || !node.expandable)) return

    setNodes((current) => updateNode(current, nodeId, { loading: true }))
    try {
      const result = await callWorker<ExpandResult>({ nodeId, type: 'expand' })
      setNodes((current) => {
        const next = updateNode(current, nodeId, {
          ...result.node,
          children: result.children,
          loaded: true,
          loading: false
        })
        for (const child of result.children) {
          next.set(child.id, {
            children: [],
            loaded: false,
            loading: false,
            ...child
          })
        }
        return next
      })
    } catch (error) {
      setNodes((current) => updateNode(current, nodeId, {
        error: messageFromError(error),
        loading: false
      }))
    }
  }

  const selectNode = async (nodeId: number): Promise<void> => {
    const node = nodes.get(nodeId)
    setSelectedNodeId(nodeId)
    if (node === undefined) return
    if (!node.actions.includes('png') && !node.actions.includes('mp3')) {
      setPreview({ kind: 'message', text: node.meta })
      return
    }

    try {
      const result = await callWorker<PreviewResult>({ nodeId, type: 'preview' })
      if (result.kind === 'image') {
        setPreview(result)
        return
      }
      const bytes = new ArrayBuffer(result.bytes.byteLength)
      new Uint8Array(bytes).set(result.bytes)
      const url = URL.createObjectURL(new Blob([bytes], { type: result.mime }))
      setPreview((current) => {
        if (current.kind === 'audio') URL.revokeObjectURL(current.url)
        return {
          kind: 'audio',
          mime: result.mime,
          name: result.name,
          url
        }
      })
    } catch (error) {
      showError(error)
    }
  }

  const toggleNode = (nodeId: number): void => {
    setExpanded((current) => {
      const next = new Set(current)
      if (next.has(nodeId)) {
        next.delete(nodeId)
      } else {
        next.add(nodeId)
        void expandNode(nodeId)
      }
      return next
    })
  }

  const downloadNode = async (nodeId: number, format: 'png' | 'mp3'): Promise<void> => {
    try {
      const result = await callWorker<ExportResult>({ format, nodeId, type: 'export' })
      const bytes = new ArrayBuffer(result.bytes.byteLength)
      new Uint8Array(bytes).set(result.bytes)
      const blob = new Blob([bytes], { type: result.mime })
      const url = URL.createObjectURL(blob)
      const anchor = document.createElement('a')
      anchor.href = url
      anchor.download = result.name
      anchor.click()
      URL.revokeObjectURL(url)
    } catch (error) {
      showError(error)
    }
  }

  const showError = (error: unknown): void => {
    setStatus(messageFromError(error))
    setErrorStatus(true)
  }

  const roots = Array.from(nodes.values()).filter((node) => node.parentId === null)
  const selectedNode = selectedNodeId === null ? undefined : nodes.get(selectedNodeId)

  return (
    <main className="app">
      <header className="toolbar">
        <div>
          <h1>libwz wasm viewer</h1>
          <p className={errorStatus ? 'error' : ''}>{status}</p>
          {selectedNode !== undefined && (
            <p className="selected-path" title={selectedNode.fullPath}>{selectedNode.fullPath}</p>
          )}
        </div>
        <div className="toolbar-actions">
          <label className="version-select">
            <span>Version</span>
            <select
              value={mapleVersion}
              onChange={(event) => {
                setMapleVersion(Number(event.currentTarget.value) as MapleVersionOption)
              }}
            >
              {mapleVersionOptions.map((option) => (
                <option key={option.value} value={option.value}>{option.label}</option>
              ))}
            </select>
          </label>
          <label className="file-picker">
            <span>Open WZ</span>
            <input
              accept=".wz,.img"
              multiple
              type="file"
              onChange={(event) => {
                const input = event.currentTarget
                const files = input.files
                if (files !== null && files.length > 0) void openFiles(files)
                input.value = ''
              }}
            />
          </label>
        </div>
      </header>
      <section className="viewer">
        <div aria-live="polite" className="tree">
          {roots.length === 0
            ? <div className="empty">No WZ file loaded.</div>
            : roots.map((root) => (
              <TreeNode
                key={root.id}
                depth={0}
                expanded={expanded}
                node={root}
                nodes={nodes}
                onDownload={(nodeId, format) => {
                  void downloadNode(nodeId, format)
                }}
                onUnload={(nodeId) => {
                  void unloadFile(nodeId)
                }}
                onSelect={(nodeId) => {
                  void selectNode(nodeId)
                }}
                selectedNodeId={selectedNodeId}
                onToggle={toggleNode}
              />
            ))}
        </div>
        <aside className="preview">
          <PreviewPanel preview={preview} canvasRef={canvasRef} />
        </aside>
      </section>
    </main>
  )
}

function TreeNode ({
  depth,
  expanded,
  node,
  nodes,
  onDownload,
  onUnload,
  onSelect,
  selectedNodeId,
  onToggle
}: {
  depth: number;
  expanded: ReadonlySet<number>;
  node: NodeState;
  nodes: ReadonlyMap<number, NodeState>;
  onDownload: (nodeId: number, format: 'png' | 'mp3') => void;
  onUnload: (nodeId: number) => void;
  onSelect: (nodeId: number) => void;
  selectedNodeId: number | null;
  onToggle: (nodeId: number) => void;
}): React.JSX.Element {
  const isExpanded = expanded.has(node.id)

  return (
    <div className="node">
      <div className={`node-row ${node.kind}${selectedNodeId === node.id ? ' selected' : ''}`} style={{ '--depth': depth } as React.CSSProperties}>
        <button
          className="toggle"
          disabled={!node.expandable || node.loading}
          title={node.expandable ? 'Expand' : undefined}
          type="button"
          onClick={() => {
            onToggle(node.id)
          }}
        >
          {node.expandable ? isExpanded ? '▾' : '▸' : ''}
        </button>
        <button
          className="node-label"
          type="button"
          onClick={() => {
            onSelect(node.id)
            if (node.expandable) onToggle(node.id)
          }}
        >
          <span className="name">{node.name}</span>
          <span className="meta">{node.meta}</span>
        </button>
        {node.actions.includes('png') && (
          <button
            className="export"
            type="button"
            onClick={() => {
              onDownload(node.id, 'png')
            }}
          >
            PNG
          </button>
        )}
        {node.actions.includes('mp3') && (
          <button
            className="export"
            type="button"
            onClick={() => {
              onDownload(node.id, 'mp3')
            }}
          >
            MP3
          </button>
        )}
        {node.actions.includes('unload') && (
          <button
            className="export"
            type="button"
            onClick={() => {
              onUnload(node.id)
            }}
          >
            Unload
          </button>
        )}
      </div>
      {node.error !== undefined && <div className="node-error">{node.error}</div>}
      {isExpanded && node.children.map((child) => {
        const childState = nodes.get(child.id)
        return childState === undefined
          ? null
          : (
            <TreeNode
              key={child.id}
              depth={depth + 1}
              expanded={expanded}
              node={childState}
              nodes={nodes}
              onDownload={onDownload}
              onUnload={onUnload}
              onSelect={onSelect}
              selectedNodeId={selectedNodeId}
              onToggle={onToggle}
            />
          )
      })}
    </div>
  )
}

function PreviewPanel ({
  canvasRef,
  preview
}: {
  canvasRef: React.RefObject<HTMLCanvasElement | null>;
  preview: PreviewState;
}): React.JSX.Element {
  if (preview.kind === 'empty') {
    return <div className="empty">Select a canvas or audio node.</div>
  }
  if (preview.kind === 'message') {
    return <div className="preview-message">{preview.text}</div>
  }
  if (preview.kind === 'audio') {
    return (
      <div className="audio-preview">
        <div className="preview-title">{preview.name}</div>
        <audio controls src={preview.url} />
      </div>
    )
  }
  return (
    <div className="image-preview">
      <div className="preview-title">
        {preview.name} · {preview.width}x{preview.height} · alpha {preview.alpha.nonZero}/{preview.width * preview.height} max {preview.alpha.max} · rgb {preview.rgb.nonZero}
      </div>
      <div className="canvas-frame">
        <canvas
          ref={canvasRef}
          style={{
            height: `${preview.height}px`,
            width: `${preview.width}px`
          }}
        />
      </div>
    </div>
  )
}

function updateNode (
  current: ReadonlyMap<number, NodeState>,
  nodeId: number,
  patch: Partial<NodeState>
): Map<number, NodeState> {
  const next = new Map(current)
  const node = next.get(nodeId)
  if (node !== undefined) next.set(nodeId, { ...node, ...patch })
  return next
}

function removeNodeSubtree (
  current: ReadonlyMap<number, NodeState>,
  nodeId: number
): Map<number, NodeState> {
  const removedIds = subtreeIds(current, nodeId)
  const next = new Map(current)
  for (const id of removedIds) next.delete(id)
  return next
}

function subtreeIds (
  nodes: ReadonlyMap<number, NodeState>,
  nodeId: number
): Set<number> {
  const ids = new Set<number>([nodeId])
  let changed = true
  while (changed) {
    changed = false
    for (const node of nodes.values()) {
      if (!ids.has(node.id) && node.parentId !== null && ids.has(node.parentId)) {
        ids.add(node.id)
        changed = true
      }
    }
  }
  return ids
}

function messageFromError (error: unknown): string {
  return error instanceof Error ? error.message : String(error)
}

function styleElement (text: string): HTMLStyleElement {
  const style = document.createElement('style')
  style.textContent = text
  return style
}

const container = document.createElement('div')
document.body.append(container)
createRoot(container).render(<App />)
