#include "preset.h"
#include <fstream>
#include <cstdlib>
#include <format>

namespace fs = std::filesystem;

fs::path Preset::path(const std::string& model_name) const {
  auto home = getenv("HOME");
  auto base = fs::path(home ? home : "/tmp") / ".llamancher" / "models";
  return base / (model_name + ".preset.json");
}

Preset Preset::load(const std::string& model_name) {
  Preset p;
  auto fp = p.path(model_name);
  if (!fs::exists(fp)) return p;

  std::ifstream f(fp);
  std::string buf((std::istreambuf_iterator<char>(f)), {});

  // ponytail: naive parse. Same caveat as settings.cpp.
  auto qs = [&](const char* key) -> std::string {
    auto k = std::string("\"") + key + "\":\"";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return {};
    pos += k.size();
    auto end = buf.find("\"", pos);
    return buf.substr(pos, end - pos);
  };
  auto qi = [&](const char* key, int def) -> int {
    auto k = std::string("\"") + key + "\":";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return def;
    pos += k.size();
    auto end = buf.find_first_of(",\n}", pos);
    return std::stoi(buf.substr(pos, end - pos));
  };
  auto qf = [&](const char* key, float def) -> float {
    auto k = std::string("\"") + key + "\":";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return def;
    pos += k.size();
    auto end = buf.find_first_of(",\n}", pos);
    return std::stof(buf.substr(pos, end - pos));
  };
  auto qb = [&](const char* key) -> bool {
    auto k = std::string("\"") + key + "\":";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return false;
    return buf.substr(pos + k.size(), 4) == "true";
  };

  p.ctx_size = qi("ctx_size", 2048);
  p.threads = qi("threads", 4);
  p.gpu_layers = qi("gpu_layers", 0);
  p.tensor_split = qs("tensor_split");
  p.temp = qf("temp", 0.7f);
  p.top_k = qi("top_k", 40);
  p.top_p = qf("top_p", 0.9f);
  p.mlock = qb("mlock");
  p.no_mmap = qb("no_mmap");
  p.flash_attn = qb("flash_attn");

  return p;
}

void Preset::save(const std::string& model_name) const {
  auto fp = path(model_name);
  fs::create_directories(fp.parent_path());
  std::ofstream f(fp);
  f << std::format(
    R"({{
  "ctx_size": {},
  "threads": {},
  "gpu_layers": {},
  "tensor_split": "{}",
  "temp": {},
  "top_k": {},
  "top_p": {},
  "mlock": {},
  "no_mmap": {},
  "flash_attn": {}
}})",
    ctx_size, threads, gpu_layers, tensor_split,
    temp, top_k, top_p,
    mlock ? "true" : "false",
    no_mmap ? "true" : "false",
    flash_attn ? "true" : "false"
  );
}

std::vector<std::string> Preset::cli_args(const std::string& model_path) const {
  std::vector<std::string> args = {
    "--model", model_path,
    "--ctx-size", std::to_string(ctx_size),
    "--threads", std::to_string(threads),
    "--temp", std::to_string(temp),
    "--top-k", std::to_string(top_k),
    "--top-p", std::to_string(top_p),
  };
  if (gpu_layers > 0) {
    args.push_back("--n-gpu-layers");
    args.push_back(std::to_string(gpu_layers));
  }
  if (!tensor_split.empty()) {
    args.push_back("--tensor-split");
    args.push_back(tensor_split);
  }
  if (mlock)    args.emplace_back("--mlock");
  if (no_mmap)  args.emplace_back("--no-mmap");
  if (flash_attn) args.emplace_back("--flash-attn");
  return args;
}
