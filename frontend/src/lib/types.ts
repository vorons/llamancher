export interface ModelInfo {
  name: string;
  path: string;
  architecture: string;
  quantization: string;
  size: string;
  block_count: string;
  context_length: string;
  file_type: string;
}

export interface Preset {
  // Basic
  ctx_size: number;
  threads: number;

  // Model & Loading
  gpu_layers: number;           // -1 = all layers
  tensor_split: string;
  numa: string;                  // '' | 'distribute' | 'isolate'
  split_mode: string;            // '' | 'none' | 'layer' | 'row' | 'tensor'
  main_gpu: number;              // -1 = auto
  device: string;                // comma-separated device list
  mlock: boolean;
  no_mmap: boolean;

  // Context & Cache
  batch_size: number;
  ubatch_size: number;
  cache_type_k: string;          // '' | 'f16' | 'q8_0' | ...
  cache_type_v: string;
  flash_attn: boolean;
  defrag_thold: number;          // -1 = off (deprecated)

  // Sampling
  samplers: string;              // e.g. 'top_k;top_p;temperature'
  seed: number;                  // -1 = random
  temp: number;
  top_k: number;
  top_p: number;
  min_p: number;
  repeat_penalty: number;
  presence_penalty: number;
  frequency_penalty: number;
  mirostat: number;              // 0=off, 1=v1, 2=v2

  // Server
  parallel: number;
  no_repack: boolean;

  // Logging
  verbose: boolean;
  verbosity: number;             // 0-5
  log_file: string;

  // Speculative decoding
  spec_type: string;             // '' | 'none' | 'draft-mtp' | 'draft-model' | 'ngram-*'
  spec_draft_n_max: number;
  spec_draft_n_min: number;
  spec_draft_p_split: number;
  draft_model: string;
  draft_gpu_layers: number;      // -1 = all, 0 = none
  threads_draft: number;
  threads_batch_draft: number;
  spec_draft_poll: boolean;

  // Auto-fit
  fit: boolean;
  fit_target_mib: string;        // comma-separated
  fit_ctx: number;
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
