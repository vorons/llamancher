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
    auto server_mgr = std::make_shared<ServerManager>();

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
      std::vector<std::map<std::string, std::string>> result;
      for (auto& m : models) {
        auto size_gb = static_cast<double>(m.file_size) / (1024.0 * 1024.0 * 1024.0);
        result.push_back({
          {"name",         m.name},
          {"path",         m.path},
          {"quantization", m.quantization},
          {"size",         std::format("{:.2f} GB", size_gb)},
        });
      }
      return result;
    });

    webview->expose("load_preset", [](const std::string& model_name) {
      auto p = Preset::load(model_name);
      return std::map<std::string, std::string>{
        {"ctx_size",     std::to_string(p.ctx_size)},
        {"threads",      std::to_string(p.threads)},
        {"gpu_layers",   std::to_string(p.gpu_layers)},
        {"tensor_split", p.tensor_split},
        {"temp",         std::to_string(p.temp)},
        {"top_k",        std::to_string(p.top_k)},
        {"top_p",        std::to_string(p.top_p)},
        {"mlock",        p.mlock ? "true" : "false"},
        {"no_mmap",      p.no_mmap ? "true" : "false"},
        {"flash_attn",   p.flash_attn ? "true" : "false"},
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

      gi("ctx_size",     p.ctx_size);
      gi("threads",      p.threads);
      gi("gpu_layers",   p.gpu_layers);
      gs("tensor_split", p.tensor_split);
      gf("temp",         p.temp);
      gi("top_k",        p.top_k);
      gf("top_p",        p.top_p);
      gb("mlock",        p.mlock);
      gb("no_mmap",      p.no_mmap);
      gb("flash_attn",   p.flash_attn);

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
