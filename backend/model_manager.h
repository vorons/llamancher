#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct ModelInfo {
  std::string name;           // filename stem
  std::string path;           // full path
  std::string quantization;   // e.g. "Q4_K_M"
  std::string architecture;   // e.g. "llama", "mistral", "phi3"
  uint32_t    block_count = 0;     // number of transformer blocks (layers)
  uint32_t    context_length = 0;  // max context length
  int32_t     file_type = -1;      // GGUF file type enum
  uint64_t    file_size = 0;
};

class ModelManager {
public:
  explicit ModelManager(const std::string& models_dir);

  std::vector<ModelInfo> scan() const;
  std::string            guess_quantization(const std::string& filename) const;

private:
  // GGUF header reader — fills architecture, block_count, context_length, file_type
  void read_gguf_metadata(const std::filesystem::path& path, ModelInfo& info) const;

  std::filesystem::path dir_;
};
