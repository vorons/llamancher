#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <glaze/glaze.hpp>

struct Preset {
  // ─── Основные ──────────────────────────────────────────
  int ctx_size = 2048;
  int threads = 4;
  int threads_batch = 0;    // 0 = auto
  int gpu_layers = 0;       // 0 = auto, 100+ = all
  int batch_size = 2048;
  int ubatch_size = 512;
  std::string mmproj;       // --mmproj
  std::string cache_type_k; // "" = default f16
  std::string cache_type_v; // "" = default f16
  int parallel = 1;         // -np
  int timeout = 0;          // -to, 0 = no timeout
  int seed = -1;            // -s, -1 = random
  bool flash_attn = false;
  bool mlock = false;
  bool no_mmap = false;     // false = mmap enabled

  // ─── Генерация ──────────────────────────────────────────
  float temp = 0.80f;
  int predict = -1;          // -n, -1 = unlimited
  float min_p = 0.05f;
  int top_k = 40;
  float top_p = 0.95f;
  float repeat_penalty = 1.00f;
  float presence_penalty = 0.00f;
  float frequency_penalty = 0.00f;
  bool reasoning_mode = false;  // -rea
  int reasoning_budget = 0;     // --reasoning-budget

  // ─── Спекулятивное декодирование ────────────────────────
  std::string spec_type;          // "" = none
  std::string draft_model;        // -md
  std::string hf_repo_draft;      // --hf-repo-draft
  int draft_gpu_layers = 0;       // -ngld
  int spec_draft_n_max = 16;      // --spec-draft-n-max
  int spec_draft_n_min = 0;       // --spec-draft-n-min
  float spec_draft_p_split = 0.50f; // --spec-draft-p-split
  float spec_draft_p_min = 0.0f;    // --spec-draft-p-min

  // ─── Дополнительные ─────────────────────────────────────
  bool cont_batching = true;   // -cb
  bool webui = true;           // --webui
  bool embedding = false;      // --embedding
  bool slots = true;           // --slots
  bool metrics = false;        // --metrics
  bool cache_prompt = true;    // --cache-prompt
  bool context_shift = true;   // --context-shift
  std::string alias;           // -a

  // ─── Custom CLI args (toggleable) ──────────────────────
  struct CustomArg {
    std::string arg;
    bool enabled = true;
  };
  std::vector<CustomArg> custom_args;

  // ─── GGUF metadata (KEEP) ───────────────────────────────
  std::string architecture;
  uint32_t block_count = 0;
  uint32_t context_length = 0;
  int32_t file_type = -1;
  std::string display_name;
  std::string size_label;
  std::string license;
  std::string author;
  std::string version;
  std::string url;
  std::string source_url;
  std::string languages;
  uint32_t vocab_size = 0;
  uint32_t embedding_length = 0;
  uint32_t feed_forward_length = 0;
  uint32_t head_count = 0;
  uint32_t head_count_kv = 0;
  uint32_t expert_count = 0;
  uint32_t expert_used_count = 0;
  std::string tokenizer_model;
  int32_t bos_token_id = -1;
  int32_t eos_token_id = -1;
  std::string chat_template;
  std::string chat_templates;
  bool has_vision = false;
  bool has_audio = false;

  static Preset load(const std::string& model_path);
  void save(const std::string& model_path) const;
  std::filesystem::path path(const std::string& model_path) const;
  static bool exists(const std::string& model_path);

  // Build CLI args vector for llama-server
  std::vector<std::string> cli_args(const std::string& model_path) const;
};

template<>
struct glz::meta<Preset::CustomArg> {
  using T = Preset::CustomArg;
  static constexpr auto value = object(
    "arg",     &T::arg,
    "enabled", &T::enabled
  );
};

template<>
struct glz::meta<Preset> {
  using T = Preset;
  static constexpr auto value = object(
    // ─── Основные ──────────────────────────────────────────
    "ctx_size",        &T::ctx_size,
    "threads",         &T::threads,
    "threads_batch",   &T::threads_batch,
    "gpu_layers",      &T::gpu_layers,
    "batch_size",      &T::batch_size,
    "ubatch_size",     &T::ubatch_size,
    "mmproj",          &T::mmproj,
    "cache_type_k",    &T::cache_type_k,
    "cache_type_v",    &T::cache_type_v,
    "parallel",        &T::parallel,
    "timeout",         &T::timeout,
    "seed",            &T::seed,
    "flash_attn",      &T::flash_attn,
    "mlock",           &T::mlock,
    "no_mmap",         &T::no_mmap,
    // ─── Генерация ──────────────────────────────────────────
    "temp",              &T::temp,
    "predict",           &T::predict,
    "min_p",             &T::min_p,
    "top_k",             &T::top_k,
    "top_p",             &T::top_p,
    "repeat_penalty",    &T::repeat_penalty,
    "presence_penalty",  &T::presence_penalty,
    "frequency_penalty", &T::frequency_penalty,
    "reasoning_mode",    &T::reasoning_mode,
    "reasoning_budget",  &T::reasoning_budget,
    // ─── Спекулятивное декодирование ────────────────────────
    "spec_type",           &T::spec_type,
    "draft_model",         &T::draft_model,
    "hf_repo_draft",       &T::hf_repo_draft,
    "draft_gpu_layers",    &T::draft_gpu_layers,
    "spec_draft_n_max",    &T::spec_draft_n_max,
    "spec_draft_n_min",    &T::spec_draft_n_min,
    "spec_draft_p_split",  &T::spec_draft_p_split,
    "spec_draft_p_min",    &T::spec_draft_p_min,
    // ─── Дополнительные ─────────────────────────────────────
    "cont_batching",  &T::cont_batching,
    "webui",          &T::webui,
    "embedding",      &T::embedding,
    "slots",          &T::slots,
    "metrics",        &T::metrics,
    "cache_prompt",   &T::cache_prompt,
    "context_shift",  &T::context_shift,
    "alias",          &T::alias,
    // ─── Custom CLI args ────────────────────────────────────
    "custom_args",   &T::custom_args,
    // ─── GGUF metadata ─────────────────────────────────────
    "architecture",        &T::architecture,
    "block_count",         &T::block_count,
    "context_length",      &T::context_length,
    "file_type",           &T::file_type,
    "display_name",        &T::display_name,
    "size_label",          &T::size_label,
    "license",             &T::license,
    "author",              &T::author,
    "version",             &T::version,
    "url",                 &T::url,
    "source_url",          &T::source_url,
    "languages",           &T::languages,
    "vocab_size",          &T::vocab_size,
    "embedding_length",    &T::embedding_length,
    "feed_forward_length", &T::feed_forward_length,
    "head_count",          &T::head_count,
    "head_count_kv",       &T::head_count_kv,
    "expert_count",        &T::expert_count,
    "expert_used_count",   &T::expert_used_count,
    "tokenizer_model",     &T::tokenizer_model,
    "bos_token_id",        &T::bos_token_id,
    "eos_token_id",        &T::eos_token_id,
    "chat_template",       &T::chat_template,
    "chat_templates",      &T::chat_templates,
    "has_vision",          &T::has_vision,
    "has_audio",           &T::has_audio
  );
};
