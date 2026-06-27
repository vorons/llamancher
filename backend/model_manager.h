#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct ModelInfo {
  std::string name;      // filename stem
  std::string path;      // full path
  std::string quantization; // e.g. "Q4_K_M"
  uint64_t    file_size = 0;
};

class ModelManager {
public:
  explicit ModelManager(const std::string& models_dir);

  std::vector<ModelInfo> scan() const;
  std::string            guess_quantization(const std::string& filename) const;

private:
  std::filesystem::path dir_;
};
