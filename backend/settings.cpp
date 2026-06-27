#include "settings.h"
#include <fstream>
#include <cstdlib>

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

  std::ifstream f(p);
  std::string buf((std::istreambuf_iterator<char>(f)), {});

  // ponytail: naive JSON parse — no error recovery. Upgrade to glaze/nlohmann if fields grow.
  auto q = [&](const char* key) -> std::string {
    auto k = std::string("\"") + key + "\":\"";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return {};
    pos += k.size();
    auto end = buf.find("\"", pos);
    return buf.substr(pos, end - pos);
  };
  auto b = [&](const char* key) -> bool {
    auto k = std::string("\"") + key + "\":";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return false;
    return buf.substr(pos + k.size(), 4) == "true";
  };

  auto sp = q("llama_server_path");
  if (!sp.empty()) s.llama_server_path = sp;
  auto md = q("models_dir");
  if (!md.empty()) s.models_dir = md;
  s.auto_start_server = b("auto_start_server");
  return s;
}

void Settings::save() const {
  fs::create_directories(path().parent_path());
  std::ofstream f(path());
  f << "{\n";
  f << "  \"llama_server_path\": \"" << llama_server_path << "\",\n";
  f << "  \"models_dir\": \"" << models_dir << "\",\n";
  f << "  \"auto_start_server\": " << (auto_start_server ? "true" : "false") << "\n";
  f << "}\n";
}
