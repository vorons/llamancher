#include "model_manager.h"
#include <regex>

namespace fs = std::filesystem;

ModelManager::ModelManager(const std::string& models_dir)
  : dir_(models_dir) {}

std::vector<ModelInfo> ModelManager::scan() const {
  std::vector<ModelInfo> models;
  if (!fs::exists(dir_)) return models;

  // ponytail: flat scan, no recursion. Upgrade to recursive_directory_iterator if nested dirs needed.
  for (auto& entry : fs::directory_iterator(dir_)) {
    if (!entry.is_regular_file()) continue;
    auto ext = entry.path().extension().string();
    if (ext != ".gguf") continue;

    auto stem = entry.path().stem().string();
    models.push_back({
      .name = stem,
      .path = entry.path().string(),
      .quantization = guess_quantization(stem),
      .file_size = entry.file_size(),
    });
  }

  return models;
}

std::string ModelManager::guess_quantization(const std::string& filename) const {
  // ponytail: simple regex hunts for common quantization tags in the name
  std::regex re(R"((Q[0-9]+_[A-Z]+|[q][0-9]+_[a-z]+|[fF][0-9]+))");
  std::smatch m;
  if (std::regex_search(filename, m, re)) {
    return m[1];
  }
  // Fallback: try to find any Q-digit pattern
  std::regex re2(R"(Q(\d)[A-Z]?)");
  if (std::regex_search(filename, m, re2)) {
    return "Q" + m[1].str();
  }
  return "unknown";
}
