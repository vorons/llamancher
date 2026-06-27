#include "server_manager.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <format>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

ServerManager::ServerManager() = default;
ServerManager::~ServerManager() {
  // Prevent the health thread from calling into the observer while we tear down
  observer_ = nullptr;
  running_.store(false);

  if (health_thread_ && health_thread_->joinable()) health_thread_->join();
  health_thread_.reset();

  if (pid_ > 0) {
    kill(pid_, SIGKILL);
    waitpid(pid_, nullptr, 0);
    pid_ = 0;
  }
}

ServerStatus ServerManager::status() const { return status_.load(); }
std::string ServerManager::current_model() const { return current_model_; }
int ServerManager::port() const { return port_; }

void ServerManager::notify(ServerStatus s) {
  if (observer_) observer_->on_status_change(s);
}

void ServerManager::start(const std::string& server_path,
                          const std::vector<std::string>& args) {
  if (status() != ServerStatus::Stopped) return;

  status_.store(ServerStatus::Starting);
  server_path_ = server_path;
  args_ = args;
  current_model_ = "unknown";

  for (size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == "--model") {
      fs::path p(args[i + 1]);
      current_model_ = p.stem().string();
      break;
    }
  }

  pid_ = fork();
  if (pid_ == 0) {
    std::vector<const char*> argv;
    argv.push_back(server_path.c_str());
    for (auto& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);

    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      dup2(devnull, STDOUT_FILENO);
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }

    execvp(server_path.c_str(), const_cast<char* const*>(argv.data()));
    _exit(127);
  }

  if (pid_ < 0) {
    status_.store(ServerStatus::Error);
    notify(ServerStatus::Error);
    return;
  }

  running_.store(true);
  health_thread_ = std::make_unique<std::thread>([this] { health_loop(); });

  notify(ServerStatus::Starting);
}

void ServerManager::stop() {
  running_.store(false);
  if (health_thread_ && health_thread_->joinable()) health_thread_->join();
  health_thread_.reset();

  if (pid_ > 0) {
    kill(pid_, SIGTERM);
    for (int i = 0; i < 50; ++i) {
      int wstatus;
      auto ret = waitpid(pid_, &wstatus, WNOHANG);
      if (ret == pid_) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    kill(pid_, SIGKILL);
    waitpid(pid_, nullptr, 0);
    pid_ = 0;
  }

  status_.store(ServerStatus::Stopped);
  current_model_.clear();
  notify(ServerStatus::Stopped);
}

void ServerManager::health_loop() {
  while (running_.load()) {
    health_check();
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

void ServerManager::health_check() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return;

  struct hostent* server = gethostbyname("127.0.0.1");
  if (!server) { close(sock); return; }

  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  std::memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
  addr.sin_port = htons(port_);

  struct timeval tv{};
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(sock);
    if (status_.load() == ServerStatus::Running) {
      status_.store(ServerStatus::Starting);
      notify(ServerStatus::Starting);
    }
    return;
  }

  const char* req = "GET /health HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
  send(sock, req, strlen(req), 0);

  char buf[256];
  auto n = read(sock, buf, sizeof(buf) - 1);
  close(sock);

  if (n > 0) {
    buf[n] = 0;
    if (std::strstr(buf, "200 OK") || std::strstr(buf, "200 ok")) {
      auto prev = status_.exchange(ServerStatus::Running);
      if (prev != ServerStatus::Running) {
        notify(ServerStatus::Running);
      }
    }
  }
}
