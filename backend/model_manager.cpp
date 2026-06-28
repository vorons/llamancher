#include "model_manager.h"
#include <regex>
#include <fstream>
#include <vector>
#include <cstring>
#include <charconv>

namespace fs = std::filesystem;

ModelManager::ModelManager(const std::string& models_dir)
  : dir_(models_dir) {}

std::vector<ModelInfo> ModelManager::scan() const {
  std::vector<ModelInfo> models;
  if (!fs::exists(dir_)) return models;

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

    read_gguf_metadata(entry.path(), info);

    models.push_back(std::move(info));
  }

  return models;
}

std::string ModelManager::guess_quantization(const std::string& filename) const {
  std::regex re(R"((Q[0-9]+_[A-Z]+|[q][0-9]+_[a-z]+|[fF][0-9]+))");
  std::smatch m;
  if (std::regex_search(filename, m, re)) return m[1];
  std::regex re2(R"(Q(\d)[A-Z]?)");
  if (std::regex_search(filename, m, re2)) return "Q" + m[1].str();
  return "unknown";
}

// ── GGUF helpers (little-endian) ───────────────────────────────────────────
static uint32_t read_u32(const char* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
static uint64_t read_u64(const char* p) { uint64_t v; std::memcpy(&v, p, 8); return v; }
static int32_t  read_i32(const char* p)  { int32_t  v; std::memcpy(&v, p, 4); return v; }
static float    read_f32(const char* p)  { float    v; std::memcpy(&v, p, 4); return v; }

// ── Skip any GGUF value by type; returns false on out-of-bounds ────────────
// Updates p to point past the value.
static bool skip_value(const char*& p, const char* end, uint32_t val_type) {
  auto check = [&](size_t need) { return p + need <= end; };

  switch (val_type) {
    case 0: case 1:  // uint8, int8
      if (!check(1)) return false; p += 1; return true;
    case 2: case 3:  // uint16, int16
      if (!check(2)) return false; p += 2; return true;
    case 4: case 5: case 6: case 7:  // uint32, int32, float32, bool
      if (!check(4)) return false; p += 4; return true;
    case 10: case 11: case 12:  // uint64, int64, float64
      if (!check(8)) return false; p += 8; return true;
    case 8: {  // string
      if (!check(8)) return false;
      uint64_t len = read_u64(p); p += 8;
      if (!check(len)) return false;
      p += len;
      return true;
    }
    case 9: {  // array: uint32 element_type + uint64 count + elements
      if (!check(4)) return false;
      uint32_t elem_type = read_u32(p); p += 4;
      if (!check(8)) return false;
      uint64_t count = read_u64(p); p += 8;
      for (uint64_t i = 0; i < count; ++i) {
        if (!skip_value(p, end, elem_type)) return false;
      }
      return true;
    }
    default:
      return false; // unknown type
  }
}

// ── Read a string value (type 8) ──────────────────────────────────────────
static std::string read_string_val(const char*& p, const char* end) {
  if (p + 8 > end) return {};
  uint64_t len = read_u64(p); p += 8;
  if (p + len > end) return {};
  std::string s(p, len); p += len;
  return s;
}

// ── Read a string array (type 9, elem_type=8) ─────────────────────────────
// Returns comma-joined string or empty if not a string array.
static std::string read_string_array(const char*& p, const char* end, uint32_t val_type) {
  if (val_type != 9) return {};
  if (p + 4 > end) return {};
  uint32_t elem_type = read_u32(p); p += 4;
  if (elem_type != 8) { std::fprintf(stderr, "gguf: unexpected array elem type %u for string array\n", elem_type); return {}; }
  if (p + 8 > end) return {};
  uint64_t count = read_u64(p); p += 8;
  std::string result;
  for (uint64_t i = 0; i < count; ++i) {
    auto s = read_string_val(p, end);
    if (!result.empty()) result += ",";
    result += s;
  }
  return result;
}

void ModelManager::read_gguf_metadata(const fs::path& path, ModelInfo& info) const {
  std::ifstream f(path, std::ios::binary);
  if (!f) return;

  char header[24];
  if (!f.read(header, sizeof(header))) return;
  if (std::memcmp(header, "GGUF", 4) != 0) return;

  uint64_t tensor_count      = read_u64(header + 8);
  uint64_t metadata_kv_count = read_u64(header + 16);

  if (tensor_count > 1'000'000 || metadata_kv_count > 100'000) return;

  static constexpr uint64_t MAX_HEADER = 256 * 1024;
  uint64_t header_size = metadata_kv_count * 256;
  if (header_size > MAX_HEADER) header_size = MAX_HEADER;

  std::vector<char> buf(header_size);
  f.read(buf.data(), buf.size());
  uint64_t bytes_read = f.gcount();
  if (bytes_read == 0) return;

  const char* p   = buf.data();
  const char* end = p + bytes_read;

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

    // ── Known fields we want to extract ──
    auto try_u32 = [&](uint32_t& dest) {
      if (val_type == 4) { if (p + 4 > end) return false; dest = read_u32(p); p += 4; return true; }
      if (val_type == 10) { if (p + 8 > end) return false; dest = static_cast<uint32_t>(read_u64(p)); p += 8; return true; }
      return skip_value(p, end, val_type);
    };
    auto try_i32 = [&](int32_t& dest) {
      if (val_type == 5) { if (p + 4 > end) return false; dest = read_i32(p); p += 4; return true; }
      if (val_type == 11) { if (p + 8 > end) return false; dest = static_cast<int32_t>(read_u64(p)); p += 8; return true; }
      return skip_value(p, end, val_type);
    };
    auto try_f32 = [&](float& dest) {
      if (val_type == 6) { if (p + 4 > end) return false; dest = read_f32(p); p += 4; return true; }
      return skip_value(p, end, val_type);
    };
    auto try_string = [&](std::string& dest) {
      if (val_type == 8) { dest = read_string_val(p, end); return true; }
      return skip_value(p, end, val_type);
    };
    auto try_bool = [&](bool& dest) {
      if (val_type == 7) { if (p + 4 > end) return false; dest = read_u32(p) != 0; p += 4; return true; }
      return skip_value(p, end, val_type);
    };

    auto handled = true;

    // General metadata
    if (key == "general.architecture" && val_type == 8) {
      info.architecture = read_string_val(p, end);
      arch = info.architecture;
    }
    else if (key == "general.name")                               try_string(info.display_name);
    else if (key == "general.size_label")                         try_string(info.size_label);
    else if (key == "general.license")                            try_string(info.license);
    else if (key == "general.author")                             try_string(info.author);
    else if (key == "general.version")                            try_string(info.version);
    else if (key == "general.url")                                try_string(info.url);
    else if (key == "general.source.url")                         try_string(info.source_url);
    else if (key == "general.languages")                         info.languages = read_string_array(p, end, val_type);
    else if (key == "general.file_type" && val_type == 5)         { info.file_type = read_i32(p); p += 4; }
    // Sampling defaults
    else if (key == "general.sampling.temp")                      try_f32(info.sample_temp);
    else if (key == "general.sampling.top_k")                    { float v; try_f32(v); info.sample_top_k = v; }
    else if (key == "general.sampling.top_p")                     try_f32(info.sample_top_p);
    else if (key == "general.sampling.min_p")                     try_f32(info.sample_min_p);
    else if (key == "general.sampling.mirostat")                 { float v; try_f32(v); info.sample_mirostat = v; }
    // Architecture-prefixed
    else if (!arch.empty() && key == arch + ".block_count")       try_u32(info.block_count);
    else if (!arch.empty() && key == arch + ".context_length")    try_u32(info.context_length);
    else if (!arch.empty() && key == arch + ".vocab_size")        try_u32(info.vocab_size);
    else if (!arch.empty() && key == arch + ".embedding_length")  try_u32(info.embedding_length);
    else if (!arch.empty() && key == arch + ".feed_forward_length") try_u32(info.feed_forward_length);
    else if (!arch.empty() && key == arch + ".attention.head_count")     try_u32(info.head_count);
    else if (!arch.empty() && key == arch + ".attention.head_count_kv")  try_u32(info.head_count_kv);
    else if (!arch.empty() && key == arch + ".expert_count")              try_u32(info.expert_count);
    else if (!arch.empty() && key == arch + ".expert_used_count")         try_u32(info.expert_used_count);
    // Tokenizer
    else if (key == "tokenizer.ggml.model")                      try_string(info.tokenizer_model);
    else if (key == "tokenizer.ggml.bos_token_id")                try_i32(info.bos_token_id);
    else if (key == "tokenizer.ggml.eos_token_id")                try_i32(info.eos_token_id);
    else if (key == "tokenizer.chat_template")                    try_string(info.chat_template);
    else if (key == "tokenizer.chat_templates")                  info.chat_templates = read_string_array(p, end, val_type);
    // Multimodal
    else if (key == "clip.has_vision_encoder")                    try_bool(info.has_vision);
    else if (key == "clip.has_audio_encoder")                     try_bool(info.has_audio);
    else {
      handled = false;
    }

    if (!handled) {
      // Skip unknown fields — never bail on arrays
      if (!skip_value(p, end, val_type)) break;
    }
  }
}
