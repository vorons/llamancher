#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <glaze/glaze.hpp>

struct Preset {
  // Basic
  int ctx_size = 2048;
  int threads = 4;

  // Model & Loading
  int gpu_layers = 0;            // number of layers offloaded to GPU
  std::string tensor_split;
  std::string numa;              // '' | distribute | isolate
  std::string split_mode;        // 'none' | layer | row | tensor
  int main_gpu = 0;              // GPU index
  std::string device;            // comma-separated, e.g. "Vulkan0,Vulkan1"
  bool mlock = false;
  bool no_mmap = false;
  bool jinja = true;             // use jinja template engine
  std::string grammar;           // BNF grammar
  std::string grammar_file;      // grammar file path
  std::string json_schema;       // JSON schema

  // Context & Cache
  int batch_size = 2048;
  int ubatch_size = 512;
  std::string cache_type_k;      // '' | f16 | q8_0 | ...
  std::string cache_type_v;
  bool flash_attn = false;
  int defrag_thold = -1;         // -1 = off (deprecated)

  // Sampling
  std::string samplers;          // e.g. "top_k;top_p;temperature"
  int seed = -1;                 // -1 = random
  float temp = 0.80f;
  int top_k = 40;
  float top_p = 0.95f;
  float min_p = 0.05f;
  float repeat_penalty = 1.00f;
  float presence_penalty = 0.00f;
  float frequency_penalty = 0.00f;
  int mirostat = 0;              // 0=off, 1=v1, 2=v2

  // Server
  int parallel = 1;
  bool no_repack = false;

  // Logging
  bool verbose = false;
  int verbosity = 0;             // 0-5
  std::string log_file;

  // Speculative decoding
  std::string spec_type;         // 'none' | draft-mtp | draft-model | ngram-*
  int spec_draft_n_max = 16;
  int spec_draft_n_min = 0;
  float spec_draft_p_split = 0.50f;
  std::string draft_model;
  int draft_gpu_layers = 0;      // -1 = all, 0 = none
  int threads_draft = 0;         // 0 = auto
  int threads_batch_draft = 0;   // 0 = auto
  bool spec_draft_poll = false;

  // N-gram speculative decoding params (per variant)
  int spec_ngram_mod_n_min = 48;
  int spec_ngram_mod_n_max = 64;
  int spec_ngram_mod_n_match = 24;
  int spec_ngram_simple_size_n = 12;
  int spec_ngram_simple_size_m = 48;
  int spec_ngram_simple_min_hits = 1;
  int spec_ngram_map_k_size_n = 12;
  int spec_ngram_map_k_size_m = 48;
  int spec_ngram_map_k_min_hits = 1;
  int spec_ngram_map_k4v_size_n = 12;
  int spec_ngram_map_k4v_size_m = 48;
  int spec_ngram_map_k4v_min_hits = 1;

  // Auto-fit
  bool fit = true;
  std::string fit_target_mib;    // comma-separated, default "1024"
  int fit_ctx = 4096;

  // GGUF metadata (read once, cached in preset file)
  std::string architecture;
  uint32_t block_count = 0;
  uint32_t context_length = 0;
  int32_t file_type = -1;

  // Extended metadata (cached from GGUF)
  std::string display_name;
  std::string size_label;
  std::string license;
  std::string author;
  std::string version;
  std::string url;
  std::string source_url;
  std::string languages;         // comma-joined
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
  std::string chat_templates;    // comma-joined
  bool has_vision = false;
  bool has_audio = false;

  static Preset load(const std::string& model_name);
  void save(const std::string& model_name) const;
  std::filesystem::path path(const std::string& model_name) const;
  static bool exists(const std::string& model_name);

  // Build CLI args vector for llama-server
  std::vector<std::string> cli_args(const std::string& model_path) const;
};

template<>
struct glz::meta<Preset> {
  using T = Preset;
  static constexpr auto value = object(
    // Basic
    "ctx_size",              &T::ctx_size,
    "threads",               &T::threads,
    // Model & Loading
    "gpu_layers",            &T::gpu_layers,
    "tensor_split",          &T::tensor_split,
    "numa",                  &T::numa,
    "split_mode",            &T::split_mode,
    "main_gpu",              &T::main_gpu,
    "device",                &T::device,
    "mlock",                 &T::mlock,
    "no_mmap",               &T::no_mmap,
    "jinja",                 &T::jinja,
    "grammar",               &T::grammar,
    "grammar_file",          &T::grammar_file,
    "json_schema",           &T::json_schema,
    // Context & Cache
    "batch_size",            &T::batch_size,
    "ubatch_size",           &T::ubatch_size,
    "cache_type_k",          &T::cache_type_k,
    "cache_type_v",          &T::cache_type_v,
    "flash_attn",            &T::flash_attn,
    "defrag_thold",          &T::defrag_thold,
    // Sampling
    "samplers",              &T::samplers,
    "seed",                  &T::seed,
    "temp",                  &T::temp,
    "top_k",                 &T::top_k,
    "top_p",                 &T::top_p,
    "min_p",                 &T::min_p,
    "repeat_penalty",        &T::repeat_penalty,
    "presence_penalty",      &T::presence_penalty,
    "frequency_penalty",     &T::frequency_penalty,
    "mirostat",              &T::mirostat,
    // Server
    "parallel",              &T::parallel,
    "no_repack",             &T::no_repack,
    // Logging
    "verbose",               &T::verbose,
    "verbosity",             &T::verbosity,
    "log_file",              &T::log_file,
    // Speculative decoding
    "spec_type",             &T::spec_type,
    "spec_draft_n_max",      &T::spec_draft_n_max,
    "spec_draft_n_min",      &T::spec_draft_n_min,
    "spec_draft_p_split",    &T::spec_draft_p_split,
    "draft_model",           &T::draft_model,
    "draft_gpu_layers",      &T::draft_gpu_layers,
    "threads_draft",         &T::threads_draft,
    "threads_batch_draft",   &T::threads_batch_draft,
    "spec_draft_poll",       &T::spec_draft_poll,
    // N-gram
    "spec_ngram_mod_n_min",       &T::spec_ngram_mod_n_min,
    "spec_ngram_mod_n_max",       &T::spec_ngram_mod_n_max,
    "spec_ngram_mod_n_match",     &T::spec_ngram_mod_n_match,
    "spec_ngram_simple_size_n",   &T::spec_ngram_simple_size_n,
    "spec_ngram_simple_size_m",   &T::spec_ngram_simple_size_m,
    "spec_ngram_simple_min_hits", &T::spec_ngram_simple_min_hits,
    "spec_ngram_map_k_size_n",    &T::spec_ngram_map_k_size_n,
    "spec_ngram_map_k_size_m",    &T::spec_ngram_map_k_size_m,
    "spec_ngram_map_k_min_hits",  &T::spec_ngram_map_k_min_hits,
    "spec_ngram_map_k4v_size_n",  &T::spec_ngram_map_k4v_size_n,
    "spec_ngram_map_k4v_size_m",  &T::spec_ngram_map_k4v_size_m,
    "spec_ngram_map_k4v_min_hits",&T::spec_ngram_map_k4v_min_hits,
    // Auto-fit
    "fit",                   &T::fit,
    "fit_target_mib",        &T::fit_target_mib,
    "fit_ctx",               &T::fit_ctx,
    // GGUF metadata
    "architecture",            &T::architecture,
    "block_count",             &T::block_count,
    "context_length",          &T::context_length,
    "file_type",               &T::file_type,
    // Extended metadata
    "display_name",            &T::display_name,
    "size_label",              &T::size_label,
    "license",                 &T::license,
    "author",                  &T::author,
    "version",                 &T::version,
    "url",                     &T::url,
    "source_url",              &T::source_url,
    "languages",               &T::languages,
    "vocab_size",              &T::vocab_size,
    "embedding_length",        &T::embedding_length,
    "feed_forward_length",     &T::feed_forward_length,
    "head_count",              &T::head_count,
    "head_count_kv",           &T::head_count_kv,
    "expert_count",            &T::expert_count,
    "expert_used_count",       &T::expert_used_count,
    "tokenizer_model",         &T::tokenizer_model,
    "bos_token_id",            &T::bos_token_id,
    "eos_token_id",            &T::eos_token_id,
    "chat_template",           &T::chat_template,
    "chat_templates",          &T::chat_templates,
    "has_vision",              &T::has_vision,
    "has_audio",               &T::has_audio
  );
};
