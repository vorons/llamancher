export interface ModelInfo {
  name: string;
  path: string;
  quantization: string;
  size: string;
}

export interface Preset {
  ctx_size: number;
  threads: number;
  gpu_layers: number;
  tensor_split: string;
  temp: number;
  top_k: number;
  top_p: number;
  mlock: boolean;
  no_mmap: boolean;
  flash_attn: boolean;
}

export interface Settings {
  llama_server_path: string;
  models_dir: string;
  auto_start_server: boolean;
}

export type ServerStatus = 'stopped' | 'starting' | 'running' | 'error';

export interface ServerStatusInfo {
  status: ServerStatus;
  model: string;
}
