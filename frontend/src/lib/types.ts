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

export interface CustomArg {
  arg: string;
  enabled: boolean;
}

export interface Preset {
  // ─── Основные ────────────────────────
  ctx_size: number;
  threads: number;
  threads_batch: number;       // 0 = auto
  gpu_layers: number;          // 0 = auto, 100+ = all
  batch_size: number;
  ubatch_size: number;
  mmproj: string;
  cache_type_k: string;        // "" = default f16
  cache_type_v: string;        // "" = default f16
  parallel: number;            // -np
  timeout: number;             // -to, 0 = no timeout
  seed: number;                // -1 = random
  flash_attn: boolean;
  mlock: boolean;
  no_mmap: boolean;            // false = mmap enabled

  // ─── Генерация ───────────────────────
  temp: number;
  predict: number;             // -1 = unlimited
  min_p: number;
  top_k: number;
  top_p: number;
  repeat_penalty: number;
  presence_penalty: number;
  frequency_penalty: number;
  reasoning_mode: boolean;
  reasoning_budget: number;

  // ─── Спекулятивное декодирование ─────
  spec_type: string;           // "" = none
  draft_model: string;
  hf_repo_draft: string;
  draft_gpu_layers: number;
  spec_draft_n_max: number;
  spec_draft_n_min: number;
  spec_draft_p_split: number;
  spec_draft_p_min: number;

  // ─── Дополнительные ─────────────────
  cont_batching: boolean;
  webui: boolean;
  embedding: boolean;
  slots: boolean;
  metrics: boolean;
  cache_prompt: boolean;
  context_shift: boolean;
  offline: boolean;
  alias: string;

  // ─── Custom CLI args ────────────────
  custom_args: CustomArg[];
}

export interface Settings {
  llama_server_path: string;
  models_dir: string;
  auto_start_server: boolean;
  theme: 'dark' | 'light';
  port: number;
  api_key: string;
  last_model: string;
  locale: 'auto' | 'en' | 'ru';
}

export type ServerStatus = 'stopped' | 'starting' | 'running' | 'error';

