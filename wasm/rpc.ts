export type RemoteRefKind = "file" | "directory" | "image" | "property" | "object";

export interface RemoteRef {
  kind: RemoteRefKind;
  handle: string;
  type?: string;
  ownerFile?: string;
}

export interface RpcRequest {
  id: number | string;
  method: string;
  args: unknown[];
}

export type RpcResponse = RpcSuccessResponse | RpcErrorResponse;

export interface RpcSuccessResponse {
  id: number | string;
  ok: true;
  value: unknown;
}

export interface RpcErrorResponse {
  id: number | string;
  ok: false;
  error: {
    name: string;
    message: string;
  };
}
