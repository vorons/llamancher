#include "model_manager.h"
#include <regex>
#include <fstream>
#include <vector>
#include <cstring>

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
    ModelInfo info{
      .name = stem,
      .path = entry.path().string(),
      .quantization = guess_quantization(stem),
      .file_size = entry.file_size(),
    };

    // Read GGUF header for architecture, layers, context, file_type
    read_gguf_metadata(entry.path(), info);

    models.push_back(std::move(info));
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

// ── Minimal GGUF header reader ──────────────────────────────────────────────
//
// GGUF format (little-endian):
//   offset 0:  magic   = 'GGUF' (4 bytes)
//   offset 4:  version = uint32
//   offset 8:  tensor_count = uint64
//   offset 16: metadata_kv_count = uint64
//   offset 24: KV pairs...
//
// Value type enum:
//   0=uint8, 1=int8, 2=uint16, 3=int16, 4=uint32, 5=int32,
//   6=float32, 7=bool, 8=string, 9=array, 10=uint64, 11=int64, 12=float64

static uint32_t read_u32(const char* p) {
  uint32_t v;
  std::memcpy(&v, p, 4);
  return v;
}

static uint64_t read_u64(const char* p) {
  uint64_t v;
  std::memcpy(&v, p, 8);
  return v;
}

static int32_t read_i32(const char* p) {
  int32_t v;
  std::memcpy(&v, p, 4);
  return v;
}

void ModelManager::read_gguf_metadata(const fs::path& path, ModelInfo& info) const {
  std::ifstream f(path, std::ios::binary);
  if (!f) return;

  // Read header preamble (24 bytes)
  char header[24];
  if (!f.read(header, sizeof(header))) return;

  // Validate magic
  if (std::memcmp(header, "GGUF", 4) != 0) return;

  uint64_t tensor_count       = read_u64(header + 8);
  uint64_t metadata_kv_count  = read_u64(header + 16);

  // Prevent pathological values — no model has billions of tensors
  if (tensor_count > 1'000'000 || metadata_kv_count > 100'000) return;

  // Read all KV pairs in one shot (header is typically <64 KB)
  // Estimate max bytes needed: metadata_kv_count * 256 avg = generous
  // But cap at 256 KB to avoid abuse
  static constexpr uint64_t MAX_HEADER = 256 * 1024;
  uint64_t header_size = metadata_kv_count * 256; // rough estimate
  if (header_size > MAX_HEADER) header_size = MAX_HEADER;

  std::vector<char> buf(header_size);
  f.read(buf.data(), buf.size());
  uint64_t bytes_read = f.gcount();
  if (bytes_read == 0) return;

  const char* p = buf.data();
  const char* end = p + bytes_read;

  // Track extracted architecture for architecture-prefixed keys
  std::string arch;

  for (uint64_t i = 0; i < metadata_kv_count; ++i) {
    if (p + 8 > end) break;

    // Key
    uint64_t key_len = read_u64(p); p += 8;
    if (p + key_len > end) break;
    std::string key(p, key_len); p += key_len;

    // Value type
    if (p + 4 > end) break;
    uint32_t val_type = read_u32(p); p += 4;

    // Extract value based on type
    auto read_string = [&]() -> std::string {
      if (p + 8 > end) return {};
      uint64_t len = read_u64(p); p += 8;
      if (p + len > end) return {};
      std::string s(p, len); p += len;
      return s;
    };

    auto read_uint32 = [&]() -> uint32_t {
      if (p + 4 > end) return 0;
      uint32_t v = read_u32(p); p += 4;
      return v;
    };

    auto read_uint64 = [&]() -> uint64_t {
      if (p + 8 > end) return 0;
      uint64_t v = read_u64(p); p += 8;
      return v;
    };

    auto read_int32 = [&]() -> int32_t {
      if (p + 4 > end) return 0;
      int32_t v = read_i32(p); p += 4;
      return v;
    };

    if (key == "general.architecture" && val_type == 8) {
      info.architecture = read_string();
      arch = info.architecture;
      continue;
    }

    // Skip string values we don't care about
    if (val_type == 8) {
      if (p + 8 > end) break;
      uint64_t len = read_u64(p); p += 8;
      if (p + len > end) break;
      p += len;
      continue;
    }

    // For architecture-prefixed keys, wait until we have the arch
    if (arch.empty() && key.find('.') != std::string::npos) {
      // Skip non-arch-prefixed value
      switch (val_type) {
        case 0: case 1: p += 1; break;
        case 2: case 3: p += 2; break;
        case 4: case 5: case 6: case 7: p += 4; break;
        case 10: case 11: case 12: p += 8; break;
        default: return; // unknown type or array — bail
      }
      continue;
    }

    // Check architecture-prefixed keys
    if (key == arch + ".block_count" && (val_type == 4 || val_type == 10)) {
      info.block_count = (val_type == 4) ? read_uint32() : static_cast<uint32_t>(read_uint64());
    } else if (key == arch + ".context_length" && (val_type == 4 || val_type == 10)) {
      info.context_length = (val_type == 4) ? read_uint32() : static_cast<uint32_t>(read_uint64());
    } else if (key == "general.file_type" && val_type == 5) {
      info.file_type = read_int32();
    } else {
      // Skip value of known scalar types
      switch (val_type) {
        case 0: case 1: p += 1; break;
        case 2: case 3: p += 2; break;
        case 4: case 5: case 6: case 7: p += 4; break;
        case 10: case 11: case 12: p += 8; break;
        default: return; // array or unknown — give up on further parsing
      }
    }
  }
}
