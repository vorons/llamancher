#include "preset.h"
#include <cstdlib>
#include <cstdio>
#include <glaze/file/file_ops.hpp>
#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

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

  // ponytail: if the file is corrupt we silently return defaults. Upgrade to
  // user-visible error when the UI has toast infrastructure.
  auto ec = glz::read_file_json(p, fp.string(), std::string{});
  if (ec) {
    std::fprintf(stderr, "llamancher: corrupt preset %s — %s\n",
                 model_name.c_str(), glz::format_error(ec).c_str());
    return Preset{};
  }
  return p;
}

void Preset::save(const std::string& model_name) const {
  auto fp = path(model_name);
  fs::create_directories(fp.parent_path());

  auto ec = glz::write_file_json(*this, fp.string(), std::string{});
  if (ec) {
    std::fprintf(stderr, "llamancher: failed to save preset %s — %s\n",
                 model_name.c_str(), glz::format_error(ec).c_str());
  }
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
