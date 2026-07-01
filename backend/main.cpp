#include <gtk/gtk.h>
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
#include <array>
#include <cstdio>
#include <fstream>
#include <deque>
#include <glaze/json/write.hpp>
#include <glaze/json/read.hpp>

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
    auto bridge = std::make_shared<StatusBridge>(webview);

    auto server_mgr = std::make_shared<ServerManager>();
    server_mgr->set_observer(bridge);

    // ── Exposed API ────────────────────────────────────────────────

    // ── File pickers (native dialogs, non-blocking) ─────────────────

    // ponytail: uses new GtkFileDialog (GTK 4.10+) instead of deprecated GtkFileChooserNative.
    // The action enum is only used as an internal dispatch key, not passed to deprecated functions.
    auto show_file_dialog = [](GtkWindow* parent, const char* title,
                                GtkFileChooserAction action,
                                saucer::executor<std::string> exec,
                                GtkFileFilter* filter = nullptr) {
      auto* dialog = gtk_file_dialog_new();
      gtk_file_dialog_set_title(dialog, title);

      if (filter) {
        auto* store = g_list_store_new(GTK_TYPE_FILE_FILTER);
        g_list_store_append(store, filter);
        g_object_unref(filter);
        gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(store));
        gtk_file_dialog_set_default_filter(dialog, filter);
        g_object_unref(store);
      }

      struct Cb {
        saucer::executor<std::string> exec;
        GtkFileChooserAction action;
      };
      auto* cb = new Cb{std::move(exec), action};

      auto handler = +[](GObject* source, GAsyncResult* result, gpointer data) {
        auto* cb = static_cast<Cb*>(data);
        GError* error = nullptr;
        GFile* file = nullptr;

        if (cb->action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
          file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
        } else {
          file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
        }

        if (file) {
          char* path = g_file_get_path(file);
          cb->exec.resolve(path ? std::string(path) : std::string{});
          g_free(path);
          g_object_unref(file);
        } else {
          cb->exec.resolve(std::string{});
        }
        g_clear_error(&error);
        delete cb;
      };

      if (action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
        gtk_file_dialog_select_folder(dialog, parent, nullptr, handler, cb);
      } else {
        gtk_file_dialog_open(dialog, parent, nullptr, handler, cb);
      }

      g_object_unref(dialog);
    };

    // ponytail: no parent window (nullptr) avoids GTK header dependency in this TU.
    // The dialog still works — it just won't be strictly modal to the app window.
    webview->expose("pick_file", [show_file_dialog](saucer::executor<std::string> exec) {
      show_file_dialog(nullptr, "Select llama-server executable",
                        GTK_FILE_CHOOSER_ACTION_OPEN, std::move(exec));
    });

    webview->expose("pick_folder", [show_file_dialog](saucer::executor<std::string> exec) {
      show_file_dialog(nullptr, "Select models directory",
                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, std::move(exec));
    });

    webview->expose("set_window_title", [window](const std::string& title) {
      window->set_title(title);
    });

    // ── Settings ──────────────────────────────────────────────────────

    webview->expose("get_settings", [settings]() {
      return std::map<std::string, std::string>{
        {"llama_server_path", settings->llama_server_path},
        {"models_dir",        settings->models_dir},
        {"auto_start_server", settings->auto_start_server ? "true" : "false"},
        {"theme",             settings->theme},
        {"port",              std::to_string(settings->port)},
        {"api_key",           settings->api_key},
        {"last_model",        settings->last_model},
        {"offline",           settings->offline ? "true" : "false"},
      };
    });

    webview->expose("update_setting", [settings](const std::string& key,
                                                  const std::string& value) {
      if (key == "llama_server_path") settings->llama_server_path = value;
      else if (key == "models_dir")   settings->models_dir = value;
      else if (key == "auto_start_server") settings->auto_start_server = (value == "true");
      else if (key == "theme")        settings->theme = value;
      else if (key == "port")         try { settings->port = std::stoi(value); } catch (...) {}
      else if (key == "api_key")      settings->api_key = value;
      else if (key == "last_model")   settings->last_model = value;
      else if (key == "offline")      settings->offline = (value == "true");
      settings->save();
    });

    webview->expose("scan_models", [model_mgr, settings]() {
      *model_mgr = ModelManager(settings->models_dir);
      auto models = model_mgr->scan();

      // For models without a preset, create one with GGUF metadata cached
      for (auto& m : models) {
        if (!Preset::exists(m.name)) {
          Preset p;
          // Populate all metadata
          p.architecture = m.architecture;
          p.block_count = m.block_count;
          p.context_length = m.context_length;
          p.file_type = m.file_type;
          p.display_name = m.display_name;
          p.size_label = m.size_label;
          p.license = m.license;
          p.author = m.author;
          p.version = m.version;
          p.url = m.url;
          p.source_url = m.source_url;
          p.languages = m.languages;
          p.vocab_size = m.vocab_size;
          p.embedding_length = m.embedding_length;
          p.feed_forward_length = m.feed_forward_length;
          p.head_count = m.head_count;
          p.head_count_kv = m.head_count_kv;
          p.expert_count = m.expert_count;
          p.expert_used_count = m.expert_used_count;
          p.tokenizer_model = m.tokenizer_model;
          p.bos_token_id = m.bos_token_id;
          p.eos_token_id = m.eos_token_id;
          p.chat_template = m.chat_template;
          p.chat_templates = m.chat_templates;
          p.has_vision = m.has_vision;
          p.has_audio = m.has_audio;
          // Override sampling defaults from GGUF recommendation
          if (m.sample_temp > 0.001f)   p.temp = m.sample_temp;
          if (m.sample_top_k > 0.001f)  p.top_k = static_cast<int>(m.sample_top_k);
          if (m.sample_top_p > 0.001f)  p.top_p = m.sample_top_p;
          if (m.sample_min_p > 0.001f)  p.min_p = m.sample_min_p;
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
          // Extended metadata
          {"display_name",        m.display_name},
          {"size_label",          m.size_label},
          {"license",             m.license},
          {"author",              m.author},
          {"version",             m.version},
          {"url",                 m.url},
          {"source_url",          m.source_url},
          {"languages",           m.languages},
          {"vocab_size",          std::to_string(m.vocab_size)},
          {"embedding_length",    std::to_string(m.embedding_length)},
          {"feed_forward_length", std::to_string(m.feed_forward_length)},
          {"head_count",          std::to_string(m.head_count)},
          {"head_count_kv",       std::to_string(m.head_count_kv)},
          {"expert_count",        std::to_string(m.expert_count)},
          {"expert_used_count",   std::to_string(m.expert_used_count)},
          {"tokenizer_model",     m.tokenizer_model},
          {"bos_token_id",        std::to_string(m.bos_token_id)},
          {"eos_token_id",        std::to_string(m.eos_token_id)},
          {"chat_template",       m.chat_template},
          {"chat_templates",      m.chat_templates},
          {"has_vision",          m.has_vision ? "true" : "false"},
          {"has_audio",           m.has_audio ? "true" : "false"},
        });
      }
      return result;
    });

    webview->expose("load_preset", [](const std::string& model_name) {
      auto p = Preset::load(model_name);
      auto result = std::map<std::string, std::string>{
        // ─── Основные ────────────────────────────────────
        {"ctx_size",      std::to_string(p.ctx_size)},
        {"threads",       std::to_string(p.threads)},
        {"threads_batch", std::to_string(p.threads_batch)},
        {"gpu_layers",    std::to_string(p.gpu_layers)},
        {"batch_size",    std::to_string(p.batch_size)},
        {"ubatch_size",   std::to_string(p.ubatch_size)},
        {"mmproj",        p.mmproj},
        {"cache_type_k",  p.cache_type_k},
        {"cache_type_v",  p.cache_type_v},
        {"parallel",      std::to_string(p.parallel)},
        {"timeout",       std::to_string(p.timeout)},
        {"seed",          std::to_string(p.seed)},
        {"flash_attn",    p.flash_attn ? "true" : "false"},
        {"mlock",         p.mlock ? "true" : "false"},
        {"no_mmap",       p.no_mmap ? "true" : "false"},
        // ─── Генерация ────────────────────────────────────
        {"temp",              std::to_string(p.temp)},
        {"predict",           std::to_string(p.predict)},
        {"min_p",             std::to_string(p.min_p)},
        {"top_k",             std::to_string(p.top_k)},
        {"top_p",             std::to_string(p.top_p)},
        {"repeat_penalty",    std::to_string(p.repeat_penalty)},
        {"presence_penalty",  std::to_string(p.presence_penalty)},
        {"frequency_penalty", std::to_string(p.frequency_penalty)},
        {"reasoning_mode",    p.reasoning_mode ? "true" : "false"},
        {"reasoning_budget",  std::to_string(p.reasoning_budget)},
        // ─── Спекулятивное декодирование ────────────────────
        {"spec_type",        p.spec_type},
        {"draft_model",      p.draft_model},
        {"hf_repo_draft",    p.hf_repo_draft},
        {"draft_gpu_layers", std::to_string(p.draft_gpu_layers)},
        {"spec_draft_n_max", std::to_string(p.spec_draft_n_max)},
        {"spec_draft_n_min", std::to_string(p.spec_draft_n_min)},
        {"spec_draft_p_split", std::to_string(p.spec_draft_p_split)},
        {"spec_draft_p_min", std::to_string(p.spec_draft_p_min)},
        // ─── Дополнительные ─────────────────────────────────
        {"cont_batching", p.cont_batching ? "true" : "false"},
        {"webui",         p.webui ? "true" : "false"},
        {"embedding",     p.embedding ? "true" : "false"},
        {"slots",         p.slots ? "true" : "false"},
        {"metrics",       p.metrics ? "true" : "false"},
        {"cache_prompt",  p.cache_prompt ? "true" : "false"},
        {"context_shift", p.context_shift ? "true" : "false"},
        {"alias",         p.alias},
        // ─── Custom args (JSON) ─────────────────────────────
        {"custom_args",   glz::write_json(p.custom_args).value_or("[]")},
        // ─── GGUF metadata ─────────────────────────────────
        {"display_name",        p.display_name},
        {"size_label",          p.size_label},
        {"license",             p.license},
        {"author",              p.author},
        {"version",             p.version},
        {"url",                 p.url},
        {"source_url",          p.source_url},
        {"languages",           p.languages},
        {"vocab_size",          std::to_string(p.vocab_size)},
        {"embedding_length",    std::to_string(p.embedding_length)},
        {"feed_forward_length", std::to_string(p.feed_forward_length)},
        {"head_count",          std::to_string(p.head_count)},
        {"head_count_kv",       std::to_string(p.head_count_kv)},
        {"expert_count",        std::to_string(p.expert_count)},
        {"expert_used_count",   std::to_string(p.expert_used_count)},
        {"tokenizer_model",     p.tokenizer_model},
        {"bos_token_id",        std::to_string(p.bos_token_id)},
        {"eos_token_id",        std::to_string(p.eos_token_id)},
        {"chat_template",       p.chat_template},
        {"chat_templates",      p.chat_templates},
        {"has_vision",          p.has_vision ? "true" : "false"},
        {"has_audio",           p.has_audio ? "true" : "false"},
      };
      return result;
    });

    webview->expose("save_preset", [](const std::string& model_name,
                                       const std::map<std::string, std::string>& kv) {
      auto p = Preset::load(model_name);
      auto gi = [&](const std::string& k, int& v) {
        if (!kv.contains(k)) return;
        try { v = std::stoi(kv.at(k)); } catch (...) {}
      };
      auto gu = [&](const std::string& k, uint32_t& v) {
        if (!kv.contains(k)) return;
        try { v = static_cast<uint32_t>(std::stoul(kv.at(k))); } catch (...) {}
      };
      auto gf = [&](const std::string& k, float& v) {
        if (!kv.contains(k)) return;
        try { v = std::stof(kv.at(k)); } catch (...) {}
      };
      auto gs = [&](const std::string& k, std::string& v) {
        if (kv.contains(k)) v = kv.at(k);
      };
      auto gb = [&](const std::string& k, bool& v) {
        if (kv.contains(k)) v = (kv.at(k) == "true");
      };

      // ─── Основные ──────────────────────────────────────
      gi("ctx_size",      p.ctx_size);
      gi("threads",       p.threads);
      gi("threads_batch", p.threads_batch);
      gi("gpu_layers",    p.gpu_layers);
      gi("batch_size",    p.batch_size);
      gi("ubatch_size",   p.ubatch_size);
      gs("mmproj",        p.mmproj);
      gs("cache_type_k",  p.cache_type_k);
      gs("cache_type_v",  p.cache_type_v);
      gi("parallel",      p.parallel);
      gi("timeout",       p.timeout);
      gi("seed",          p.seed);
      gb("flash_attn",    p.flash_attn);
      gb("mlock",         p.mlock);
      gb("no_mmap",       p.no_mmap);
      // ─── Генерация ────────────────────────────────────
      gf("temp",              p.temp);
      gi("predict",           p.predict);
      gf("min_p",             p.min_p);
      gi("top_k",             p.top_k);
      gf("top_p",             p.top_p);
      gf("repeat_penalty",    p.repeat_penalty);
      gf("presence_penalty",  p.presence_penalty);
      gf("frequency_penalty", p.frequency_penalty);
      gb("reasoning_mode",    p.reasoning_mode);
      gi("reasoning_budget",  p.reasoning_budget);
      // ─── Спекулятивное декодирование ──────────────────
      gs("spec_type",        p.spec_type);
      gs("draft_model",      p.draft_model);
      gs("hf_repo_draft",    p.hf_repo_draft);
      gi("draft_gpu_layers", p.draft_gpu_layers);
      gi("spec_draft_n_max", p.spec_draft_n_max);
      gi("spec_draft_n_min", p.spec_draft_n_min);
      gf("spec_draft_p_split", p.spec_draft_p_split);
      gf("spec_draft_p_min", p.spec_draft_p_min);
      // ─── Дополнительные ───────────────────────────────
      gb("cont_batching",  p.cont_batching);
      gb("webui",          p.webui);
      gb("embedding",      p.embedding);
      gb("slots",          p.slots);
      gb("metrics",        p.metrics);
      gb("cache_prompt",   p.cache_prompt);
      gb("context_shift",  p.context_shift);
      gs("alias",          p.alias);
      // ─── Custom args (JSON) ───────────────────────────
      if (kv.contains("custom_args")) {
        auto ec = glz::read_json(p.custom_args, kv.at("custom_args"));
        if (ec) {
          std::fprintf(stderr, "llamancher: invalid custom_args JSON — %s\n",
                       glz::format_error(ec).c_str());
        }
      }
      // ─── GGUF metadata ───────────────────────────────
      gs("display_name",        p.display_name);
      gs("size_label",          p.size_label);
      gs("license",             p.license);
      gs("author",              p.author);
      gs("version",             p.version);
      gs("url",                 p.url);
      gs("source_url",          p.source_url);
      gs("languages",           p.languages);
      gu("vocab_size",          p.vocab_size);
      gu("embedding_length",    p.embedding_length);
      gu("feed_forward_length", p.feed_forward_length);
      gu("head_count",          p.head_count);
      gu("head_count_kv",       p.head_count_kv);
      gu("expert_count",        p.expert_count);
      gu("expert_used_count",   p.expert_used_count);
      gs("tokenizer_model",     p.tokenizer_model);
      { if (kv.contains("bos_token_id")) try { p.bos_token_id = static_cast<int32_t>(std::stoi(kv.at("bos_token_id"))); } catch (...) {} }
      { if (kv.contains("eos_token_id")) try { p.eos_token_id = static_cast<int32_t>(std::stoi(kv.at("eos_token_id"))); } catch (...) {} }
      gs("chat_template",       p.chat_template);
      gs("chat_templates",      p.chat_templates);
      gb("has_vision",          p.has_vision);
      gb("has_audio",           p.has_audio);

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
      if (settings->llama_server_path.empty()) {
        return std::string("server_not_found");
      }
      auto preset = Preset::load(model_name);
      auto args = preset.cli_args(model_path);
      args.insert(args.end(), {"--host", "127.0.0.1"});
      args.push_back("--port");
      args.push_back(std::to_string(settings->port));
      if (!settings->api_key.empty()) {
        args.push_back("--api-key");
        args.push_back(settings->api_key);
      }
      if (settings->offline) {
        args.push_back("--offline");
      }
      server_mgr->start(settings->llama_server_path, args);
      // Use display_name from preset for the title, fallback to filename stem
      if (!preset.display_name.empty()) {
        server_mgr->set_current_model(preset.display_name);
      }
      // Remember last started model for auto-start
      settings->last_model = model_name;
      settings->save();
      return std::string("ok");
    });

    webview->expose("stop_server", [server_mgr]() {
      server_mgr->stop();
    });

    // ── Server logs ────────────────────────────────────────────────

    webview->expose("get_server_log_path", [server_mgr]() {
      return server_mgr->log_path();
    });

    webview->expose("read_log", [](const std::string& path, int max_lines) {
      std::ifstream f(path);
      if (!f) return std::vector<std::string>{};
      std::deque<std::string> queue;
      std::string line;
      while (std::getline(f, line)) {
        queue.push_back(line);
        if (static_cast<int>(queue.size()) > max_lines)
          queue.pop_front();
      }
      return std::vector<std::string>(queue.begin(), queue.end());
    });

    // ── Grammar file picker ────────────────────────────────────────

    webview->expose("pick_grammar_file", [show_file_dialog](saucer::executor<std::string> exec) {
      auto* filter = gtk_file_filter_new();
      gtk_file_filter_set_name(filter, "Grammar files");
      gtk_file_filter_add_pattern(filter, "*.gbnf");
      gtk_file_filter_add_pattern(filter, "*.txt");
      show_file_dialog(nullptr, "Select grammar file",
                        GTK_FILE_CHOOSER_ACTION_OPEN, std::move(exec), filter);
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
