export interface ModelInfo {
  name: string;
  path: string;
  architecture: string;
  quantization: string;
  size: string;
  block_count: string;
  context_length: string;
  file_type: string;

  // Extended metadata
  display_name: string;
  size_label: string;
  license: string;
  author: string;
  version: string;
  url: string;
  source_url: string;
  languages: string;
  vocab_size: string;
  embedding_length: string;
  feed_forward_length: string;
  head_count: string;
  head_count_kv: string;
  expert_count: string;
  expert_used_count: string;
  tokenizer_model: string;
  bos_token_id: string;
  eos_token_id: string;
  chat_template: string;
  chat_templates: string;
  has_vision: string;
  has_audio: string;
}

export interface Preset {
  // Basic
  ctx_size: number;
  threads: number;

  // Model & Loading
  gpu_layers: number;           // 0-100
  tensor_split: string;
  numa: string;                  // '' | 'distribute' | 'isolate'
  split_mode: string;            // 'none' | 'layer' | 'row' | 'tensor'
  main_gpu: number;              // GPU index
  device: string;                // comma-separated device list
  mlock: boolean;
  no_mmap: boolean;
  jinja: boolean;                // use jinja template engine
  grammar: string;               // BNF grammar
  grammar_file: string;          // grammar file path
  json_schema: string;           // JSON schema

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
  spec_type: string;             // 'none' | 'draft-mtp' | 'draft-model' | 'ngram-*'
  spec_draft_n_max: number;
  spec_draft_n_min: number;
  spec_draft_p_split: number;
  draft_model: string;
  draft_gpu_layers: number;      // -1 = all, 0 = none
  threads_draft: number;
  threads_batch_draft: number;
  spec_draft_poll: boolean;

  // N-gram params
  spec_ngram_mod_n_min: number;
  spec_ngram_mod_n_max: number;
  spec_ngram_mod_n_match: number;
  spec_ngram_simple_size_n: number;
  spec_ngram_simple_size_m: number;
  spec_ngram_simple_min_hits: number;
  spec_ngram_map_k_size_n: number;
  spec_ngram_map_k_size_m: number;
  spec_ngram_map_k_min_hits: number;
  spec_ngram_map_k4v_size_n: number;
  spec_ngram_map_k4v_size_m: number;
  spec_ngram_map_k4v_min_hits: number;

  // Chat template override
  chat_template: string;

  // Auto-fit
  fit: boolean;
  fit_target_mib: string;        // comma-separated
  fit_ctx: number;

  // --- Fields from the comprehensive launch-params spec ---

  // Основные (Main)
  predict: number;               // -n, --predict (default -1 = unlimited)

  // Дополнительные (Additional)
  json_schema_file: string;      // -jf
  cache_type_k_draft: string;    // -ctkd, draft-model KV cache type
  mmproj: string;                // -mm, --mmproj, multimodal projector

  // Специализированные (Specialized)
  rope_scaling: string;          // 'none' | 'linear' | 'yarn'
  rope_scale: number;            // --rope-scale
  yarn_attn_factor: number;      // --yarn-attn-factor (alpha)
  yarn_beta_fast: number;        // --yarn-beta-fast
  yarn_beta_slow: number;        // --yarn-beta-slow
  yarn_orig_ctx: number;         // --yarn-orig-ctx
  lora: string;                  // --lora path
  lora_scaled: string;           // --lora-scaled path,scale
  control_vector: string;        // --control-vector path
  metrics: boolean;              // --metrics endpoint
  offline: boolean;              // --offline (cache only)
  slot_save_path: string;        // --slot-save-path
  sleep_idle_seconds: number;    // --sleep-idle-seconds
  webui: boolean;                // --webui

  // Server-mode flags (used for dependency logic)
  embedding: boolean;            // --embedding (hides sampling params)
  rerank: boolean;               // --rerank (hides sampling params)
}

export interface Settings {
  llama_server_path: string;
  models_dir: string;
  auto_start_server: boolean;
  theme: 'dark' | 'light';
  port: number;
  api_key: string;
  last_model: string;
}

export type ServerStatus = 'stopped' | 'starting' | 'running' | 'error';

export interface ServerStatusInfo {
  status: ServerStatus;
  model: string;
}
