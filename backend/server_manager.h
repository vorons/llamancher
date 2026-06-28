#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>

enum class ServerStatus { Stopped, Starting, Running, Error };

// Abstract observer — avoids GCC 16 std::function/consteval bugs
class ServerObserver {
public:
  virtual ~ServerObserver() = default;
  virtual void on_status_change(ServerStatus) = 0;
};

class ServerManager {
public:
  ServerManager();
  ~ServerManager();

  void start(const std::string& server_path, const std::vector<std::string>& args);
  void stop();
  void health_check();

  ServerStatus status() const;
  std::string   current_model() const;
  void          set_current_model(const std::string& name) { current_model_ = name; }
  int           port() const;

  void set_observer(ServerObserver* obs) { observer_ = obs; }
  std::string log_path() const { return log_path_; }

private:
  void health_loop();
  void notify(ServerStatus s);

  std::atomic<ServerStatus> status_{ServerStatus::Stopped};
  std::string current_model_;
  std::string server_path_;
  std::vector<std::string> args_;
  int port_ = 8080;
  std::string log_path_;
  pid_t pid_ = 0;
  std::unique_ptr<std::thread> health_thread_;
  std::atomic<bool> running_{false};
  ServerObserver* observer_ = nullptr;
};
