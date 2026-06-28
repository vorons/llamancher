#pragma once

#include <string>
#include <filesystem>

struct Settings {
  std::string llama_server_path;
  std::string models_dir;
  bool auto_start_server = false;

  Settings();

  static Settings load();
  void save() const;
  std::filesystem::path path() const;
};
