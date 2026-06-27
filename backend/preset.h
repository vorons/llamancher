#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct Preset {
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

  static Preset load(const std::string& model_name);
  void save(const std::string& model_name) const;
  std::filesystem::path path(const std::string& model_name) const;

  // Build CLI args vector for llama-server
  std::vector<std::string> cli_args(const std::string& model_path) const;
};
