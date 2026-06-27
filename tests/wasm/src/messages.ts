export interface TreeNodeSummary {
  actions: Array<'png' | 'mp3' | 'unload'>;
  expandable: boolean;
  fullPath: string;
  id: number;
  kind: 'file' | 'directory' | 'image' | 'property' | 'canvas' | 'binary' | 'unknown';
  meta: string;
  name: string;
  parentId: number | null;
}

export type MapleVersionOption = 0 | 1 | 2 | 6

export type PreviewResult
  = | {
    alpha: {
      max: number;
      nonZero: number;
    };
    height: number;
    kind: 'image';
    name: string;
    pixels: Uint8ClampedArray<ArrayBuffer>;
    rgb: {
      nonZero: number;
    };
    width: number;
  }
  | {
    bytes: Uint8Array;
    kind: 'audio';
    mime: string;
    name: string;
  }

export interface ExpandResult {
  children: TreeNodeSummary[];
  node: TreeNodeSummary;
}

export type WorkerRequest
  = | { file: File; id: number; mapleVersion: MapleVersionOption; type: 'open' }
  | { id: number; nodeId: number; type: 'expand' }
  | { format: 'png' | 'mp3'; id: number; nodeId: number; type: 'export' }
  | { id: number; nodeId: number; type: 'preview' }
  | { id: number; nodeId: number; type: 'unload' }

export type WorkerRequestPayload
  = | { file: File; mapleVersion: MapleVersionOption; type: 'open' }
  | { nodeId: number; type: 'expand' }
  | { format: 'png' | 'mp3'; nodeId: number; type: 'export' }
  | { nodeId: number; type: 'preview' }
  | { nodeId: number; type: 'unload' }

export type WorkerResponse
  = | { id: number; ok: true; result: TreeNodeSummary | ExpandResult | ExportResult | PreviewResult }
  | { error: string; id: number; ok: false }

export interface ExportResult {
  bytes: Uint8Array;
  mime: string;
  name: string;
}
