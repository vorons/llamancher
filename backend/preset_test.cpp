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
    p.seed = 42;
    p.predict = 512;
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
        p2.seed != 42 || p2.predict != 512 ||
        p2.architecture != "llama" || p2.block_count != 32) {
      std::fprintf(stderr, "FAIL: roundtrip values mismatch\n");
      return 1;
    }
  }

  // ── Test 2: backward compat — glaze silently skips unknown fields ──
  {
    const char* old_json = R"({
  "ctx_size": 2048,
  "threads": 4,
  "temp": 0.80
})";
    Preset p;
    auto ec = glz::read_json(p, old_json);
    if (ec) {
      std::fprintf(stderr, "FAIL: backward compat read error — %s\n",
                   glz::format_error(ec).c_str());
      return 1;
    }
    // Defaults for fields not in the JSON
    if (p.threads != 4 || p.temp != 0.80f || p.top_k != 40 ||
        p.gpu_layers != 0 || p.seed != -1 || p.predict != -1) {
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
        p.temp != 0.80f) {
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

  // ── Test 5: cli_args — non-default fields ──
  {
    Preset p;
    p.ctx_size = 4096;
    p.threads = 8;
    p.gpu_layers = 20;
    p.flash_attn = true;
    p.mlock = true;
    p.temp = 0.80f; // default — should not appear
    p.top_k = 50;
    p.predict = -1; // default, should not appear
    p.seed = 42;
    p.no_mmap = true;
    p.metrics = true;
    p.cont_batching = false;
    p.alias = "test-model";
    p.custom_args.push_back({"--some-flag", true});

    auto args = p.cli_args("/path/to/model.gguf");
    // Basic validation: model path always first two args
    if (args.size() < 2 || args[0] != "--model" || args[1] != "/path/to/model.gguf") {
      std::fprintf(stderr, "FAIL: cli_args missing model path\n");
      return 1;
    }

    // Spot-check specific flags
    bool has_ctx = false, has_flash = false, has_mlock = false;
    bool has_nommap = false, has_metrics = false, has_no_cont = false;
    bool has_alias = false, has_seed = false, has_gpu = false;
    bool has_custom = false;
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i] == "--ctx-size" && i + 1 < args.size() && args[i+1] == "4096") has_ctx = true;
      if (args[i] == "--flash-attn") has_flash = true;
      if (args[i] == "--mlock") has_mlock = true;
      if (args[i] == "--no-mmap") has_nommap = true;
      if (args[i] == "--metrics") has_metrics = true;
      if (args[i] == "--no-cont-batching") has_no_cont = true;
      if (args[i] == "--alias" && i + 1 < args.size() && args[i+1] == "test-model") has_alias = true;
      if (args[i] == "--seed" && i + 1 < args.size() && args[i+1] == "42") has_seed = true;
      if (args[i] == "--n-gpu-layers" && i + 1 < args.size() && args[i+1] == "20") has_gpu = true;
      if (args[i] == "--some-flag") has_custom = true;
    }

    if (!has_ctx || !has_flash || !has_mlock || !has_nommap ||
        !has_metrics || !has_no_cont || !has_alias || !has_seed || !has_gpu || !has_custom) {
      std::fprintf(stderr, "FAIL: cli_args missing expected flags\n");
      return 1;
    }

    // Should NOT have --predict (default -1), --parallel (default 1), --top-p (default 0.95)
    for (const auto& a : args) {
      if (a == "--predict" || a == "--parallel" || a == "--top-p") {
        std::fprintf(stderr, "FAIL: cli_args should not emit default flags (%s)\n", a.c_str());
        return 1;
      }
    }
  }

  std::printf("PASS\n");
  return 0;
}
