#include <saucer/smartview.hpp>
#include <saucer/embedded/all.hpp>

#include "settings.h"
#include "model_manager.h"
#include "server_manager.h"
#include "preset.h"

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <format>

// Observer that bridges server status → JS via saucer execute
class StatusBridge : public ServerObserver {
  std::shared_ptr<saucer::smartview> webview_;

public:
  explicit StatusBridge(std::shared_ptr<saucer::smartview> wv)
    : webview_(std::move(wv)) {}

  void on_status_change(ServerStatus s) override {
    const char* st = "stopped";
    if (s == ServerStatus::Starting) st = "starting";
    else if (s == ServerStatus::Running) st = "running";
    else if (s == ServerStatus::Error)   st = "error";
    webview_->execute(R"(window.__onServerStatus("{}"))", st);
  }
};

int main(int argc, char* argv[]) {
  auto app = saucer::application::create({.id = "llamancher"});

  bool dev_mode = false;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--dev") dev_mode = true;
  }

  app->run([dev_mode](saucer::application* app) -> coco::stray {
    // ── Shared state ───────────────────────────────────────────────
    auto settings   = std::make_shared<Settings>(Settings::load());
    auto model_mgr  = std::make_shared<ModelManager>(settings->models_dir);

    // ── Create window ──────────────────────────────────────────────
    auto window  = saucer::window::create(app).value();
    auto webview = std::make_shared<saucer::smartview>(
      std::move(saucer::smartview::create({.window = window}).value())
    );

    window->set_title("llamancher");
    window->set_size({.w = 960, .h = 680});
    window->set_min_size({.w = 720, .h = 480});
    window->set_decorations(saucer::window::decoration::partial);
    window->set_background({.r = 10, .g = 10, .b = 10, .a = 255});

    // ── Status bridge (observer) ───────────────────────────────────
    auto bridge = std::make_unique<StatusBridge>(webview);

    // server_mgr declared last so it is destroyed before bridge
    auto server_mgr = std::make_shared<ServerManager>();
    server_mgr->set_observer(bridge.get());

    // ── Exposed API ────────────────────────────────────────────────

    webview->expose("get_settings", [settings]() {
      return std::map<std::string, std::string>{
        {"llama_server_path", settings->llama_server_path},
        {"models_dir",        settings->models_dir},
        {"auto_start_server", settings->auto_start_server ? "true" : "false"},
      };
    });

    webview->expose("update_setting", [settings](const std::string& key,
                                                  const std::string& value) {
      if (key == "llama_server_path") settings->llama_server_path = value;
      else if (key == "models_dir")   settings->models_dir = value;
      else if (key == "auto_start_server") settings->auto_start_server = (value == "true");
      settings->save();
    });

    webview->expose("scan_models", [model_mgr, settings]() {
      *model_mgr = ModelManager(settings->models_dir);
      auto models = model_mgr->scan();

      // For models without a preset, create one with GGUF metadata cached
      for (auto& m : models) {
        if (!Preset::exists(m.name)) {
          Preset p;
          p.architecture = m.architecture;
          p.block_count = m.block_count;
          p.context_length = m.context_length;
          p.file_type = m.file_type;
          p.save(m.name);
        }
      }

      std::vector<std::map<std::string, std::string>> result;
      for (auto& m : models) {
        auto size_gb = static_cast<double>(m.file_size) / (1024.0 * 1024.0 * 1024.0);
        result.push_back({
          {"name",         m.name},
          {"path",         m.path},
          {"architecture", m.architecture},
          {"quantization", m.quantization},
          {"size",         std::format("{:.2f} GB", size_gb)},
          {"block_count",  std::to_string(m.block_count)},
          {"context_length", std::to_string(m.context_length)},
          {"file_type",    std::to_string(m.file_type)},
        });
      }
      return result;
    });

    webview->expose("load_preset", [](const std::string& model_name) {
      auto p = Preset::load(model_name);
      return std::map<std::string, std::string>{
        // Basic
        {"ctx_size",     std::to_string(p.ctx_size)},
        {"threads",      std::to_string(p.threads)},
        // Model & Loading
        {"gpu_layers",   std::to_string(p.gpu_layers)},
        {"tensor_split", p.tensor_split},
        {"numa",         p.numa},
        {"split_mode",   p.split_mode},
        {"main_gpu",     std::to_string(p.main_gpu)},
        {"device",       p.device},
        {"mlock",        p.mlock ? "true" : "false"},
        {"no_mmap",      p.no_mmap ? "true" : "false"},
        {"jinja",        p.jinja ? "true" : "false"},
        {"grammar",      p.grammar},
        {"grammar_file", p.grammar_file},
        {"json_schema",  p.json_schema},
        // Context & Cache
        {"batch_size",   std::to_string(p.batch_size)},
        {"ubatch_size",  std::to_string(p.ubatch_size)},
        {"cache_type_k", p.cache_type_k},
        {"cache_type_v", p.cache_type_v},
        {"flash_attn",   p.flash_attn ? "true" : "false"},
        {"defrag_thold", std::to_string(p.defrag_thold)},
        // Sampling
        {"samplers",          p.samplers},
        {"seed",              std::to_string(p.seed)},
        {"temp",              std::to_string(p.temp)},
        {"top_k",             std::to_string(p.top_k)},
        {"top_p",             std::to_string(p.top_p)},
        {"min_p",             std::to_string(p.min_p)},
        {"repeat_penalty",    std::to_string(p.repeat_penalty)},
        {"presence_penalty",  std::to_string(p.presence_penalty)},
        {"frequency_penalty", std::to_string(p.frequency_penalty)},
        {"mirostat",          std::to_string(p.mirostat)},
        // Server
        {"parallel",   std::to_string(p.parallel)},
        {"no_repack",  p.no_repack ? "true" : "false"},
        // Logging
        {"verbose",    p.verbose ? "true" : "false"},
        {"verbosity",  std::to_string(p.verbosity)},
        {"log_file",   p.log_file},
        // Speculative decoding
        {"spec_type",             p.spec_type},
        {"spec_draft_n_max",      std::to_string(p.spec_draft_n_max)},
        {"spec_draft_n_min",      std::to_string(p.spec_draft_n_min)},
        {"spec_draft_p_split",    std::to_string(p.spec_draft_p_split)},
        {"draft_model",           p.draft_model},
        {"draft_gpu_layers",      std::to_string(p.draft_gpu_layers)},
        {"threads_draft",         std::to_string(p.threads_draft)},
        {"threads_batch_draft",   std::to_string(p.threads_batch_draft)},
        {"spec_draft_poll",       p.spec_draft_poll ? "true" : "false"},
        // N-gram params
        {"spec_ngram_mod_n_min",      std::to_string(p.spec_ngram_mod_n_min)},
        {"spec_ngram_mod_n_max",      std::to_string(p.spec_ngram_mod_n_max)},
        {"spec_ngram_mod_n_match",    std::to_string(p.spec_ngram_mod_n_match)},
        {"spec_ngram_simple_size_n",  std::to_string(p.spec_ngram_simple_size_n)},
        {"spec_ngram_simple_size_m",  std::to_string(p.spec_ngram_simple_size_m)},
        {"spec_ngram_simple_min_hits", std::to_string(p.spec_ngram_simple_min_hits)},
        {"spec_ngram_map_k_size_n",   std::to_string(p.spec_ngram_map_k_size_n)},
        {"spec_ngram_map_k_size_m",   std::to_string(p.spec_ngram_map_k_size_m)},
        {"spec_ngram_map_k_min_hits", std::to_string(p.spec_ngram_map_k_min_hits)},
        {"spec_ngram_map_k4v_size_n",  std::to_string(p.spec_ngram_map_k4v_size_n)},
        {"spec_ngram_map_k4v_size_m",  std::to_string(p.spec_ngram_map_k4v_size_m)},
        {"spec_ngram_map_k4v_min_hits", std::to_string(p.spec_ngram_map_k4v_min_hits)},
        // Auto-fit
        {"fit",            p.fit ? "true" : "false"},
        {"fit_target_mib", p.fit_target_mib},
        {"fit_ctx",        std::to_string(p.fit_ctx)},
      };
    });

    webview->expose("save_preset", [](const std::string& model_name,
                                       const std::map<std::string, std::string>& kv) {
      auto p = Preset::load(model_name);
      auto gi = [&](const std::string& k, int& v) {
        if (kv.contains(k)) v = std::stoi(kv.at(k));
      };
      auto gf = [&](const std::string& k, float& v) {
        if (kv.contains(k)) v = std::stof(kv.at(k));
      };
      auto gs = [&](const std::string& k, std::string& v) {
        if (kv.contains(k)) v = kv.at(k);
      };
      auto gb = [&](const std::string& k, bool& v) {
        if (kv.contains(k)) v = (kv.at(k) == "true");
      };

      // Basic
      gi("ctx_size",     p.ctx_size);
      gi("threads",      p.threads);
      // Model & Loading
      gi("gpu_layers",   p.gpu_layers);
      gs("tensor_split", p.tensor_split);
      gs("numa",         p.numa);
      gs("split_mode",   p.split_mode);
      gi("main_gpu",     p.main_gpu);
      gs("device",       p.device);
      gb("mlock",        p.mlock);
      gb("no_mmap",      p.no_mmap);
      gb("jinja",        p.jinja);
      gs("grammar",      p.grammar);
      gs("grammar_file", p.grammar_file);
      gs("json_schema",  p.json_schema);
      // Context & Cache
      gi("batch_size",   p.batch_size);
      gi("ubatch_size",  p.ubatch_size);
      gs("cache_type_k", p.cache_type_k);
      gs("cache_type_v", p.cache_type_v);
      gb("flash_attn",   p.flash_attn);
      gi("defrag_thold", p.defrag_thold);
      // Sampling
      gs("samplers",          p.samplers);
      gi("seed",              p.seed);
      gf("temp",              p.temp);
      gi("top_k",             p.top_k);
      gf("top_p",             p.top_p);
      gf("min_p",             p.min_p);
      gf("repeat_penalty",    p.repeat_penalty);
      gf("presence_penalty",  p.presence_penalty);
      gf("frequency_penalty", p.frequency_penalty);
      gi("mirostat",          p.mirostat);
      // Server
      gi("parallel",   p.parallel);
      gb("no_repack",  p.no_repack);
      // Logging
      gb("verbose",    p.verbose);
      gi("verbosity",  p.verbosity);
      gs("log_file",   p.log_file);
      // Speculative decoding
      gs("spec_type",             p.spec_type);
      gi("spec_draft_n_max",      p.spec_draft_n_max);
      gi("spec_draft_n_min",      p.spec_draft_n_min);
      gf("spec_draft_p_split",    p.spec_draft_p_split);
      gs("draft_model",           p.draft_model);
      gi("draft_gpu_layers",      p.draft_gpu_layers);
      gi("threads_draft",         p.threads_draft);
      gi("threads_batch_draft",   p.threads_batch_draft);
      gb("spec_draft_poll",       p.spec_draft_poll);
      // N-gram
      gi("spec_ngram_mod_n_min",       p.spec_ngram_mod_n_min);
      gi("spec_ngram_mod_n_max",       p.spec_ngram_mod_n_max);
      gi("spec_ngram_mod_n_match",     p.spec_ngram_mod_n_match);
      gi("spec_ngram_simple_size_n",   p.spec_ngram_simple_size_n);
      gi("spec_ngram_simple_size_m",   p.spec_ngram_simple_size_m);
      gi("spec_ngram_simple_min_hits", p.spec_ngram_simple_min_hits);
      gi("spec_ngram_map_k_size_n",    p.spec_ngram_map_k_size_n);
      gi("spec_ngram_map_k_size_m",    p.spec_ngram_map_k_size_m);
      gi("spec_ngram_map_k_min_hits",  p.spec_ngram_map_k_min_hits);
      gi("spec_ngram_map_k4v_size_n",   p.spec_ngram_map_k4v_size_n);
      gi("spec_ngram_map_k4v_size_m",   p.spec_ngram_map_k4v_size_m);
      gi("spec_ngram_map_k4v_min_hits", p.spec_ngram_map_k4v_min_hits);
      // Auto-fit
      gb("fit",            p.fit);
      gs("fit_target_mib", p.fit_target_mib);
      gi("fit_ctx",        p.fit_ctx);

      p.save(model_name);
    });

    webview->expose("server_status", [server_mgr]() {
      auto s = server_mgr->status();
      std::string st = "stopped";
      if (s == ServerStatus::Starting) st = "starting";
      else if (s == ServerStatus::Running) st = "running";
      else if (s == ServerStatus::Error)   st = "error";
      return std::map<std::string, std::string>{
        {"status", st},
        {"model",  server_mgr->current_model()},
      };
    });

    webview->expose("start_server", [server_mgr, settings](
                                     const std::string& model_name,
                                     const std::string& model_path) {
      if (server_mgr->status() != ServerStatus::Stopped) {
        return std::string("server_not_stopped");
      }
      auto preset = Preset::load(model_name);
      auto args = preset.cli_args(model_path);
      args.insert(args.end(), {"--host", "127.0.0.1", "--port", "8080"});
      server_mgr->start(settings->llama_server_path, args);
      return std::string("ok");
    });

    webview->expose("stop_server", [server_mgr]() {
      server_mgr->stop();
    });

    // ── Load frontend ──────────────────────────────────────────────
    if (dev_mode) {
      webview->set_url("http://127.0.0.1:5173");
    } else {
      webview->embed(saucer::embedded::all());
      webview->serve("/index.html");
    }

    window->show();
    co_await app->finish();
  });
}
