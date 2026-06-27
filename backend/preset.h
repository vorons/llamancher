#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct Preset {
  // Basic
  int ctx_size = 2048;
  int threads = 4;

  // Model & Loading
  int gpu_layers = 0;            // -1 = all layers
  std::string tensor_split;
  std::string numa;              // '' | distribute | isolate
  std::string split_mode;        // '' | none | layer | row | tensor
  int main_gpu = -1;             // -1 = auto
  std::string device;            // comma-separated, e.g. "Vulkan0,Vulkan1"
  bool mlock = false;
  bool no_mmap = false;

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
  std::string spec_type;         // '' | none | draft-mtp | draft-model | ngram-*
  int spec_draft_n_max = 16;
  int spec_draft_n_min = 0;
  float spec_draft_p_split = 0.50f;
  std::string draft_model;
  int draft_gpu_layers = 0;      // -1 = all, 0 = none
  int threads_draft = 0;         // 0 = auto
  int threads_batch_draft = 0;   // 0 = auto
  bool spec_draft_poll = false;

  // Auto-fit
  bool fit = true;
  std::string fit_target_mib;    // comma-separated, default "1024"
  int fit_ctx = 4096;

  // GGUF metadata (read once, cached in preset file)
  std::string architecture;
  uint32_t block_count = 0;
  uint32_t context_length = 0;
  int32_t file_type = -1;

  static Preset load(const std::string& model_name);
  void save(const std::string& model_name) const;
  std::filesystem::path path(const std::string& model_name) const;
  static bool exists(const std::string& model_name);

  // Build CLI args vector for llama-server
  std::vector<std::string> cli_args(const std::string& model_path) const;
};
