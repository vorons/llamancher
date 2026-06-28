#include "settings.h"
#include <cstdio>
#include <cstdlib>
#include <glaze/glaze.hpp>

int main() {
  // ── Test 1: roundtrip through Glaze ──
  {
    Settings s;
    s.llama_server_path = "/usr/local/bin/llama-server";
    s.models_dir = "/home/user/.llamancher/models";
    s.auto_start_server = true;

    auto json = glz::write_json(s);
    if (!json) {
      std::fprintf(stderr, "FAIL: write_json error\n");
      return 1;
    }

    Settings s2;
    auto ec = glz::read_json(s2, json.value());
    if (ec) {
      std::fprintf(stderr, "FAIL: read_json error — %s\n",
                   glz::format_error(ec).c_str());
      return 1;
    }

    if (s2.llama_server_path != s.llama_server_path ||
        s2.models_dir != s.models_dir ||
        s2.auto_start_server != s.auto_start_server) {
      std::fprintf(stderr, "FAIL: roundtrip mismatch\n");
      return 1;
    }
  }

  // ── Test 2: backward compat (old format) ──
  {
    const char* old_json = R"({
  "llama_server_path": "/old/path",
  "models_dir": "/old/models",
  "auto_start_server": false
})";
    Settings s;
    auto ec = glz::read_json(s, old_json);
    if (ec) {
      std::fprintf(stderr, "FAIL: backward compat read error — %s\n",
                   glz::format_error(ec).c_str());
      return 1;
    }
    if (s.llama_server_path != "/old/path" ||
        s.models_dir != "/old/models" ||
        s.auto_start_server != false) {
      std::fprintf(stderr, "FAIL: backward compat values mismatch\n");
      return 1;
    }
  }

  // ── Test 3: corrupt file → defaults ──
  {
    Settings s;
    auto ec = glz::read_json(s, R"({broken json)");
    if (!ec) {
      // should have failed
      std::fprintf(stderr, "FAIL: corrupt json should error\n");
      return 1;
    }
  }

  std::printf("PASS\n");
  return 0;
}
