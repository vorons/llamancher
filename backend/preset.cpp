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

bool Preset::exists(const std::string& model_name) {
  Preset p;
  return fs::exists(p.path(model_name));
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
  auto qu = [&](const char* key, uint32_t def) -> uint32_t {
    auto k = std::string("\"") + key + "\":";
    auto pos = buf.find(k);
    if (pos == std::string::npos) return def;
    pos += k.size();
    auto end = buf.find_first_of(",\n}", pos);
    return static_cast<uint32_t>(std::stoul(buf.substr(pos, end - pos)));
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

  // Model & Loading
  p.gpu_layers = qi("gpu_layers", 0);
  p.tensor_split = qs("tensor_split");
  p.numa = qs("numa");
  p.split_mode = qs("split_mode");
  p.main_gpu = qi("main_gpu", 0);
  p.device = qs("device");
  p.mlock = qb("mlock");
  p.no_mmap = qb("no_mmap");
  p.jinja = qb("jinja");
  p.grammar = qs("grammar");
  p.grammar_file = qs("grammar_file");
  p.json_schema = qs("json_schema");

  // Context & Cache
  p.batch_size = qi("batch_size", 2048);
  p.ubatch_size = qi("ubatch_size", 512);
  p.cache_type_k = qs("cache_type_k");
  p.cache_type_v = qs("cache_type_v");
  p.flash_attn = qb("flash_attn");
  p.defrag_thold = qi("defrag_thold", -1);

  // Sampling
  p.samplers = qs("samplers");
  p.seed = qi("seed", -1);
  p.temp = qf("temp", 0.80f);
  p.top_k = qi("top_k", 40);
  p.top_p = qf("top_p", 0.95f);
  p.min_p = qf("min_p", 0.05f);
  p.repeat_penalty = qf("repeat_penalty", 1.00f);
  p.presence_penalty = qf("presence_penalty", 0.00f);
  p.frequency_penalty = qf("frequency_penalty", 0.00f);
  p.mirostat = qi("mirostat", 0);

  // Server
  p.parallel = qi("parallel", 1);
  p.no_repack = qb("no_repack");

  // Logging
  p.verbose = qb("verbose");
  p.verbosity = qi("verbosity", 0);
  p.log_file = qs("log_file");

  // Speculative decoding
  p.spec_type = qs("spec_type");
  p.spec_draft_n_max = qi("spec_draft_n_max", 16);
  p.spec_draft_n_min = qi("spec_draft_n_min", 0);
  p.spec_draft_p_split = qf("spec_draft_p_split", 0.50f);
  p.draft_model = qs("draft_model");
  p.draft_gpu_layers = qi("draft_gpu_layers", 0);
  p.threads_draft = qi("threads_draft", 0);
  p.threads_batch_draft = qi("threads_batch_draft", 0);
  p.spec_draft_poll = qb("spec_draft_poll");
  p.spec_ngram_mod_n_min = qi("spec_ngram_mod_n_min", 48);
  p.spec_ngram_mod_n_max = qi("spec_ngram_mod_n_max", 64);
  p.spec_ngram_mod_n_match = qi("spec_ngram_mod_n_match", 24);
  p.spec_ngram_simple_size_n = qi("spec_ngram_simple_size_n", 12);
  p.spec_ngram_simple_size_m = qi("spec_ngram_simple_size_m", 48);
  p.spec_ngram_simple_min_hits = qi("spec_ngram_simple_min_hits", 1);
  p.spec_ngram_map_k_size_n = qi("spec_ngram_map_k_size_n", 12);
  p.spec_ngram_map_k_size_m = qi("spec_ngram_map_k_size_m", 48);
  p.spec_ngram_map_k_min_hits = qi("spec_ngram_map_k_min_hits", 1);
  p.spec_ngram_map_k4v_size_n = qi("spec_ngram_map_k4v_size_n", 12);
  p.spec_ngram_map_k4v_size_m = qi("spec_ngram_map_k4v_size_m", 48);
  p.spec_ngram_map_k4v_min_hits = qi("spec_ngram_map_k4v_min_hits", 1);

  // Auto-fit
  p.fit = true; // default
  {
    auto k = std::string("\"fit\":");
    auto pos = buf.find(k);
    if (pos != std::string::npos)
      p.fit = (buf.substr(pos + k.size(), 4) == "true");
  }
  p.fit_target_mib = qs("fit_target_mib");
  p.fit_ctx = qi("fit_ctx", 4096);

  // Metadata fields (optional, may not exist in older presets)
  p.architecture = qs("architecture");
  p.block_count = qu("block_count", 0);
  p.context_length = qu("context_length", 0);
  p.file_type = qi("file_type", -1);

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
  "numa": "{}",
  "split_mode": "{}",
  "main_gpu": {},
  "device": "{}",
  "mlock": {},
  "no_mmap": {},
  "jinja": {},
  "grammar": "{}",
  "grammar_file": "{}",
  "json_schema": "{}",
  "batch_size": {},
  "ubatch_size": {},
  "cache_type_k": "{}",
  "cache_type_v": "{}",
  "flash_attn": {},
  "defrag_thold": {},
  "samplers": "{}",
  "seed": {},
  "temp": {},
  "top_k": {},
  "top_p": {},
  "min_p": {},
  "repeat_penalty": {},
  "presence_penalty": {},
  "frequency_penalty": {},
  "mirostat": {},
  "parallel": {},
  "no_repack": {},
  "verbose": {},
  "verbosity": {},
  "log_file": "{}",
  "spec_type": "{}",
  "spec_draft_n_max": {},
  "spec_draft_n_min": {},
  "spec_draft_p_split": {},
  "draft_model": "{}",
  "draft_gpu_layers": {},
  "threads_draft": {},
  "threads_batch_draft": {},
  "spec_draft_poll": {},
  "spec_ngram_mod_n_min": {},
  "spec_ngram_mod_n_max": {},
  "spec_ngram_mod_n_match": {},
  "spec_ngram_simple_size_n": {},
  "spec_ngram_simple_size_m": {},
  "spec_ngram_simple_min_hits": {},
  "spec_ngram_map_k_size_n": {},
  "spec_ngram_map_k_size_m": {},
  "spec_ngram_map_k_min_hits": {},
  "spec_ngram_map_k4v_size_n": {},
  "spec_ngram_map_k4v_size_m": {},
  "spec_ngram_map_k4v_min_hits": {},
  "fit": {},
  "fit_target_mib": "{}",
  "fit_ctx": {},
  "architecture": "{}",
  "block_count": {},
  "context_length": {},
  "file_type": {}
}})",
    ctx_size, threads,
    gpu_layers, tensor_split,
    numa, split_mode, main_gpu, device,
    mlock ? "true" : "false",
    no_mmap ? "true" : "false",
    jinja ? "true" : "false",
    grammar, grammar_file, json_schema,
    batch_size, ubatch_size,
    cache_type_k, cache_type_v,
    flash_attn ? "true" : "false",
    defrag_thold,
    samplers, seed,
    temp, top_k, top_p,
    min_p, repeat_penalty, presence_penalty, frequency_penalty,
    mirostat,
    parallel,
    no_repack ? "true" : "false",
    verbose ? "true" : "false",
    verbosity, log_file,
    spec_type, spec_draft_n_max, spec_draft_n_min, spec_draft_p_split,
    draft_model, draft_gpu_layers, threads_draft, threads_batch_draft,
    spec_draft_poll ? "true" : "false",
    spec_ngram_mod_n_min,
    spec_ngram_mod_n_max,
    spec_ngram_mod_n_match,
    spec_ngram_simple_size_n,
    spec_ngram_simple_size_m,
    spec_ngram_simple_min_hits,
    spec_ngram_map_k_size_n,
    spec_ngram_map_k_size_m,
    spec_ngram_map_k_min_hits,
    spec_ngram_map_k4v_size_n,
    spec_ngram_map_k4v_size_m,
    spec_ngram_map_k4v_min_hits,
    fit ? "true" : "false",
    fit_target_mib, fit_ctx,
    architecture,
    block_count, context_length, file_type
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

  // Model & Loading
  if (gpu_layers > 0) {
    args.push_back("--n-gpu-layers");
    args.push_back(std::to_string(gpu_layers));
  }
  if (!tensor_split.empty()) {
    args.push_back("--tensor-split");
    args.push_back(tensor_split);
  }
  if (!numa.empty()) {
    args.push_back("--numa");
    args.push_back(numa);
  }
  if (!split_mode.empty()) {
    args.push_back("--split-mode");
    args.push_back(split_mode);
  }
  args.push_back("--main-gpu");
  args.push_back(std::to_string(main_gpu));
  if (!device.empty()) {
    args.push_back("--device");
    args.push_back(device);
  }
  if (mlock)    args.emplace_back("--mlock");
  if (no_mmap)  args.emplace_back("--no-mmap");

  // Jinja & grammar / JSON schema
  if (!jinja) {
    args.emplace_back("--no-jinja");
  }
  if (!grammar.empty()) {
    args.push_back("--grammar");
    args.push_back(grammar);
  }
  if (!grammar_file.empty()) {
    args.push_back("--grammar-file");
    args.push_back(grammar_file);
  }
  if (!json_schema.empty()) {
    args.push_back("--json-schema");
    args.push_back(json_schema);
  }

  // Context & Cache
  if (batch_size != 2048) {
    args.push_back("--batch-size");
    args.push_back(std::to_string(batch_size));
  }
  if (ubatch_size != 512) {
    args.push_back("--ubatch-size");
    args.push_back(std::to_string(ubatch_size));
  }
  if (!cache_type_k.empty()) {
    args.push_back("--cache-type-k");
    args.push_back(cache_type_k);
  }
  if (!cache_type_v.empty()) {
    args.push_back("--cache-type-v");
    args.push_back(cache_type_v);
  }
  if (flash_attn) args.emplace_back("--flash-attn");
  if (defrag_thold >= 0) {
    args.push_back("--defrag-thold");
    args.push_back(std::to_string(defrag_thold));
  }

  // Sampling
  if (!samplers.empty()) {
    args.push_back("--samplers");
    args.push_back(samplers);
  }
  if (seed >= 0) {
    args.push_back("--seed");
    args.push_back(std::to_string(seed));
  }
  if (min_p != 0.05f) {
    args.push_back("--min-p");
    args.push_back(std::to_string(min_p));
  }
  if (repeat_penalty != 1.00f) {
    args.push_back("--repeat-penalty");
    args.push_back(std::to_string(repeat_penalty));
  }
  if (presence_penalty != 0.00f) {
    args.push_back("--presence-penalty");
    args.push_back(std::to_string(presence_penalty));
  }
  if (frequency_penalty != 0.00f) {
    args.push_back("--frequency-penalty");
    args.push_back(std::to_string(frequency_penalty));
  }
  if (mirostat > 0) {
    args.push_back("--mirostat");
    args.push_back(std::to_string(mirostat));
  }

  // Server
  if (parallel > 1) {
    args.push_back("--parallel");
    args.push_back(std::to_string(parallel));
  }
  if (no_repack) args.emplace_back("--no-repack");

  // Logging
  if (verbose)  args.emplace_back("--verbose");
  if (verbosity > 0) {
    args.push_back("--verbosity");
    args.push_back(std::to_string(verbosity));
  }
  if (!log_file.empty()) {
    args.push_back("--log-file");
    args.push_back(log_file);
  }

  // Speculative decoding
  if (!spec_type.empty() && spec_type != "none") {
    args.push_back("--spec-type");
    args.push_back(spec_type);
  }
  if (spec_draft_n_max != 16) {
    args.push_back("--spec-draft-n-max");
    args.push_back(std::to_string(spec_draft_n_max));
  }
  if (spec_draft_n_min != 0) {
    args.push_back("--spec-draft-n-min");
    args.push_back(std::to_string(spec_draft_n_min));
  }
  if (spec_draft_p_split != 0.50f) {
    args.push_back("--spec-draft-p-split");
    args.push_back(std::to_string(spec_draft_p_split));
  }
  if (!draft_model.empty()) {
    args.push_back("--draft-model");
    args.push_back(draft_model);
  }
  if (draft_gpu_layers > 0 || draft_gpu_layers == -1) {
    args.push_back("--draft-gpu-layers");
    args.push_back(std::to_string(draft_gpu_layers));
  }
  if (threads_draft > 0) {
    args.push_back("--threads-draft");
    args.push_back(std::to_string(threads_draft));
  }
  if (threads_batch_draft > 0) {
    args.push_back("--threads-batch-draft");
    args.push_back(std::to_string(threads_batch_draft));
  }
  if (spec_draft_poll) {
    args.push_back("--spec-draft-poll");
    args.push_back("1");
  }

  // N-gram speculative decoding params
  if (spec_type.find("ngram") != std::string::npos) {
    auto push_int = [&](const std::string& flag, int val, int def) {
      if (val != def) {
        args.push_back(flag);
        args.push_back(std::to_string(val));
      }
    };
    push_int("--spec-ngram-mod-n-min",      spec_ngram_mod_n_min, 48);
    push_int("--spec-ngram-mod-n-max",      spec_ngram_mod_n_max, 64);
    push_int("--spec-ngram-mod-n-match",    spec_ngram_mod_n_match, 24);
    push_int("--spec-ngram-simple-size-n",  spec_ngram_simple_size_n, 12);
    push_int("--spec-ngram-simple-size-m",  spec_ngram_simple_size_m, 48);
    push_int("--spec-ngram-simple-min-hits", spec_ngram_simple_min_hits, 1);
    push_int("--spec-ngram-map-k-size-n",   spec_ngram_map_k_size_n, 12);
    push_int("--spec-ngram-map-k-size-m",   spec_ngram_map_k_size_m, 48);
    push_int("--spec-ngram-map-k-min-hits", spec_ngram_map_k_min_hits, 1);
    push_int("--spec-ngram-map-k4v-size-n", spec_ngram_map_k4v_size_n, 12);
    push_int("--spec-ngram-map-k4v-size-m", spec_ngram_map_k4v_size_m, 48);
    push_int("--spec-ngram-map-k4v-min-hits", spec_ngram_map_k4v_min_hits, 1);
  }

  // Auto-fit
  if (!fit) {
    args.push_back("--fit");
    args.push_back("off");
  }
  if (!fit_target_mib.empty()) {
    args.push_back("--fit-target");
    args.push_back(fit_target_mib);
  }
  if (fit_ctx != 4096) {
    args.push_back("--fit-ctx");
    args.push_back(std::to_string(fit_ctx));
  }

  return args;
}
