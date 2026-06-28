#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct ModelInfo {
  std::string name;                // filename stem (fallback if no general.name)
  std::string path;                // full path
  std::string quantization;        // e.g. "Q4_K_M"

  // Architecture (already read)
  std::string architecture;        // e.g. "llama", "mistral", "phi3"
  uint32_t    block_count = 0;     // layers
  uint32_t    context_length = 0;  // max context
  int32_t     file_type = -1;      // GGUF enum

  // New metadata
  std::string display_name;        // general.name
  std::string size_label;          // general.size_label ("7B", "70B")
  std::string license;             // general.license
  std::string author;              // general.author
  std::string version;             // general.version
  std::string url;                 // general.url
  std::string source_url;          // general.source.url
  std::string languages;           // general.languages (comma-joined)
  uint32_t    vocab_size = 0;      // {arch}.vocab_size
  uint32_t    embedding_length = 0;// {arch}.embedding_length
  uint32_t    feed_forward_length = 0; // {arch}.feed_forward_length
  uint32_t    head_count = 0;      // {arch}.attention.head_count
  uint32_t    head_count_kv = 0;   // {arch}.attention.head_count_kv
  uint32_t    expert_count = 0;    // {arch}.expert_count
  uint32_t    expert_used_count = 0; // {arch}.expert_used_count
  std::string tokenizer_model;     // tokenizer.ggml.model
  int32_t     bos_token_id = -1;   // tokenizer.ggml.bos_token_id
  int32_t     eos_token_id = -1;   // tokenizer.ggml.eos_token_id
  std::string chat_template;       // tokenizer.chat_template
  std::string chat_templates;      // tokenizer.chat_templates (comma-joined)
  bool        has_vision = false;  // clip.has_vision_encoder
  bool        has_audio = false;   // clip.has_audio_encoder

  // Recommended sampling defaults (general.sampling.*)
  // 0.0 = not set in GGUF, use programmatic default
  float sample_temp = 0.0f;
  float sample_top_k = 0.0f;
  float sample_top_p = 0.0f;
  float sample_min_p = 0.0f;
  float sample_mirostat = 0.0f;

  uint64_t    file_size = 0;
};

class ModelManager {
public:
  explicit ModelManager(const std::string& models_dir);

  std::vector<ModelInfo> scan() const;
  std::string            guess_quantization(const std::string& filename) const;

private:
  void read_gguf_metadata(const std::filesystem::path& path, ModelInfo& info) const;

  std::filesystem::path dir_;
};
