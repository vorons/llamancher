#pragma once

#include <string>
#include <filesystem>

struct Settings {
  std::string llama_server_path;
  std::string models_dir;
  bool auto_start_server = false;
  std::string theme = "dark"; // "dark" | "light"

  Settings();

  static Settings load();
  void save() const;
  std::filesystem::path path() const;
};

// Glaze reflection — keeps field names in JSON consistent with struct members
#include <glaze/glaze.hpp>

template<>
struct glz::meta<Settings> {
  using T = Settings;
  static constexpr auto value = object(
    "llama_server_path", &T::llama_server_path,
    "models_dir",        &T::models_dir,
    "auto_start_server", &T::auto_start_server,
    "theme",              &T::theme
  );
};
