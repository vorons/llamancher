# llamancher

> Desktop app for managing and launching LLM models (GGUF) via `llama-server`.

Built with [Saucer 8](https://saucer.app) (C++ webview), [Svelte 5](https://svelte.dev), [shadcn-svelte](https://shadcn-svelte.com) components, and [Tailwind CSS 4](https://tailwindcss.com).

## Screens

**Model List** — browse GGUF files in your models directory.  
**Model Detail** — edit per-model launch presets, start/stop the server.  
**Settings** — configure `llama-server` path and models directory.

## Prerequisites

- **C++23 compiler** (GCC 14+ or Clang 18+)
- **CMake 3.25+**
- **Node.js 20+** + **pnpm**
- **Linux**: WebKitGTK 4.1, GTK4, libsoup3, pkg-config

```bash
# Fedora
sudo dnf install webkitgtk6.0-devel gtk4-devel libsoup3-devel

# Ubuntu / Debian
sudo apt install libwebkitgtk-6.0-dev libgtk-4-dev libsoup-3.0-dev
```

## Quick start

```bash
# Build everything
./scripts/build.sh

# Run
./build/llamancher
```

### Development

```bash
# Terminal 1: Vite dev server
cd frontend && pnpm dev

# Terminal 2: C++ binary that loads from Vite
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/llamancher --dev
```

Or use the combined script:

```bash
./scripts/dev.sh
```

## Project structure

```
llamancher/
├── backend/                    # C++ source (Saucer IPC host)
│   ├── main.cpp                # Entry point, window, exposed API
│   ├── model_manager.h/cpp     # GGUF file scanner
│   ├── server_manager.h/cpp    # llama-server child process
│   ├── settings.h/cpp          # Persistent settings (~/.config/llamancher/)
│   └── preset.h/cpp            # Per-model presets (~/.llamancher/models/)
├── frontend/                   # Svelte 5 + Vite + Tailwind CSS 4
│   ├── src/
│   │   ├── lib/
│   │   │   ├── components/     # App components (HeaderBar, ModelList, …)
│   │   │   ├── ui/             # shadcn-style primitives (button, dialog, …)
│   │   │   ├── types.ts        # Shared TypeScript types
│   │   │   ├── stores.svelte.ts# Svelte stores for state
│   │   │   ├── saucer.ts       # IPC wrapper for C++ → JS bridge
│   │   │   └── utils.ts        # cn(), debounce(), …
│   │   ├── App.svelte          # Root component
│   │   └── main.ts             # Entry point
│   └── vite.config.ts
├── CMakeLists.txt              # Root CMake (builds C++ + embeds frontend)
├── scripts/
│   ├── build.sh                # Full production build
│   └── dev.sh                  # Dev mode (Vite + C++ backend)
└── README.md
```

## Configuration

| Setting | Path |
|---------|------|
| App settings | `~/.config/llamancher/settings.json` |
| Model presets | `~/.llamancher/models/<name>.preset.json` |
| llama-server (default) | `~/.llamancher/server/<version>/llama-server` |

## Architecture

```
┌─────────────┐  saucer IPC   ┌──────────────┐
│  Svelte UI  │ ←──────────→  │  C++ Backend │
│  (webview)  │  exposed API  │  (saucer)    │
└─────────────┘               └──────┬───────┘
                                     │ fork/exec
                                     ▼
                             ┌──────────────┐
                             │ llama-server │
                             │  (child PID) │
                             └──────────────┘
                                     │ HTTP health
                                     ▼
                             ┌──────────────┐
                             │  LLM model   │
                             │  (GGUF file) │
                             └──────────────┘
```
