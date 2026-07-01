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
  std::vector<std::string> args;
  args.reserve(64);

  args.push_back("--model");
  args.push_back(model_path);

  // ─── Основные ──────────────────────────────────────────
  if (ctx_size != 2048) {
    args.push_back("--ctx-size");
    args.push_back(std::to_string(ctx_size));
  }
  if (threads != 4) {
    args.push_back("--threads");
    args.push_back(std::to_string(threads));
  }
  if (threads_batch > 0) {
    args.push_back("--threads-batch");
    args.push_back(std::to_string(threads_batch));
  }
  if (gpu_layers > 0) {
    args.push_back("--n-gpu-layers");
    args.push_back(std::to_string(gpu_layers));
  }
  if (batch_size != 2048) {
    args.push_back("--batch-size");
    args.push_back(std::to_string(batch_size));
  }
  if (ubatch_size != 512) {
    args.push_back("--ubatch-size");
    args.push_back(std::to_string(ubatch_size));
  }
  if (!mmproj.empty()) {
    args.push_back("--mmproj");
    args.push_back(mmproj);
  }
  if (!cache_type_k.empty()) {
    args.push_back("--cache-type-k");
    args.push_back(cache_type_k);
  }
  if (!cache_type_v.empty()) {
    args.push_back("--cache-type-v");
    args.push_back(cache_type_v);
  }
  if (parallel != 1) {
    args.push_back("--parallel");
    args.push_back(std::to_string(parallel));
  }
  if (timeout > 0) {
    args.push_back("--timeout");
    args.push_back(std::to_string(timeout));
  }
  if (seed >= 0) {
    args.push_back("--seed");
    args.push_back(std::to_string(seed));
  }
  if (flash_attn)  args.emplace_back("--flash-attn");
  if (mlock)       args.emplace_back("--mlock");
  if (no_mmap)     args.emplace_back("--no-mmap");

  // ─── Генерация ──────────────────────────────────────────
  if (temp != 0.80f) {
    args.push_back("--temp");
    args.push_back(std::to_string(temp));
  }
  if (predict >= 0) {
    args.push_back("--predict");
    args.push_back(std::to_string(predict));
  }
  if (min_p != 0.05f) {
    args.push_back("--min-p");
    args.push_back(std::to_string(min_p));
  }
  if (top_k != 40) {
    args.push_back("--top-k");
    args.push_back(std::to_string(top_k));
  }
  if (top_p != 0.95f) {
    args.push_back("--top-p");
    args.push_back(std::to_string(top_p));
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
  if (reasoning_mode) args.emplace_back("--reasoning");
  if (reasoning_budget > 0) {
    args.push_back("--reasoning-budget");
    args.push_back(std::to_string(reasoning_budget));
  }

  // ─── Спекулятивное декодирование ────────────────────────
  if (!spec_type.empty()) {
    args.push_back("--spec-type");
    args.push_back(spec_type);
  }
  if (!draft_model.empty()) {
    args.push_back("--draft-model");
    args.push_back(draft_model);
  }
  if (!hf_repo_draft.empty()) {
    args.push_back("--hf-repo-draft");
    args.push_back(hf_repo_draft);
  }
  if (draft_gpu_layers != 0) {
    args.push_back("--draft-gpu-layers");
    args.push_back(std::to_string(draft_gpu_layers));
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
  if (spec_draft_p_min != 0.0f) {
    args.push_back("--spec-draft-p-min");
    args.push_back(std::to_string(spec_draft_p_min));
  }

  // ─── Дополнительные ─────────────────────────────────────
  if (!cont_batching) args.emplace_back("--no-cont-batching");
  if (!webui)         args.emplace_back("--no-webui");
  if (embedding)      args.emplace_back("--embedding");
  if (!slots)         args.emplace_back("--no-slots");
  if (metrics)        args.emplace_back("--metrics");
  if (!cache_prompt)  args.emplace_back("--no-cache-prompt");
  if (!context_shift) args.emplace_back("--no-context-shift");
  if (!alias.empty()) {
    args.push_back("--alias");
    args.push_back(alias);
  }

  // ─── Custom CLI args ────────────────────────────────────
  for (const auto& ca : custom_args) {
    if (ca.enabled) {
      args.push_back(ca.arg);
    }
  }

  return args;
}
