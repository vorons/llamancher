#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct Preset {
  // User-configurable parameters
  int ctx_size = 2048;
  int threads = 4;
  int gpu_layers = 0;
  std::string tensor_split;
  float temp = 0.7f;
  int top_k = 40;
  float top_p = 0.9f;
  bool mlock = false;
  bool no_mmap = false;
  bool flash_attn = false;

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
