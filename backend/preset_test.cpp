#include "preset.h"
#include <cstdio>
#include <cstdlib>
#include <glaze/glaze.hpp>

int main() {
  // ── Test 1: roundtrip through Glaze ──
  {
    Preset p;
    p.ctx_size = 4096;
    p.threads = 8;
    p.gpu_layers = 35;
    p.temp = 0.7f;
    p.top_k = 50;
    p.top_p = 0.9f;
    p.mlock = true;
    p.flash_attn = true;
    p.architecture = "llama";
    p.block_count = 32;

    auto json = glz::write_json(p);
    if (!json) {
      std::fprintf(stderr, "FAIL: write_json error\n");
      return 1;
    }

    Preset p2;
    auto ec = glz::read_json(p2, json.value());
    if (ec) {
      std::fprintf(stderr, "FAIL: read_json error — %s\n",
                   glz::format_error(ec).c_str());
      return 1;
    }

    if (p2.ctx_size != 4096 || p2.threads != 8 || p2.gpu_layers != 35 ||
        p2.temp != 0.7f || p2.top_k != 50 || p2.top_p != 0.9f ||
        p2.mlock != true || p2.flash_attn != true ||
        p2.architecture != "llama" || p2.block_count != 32) {
      std::fprintf(stderr, "FAIL: roundtrip values mismatch\n");
      return 1;
    }
  }

  // ── Test 2: backward compat with old hand-written format ──
  {
    const char* old_json = R"({
  "ctx_size": 2048,
  "threads": 4,
  "gpu_layers": 0,
  "tensor_split": "",
  "numa": "",
  "split_mode": "",
  "main_gpu": 0,
  "device": "",
  "mlock": false,
  "no_mmap": false,
  "jinja": true,
  "grammar": "",
  "grammar_file": "",
  "json_schema": "",
  "batch_size": 2048,
  "ubatch_size": 512,
  "cache_type_k": "",
  "cache_type_v": "",
  "flash_attn": false,
  "defrag_thold": -1,
  "samplers": "",
  "seed": -1,
  "temp": 0.80,
  "top_k": 40,
  "top_p": 0.95,
  "min_p": 0.05,
  "repeat_penalty": 1.00,
  "presence_penalty": 0.00,
  "frequency_penalty": 0.00,
  "mirostat": 0,
  "parallel": 1,
  "no_repack": false,
  "verbose": false,
  "verbosity": 0,
  "log_file": "",
  "spec_type": "",
  "spec_draft_n_max": 16,
  "spec_draft_n_min": 0,
  "spec_draft_p_split": 0.50,
  "draft_model": "",
  "draft_gpu_layers": 0,
  "threads_draft": 0,
  "threads_batch_draft": 0,
  "spec_draft_poll": false,
  "spec_ngram_mod_n_min": 48,
  "spec_ngram_mod_n_max": 64,
  "spec_ngram_mod_n_match": 24,
  "spec_ngram_simple_size_n": 12,
  "spec_ngram_simple_size_m": 48,
  "spec_ngram_simple_min_hits": 1,
  "spec_ngram_map_k_size_n": 12,
  "spec_ngram_map_k_size_m": 48,
  "spec_ngram_map_k_min_hits": 1,
  "spec_ngram_map_k4v_size_n": 12,
  "spec_ngram_map_k4v_size_m": 48,
  "spec_ngram_map_k4v_min_hits": 1,
  "fit": true,
  "fit_target_mib": "",
  "fit_ctx": 4096,
  "architecture": "",
  "block_count": 0,
  "context_length": 0,
  "file_type": -1
})";
    Preset p;
    auto ec = glz::read_json(p, old_json);
    if (ec) {
      std::fprintf(stderr, "FAIL: backward compat read error — %s\n",
                   glz::format_error(ec).c_str());
      return 1;
    }
    // Spot-check a few fields
    if (p.threads != 4 || p.gpu_layers != 0 || p.temp != 0.80f ||
        p.jinja != true || p.fit != true || p.seed != -1) {
      std::fprintf(stderr, "FAIL: backward compat values\n");
      return 1;
    }
  }

  // ── Test 3: missing fields → defaults from struct ──
  {
    const char* partial = R"({"ctx_size": 8192, "gpu_layers": 99})";
    Preset p;
    auto ec = glz::read_json(p, partial);
    if (ec) {
      std::fprintf(stderr, "FAIL: partial read error — %s\n",
                   glz::format_error(ec).c_str());
      return 1;
    }
    if (p.ctx_size != 8192 || p.gpu_layers != 99 || p.threads != 4 ||
        p.temp != 0.80f || p.fit != true) {
      std::fprintf(stderr, "FAIL: partial missing field defaults (%d %d %d)\n",
                   p.ctx_size, p.gpu_layers, p.threads);
      return 1;
    }
  }

  // ── Test 4: corrupt → defaults ──
  {
    Preset p;
    p.threads = 42; // should be reset
    auto ec = glz::read_json(p, R"({broken)");
    if (!ec) {
      std::fprintf(stderr, "FAIL: corrupt should error\n");
      return 1;
    }
  }

  std::printf("PASS\n");
  return 0;
}
