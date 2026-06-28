#include "settings.h"
#include <cstdlib>
#include <cstdio>
#include <glaze/file/file_ops.hpp>
#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace fs = std::filesystem;

Settings::Settings() {
  auto home = getenv("HOME");
  models_dir = (home ? fs::path(home) : fs::path("/tmp")) / ".llamancher" / "models";
}

fs::path Settings::path() const {
  auto home = getenv("HOME");
  return (home ? fs::path(home) : fs::path("/tmp")) / ".config" / "llamancher" / "settings.json";
}

Settings Settings::load() {
  Settings s;
  auto p = s.path();
  if (!fs::exists(p)) return s;

  // Glaze reads JSON directly into the struct with proper error handling.
  // If the file is malformed or missing fields, defaults remain.
  auto ec = glz::read_file_json(s, p.string(), std::string{});
  if (ec) {
    // ponytail: silent fallback on corrupt file. Upgrade to user-facing toast
    // when settings UI has error reporting.
    return Settings{};
  }
  return s;
}

void Settings::save() const {
  fs::create_directories(path().parent_path());
  // ponytail: writes compact JSON, not pretty-printed. If human readability
  // matters, switch to glz::write_pretty_json with indent=2.
  auto ec = glz::write_file_json(*this, path().string(), std::string{});
  if (ec) {
    std::fprintf(stderr, "llamancher: failed to save settings — %s\n",
                 glz::format_error(ec).c_str());
  }
}
