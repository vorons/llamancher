# llamancher

> Desktop GUI for managing and launching GGUF models via `llama-server`.

Scans a directory for `.gguf` files, lets you configure per-model presets (context size, GPU layers, sampling params), and launches `llama-server` as a child process — all from a borderless window with a dark theme.

![screenshot placeholder](https://img.shields.io/badge/status-alpha-orange)
![binary size](https://img.shields.io/badge/size-888%C2%A0KB-brightgreen)

## Features

- **Model browser** — scans a directory for GGUF files, detects quantization from filename
- **Rich metadata extraction** — reads 27+ GGUF metadata fields (model name, description, author, license, architecture params, MoE config, sampling defaults, tokenizer config, chat templates, vision/audio support) and caches them in the preset
- **Per-model presets** — full editor for all llama-server parameters: context size, GPU layers, sampling (temp, top-k/p, min-p, repeat/presence/frequency penalty, mirostat), KV cache (batch/ubatch size, cache types, flash-attn), speculative decoding, auto-fit, and more. GGUF metadata (architecture, sampling defaults) is auto-populated when creating a preset
- **Server lifecycle** — start/stop `llama-server`, health polling, status displayed in UI
- **Persistent settings** — `~/.config/llamancher/settings.json`
- **Persistent presets** — `~/.llamancher/models/<name>.preset.json`
- **Borderless window** — custom title bar via Svelte, `window::decoration::partial`
- **Embedded frontend** — production builds bundle the Svelte app into the binary (no extra files)
- **Array-safe GGUF parser** — recursive `skip_value()` doesn't crash on arrays or unknown data types

## Requirements

| Dependency | Minimum | Notes |
|-----------|---------|-------|
| GCC | 13+ | GCC 16 (Fedora Rawhide) supported; `std::function` workaround via virtual observer |
| CMake | 3.25+ | |
| WebKitGTK | 2.44+ | Bundled with Saucer 8 |
| GTK | 4.16+ | Saucer 8 requirement |
| pnpm | 9+ | For frontend build |
| Node.js | 22+ | For frontend build |
| llama-server | any | From [llama.cpp](https://github.com/ggml-org/llama.cpp) releases |

System dependencies on Fedora:

```bash
sudo dnf install webkitgtk6.0-devel gtk4-devel cmake gcc-c++
```

### Optional

- `curl` — dev mode health check in `dev.sh`

## Quick start

```bash
# 1. Clone and enter
git clone https://github.com/your/llamancher
cd llamancher

# 2. Build (frontend + backend, release)
./scripts/build.sh

# 3. Run
./build/llamancher

# Dev mode (frontend HMR on :5173)
./scripts/dev.sh
```

Or manually:

```bash
# Build frontend
cd frontend && pnpm install && pnpm run build && cd ..

# Configure & build backend
cmake -B build --preset release
cmake --build build --parallel

# Run
./build/llamancher
```

## Usage

1. Set the **models directory** and **llama-server path** in Settings (gear icon in title bar)
2. Click **Scan** to discover GGUF models
3. Click a model to edit its preset (sampling params, GPU layers, etc.)
4. Press **Launch** to start `llama-server` with that model
5. `llama-server` is now available at `http://127.0.0.1:8080` — connect with any OpenAI-compatible client (e.g. [Continue](https://continue.dev), [Open WebUI](https://openwebui.com))

## Project structure

```
llamancher/
├── backend/               # C++22, Saucer 8
│   ├── main.cpp           # Window, IPC API, status observer
│   ├── model_manager.h/cpp   # .gguf scanner
│   ├── server_manager.h/cpp  # Child process + health polling
│   ├── preset.h/cpp          # Per-model presets (JSON)
│   └── settings.h/cpp        # App settings (JSON)
├── frontend/              # Svelte 5 + TypeScript + Tailwind 4
│   └── src/
│       ├── App.svelte
│       ├── lib/
│       │   ├── components/   # HeaderBar, ModelList, ModelDetail, SettingsDialog
│       │   ├── ui/           # button, dialog, input, slider, switch, etc.
│       │   ├── saucer.ts     # IPC wrapper
│       │   ├── stores.svelte.ts
│       │   └── types.ts
│       └── main.ts
├── scripts/
│   ├── build.sh            # Production build
│   └── dev.sh              # Development mode (HMR)
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

## Build presets

| Preset | Optimisation | Binary size |
|--------|-------------|-------------|
| `release` | `-O2 -flto=auto -fvisibility=hidden` + gc-sections + strip | ~888 KB |
| `debug` | `-O0 -g0` | ~33 MB |

```bash
cmake --build build --preset release   # release
cmake --build build --preset debug     # debug (fast iteration)
```

## Configuration

**`~/.config/llamancher/settings.json`**

```json
{
  "llama_server_path": "llama-server",
  "models_dir": "/home/user/models",
  "auto_start_server": false
}
```

**`~/.llamancher/models/<name>.preset.json`** — on first scan, the preset is auto-populated with GGUF metadata. On subsequent scans, metadata is refreshed.

```jsonc
{
  // ─── Performance ────────────────────
  "ctx_size": 2048,
  "threads": 4,
  "threads_batch": 0,
  "gpu_layers": 0,
  "batch_size": 2048,
  "ubatch_size": 512,
  "flash_attn": false,
  "mlock": false,
  "no_mmap": false,
  "parallel": 1,
  "timeout": 0,

  // ─── Generation ─────────────────────
  "temp": 0.80,
  "predict": -1,
  "min_p": 0.05,
  "top_k": 40,
  "top_p": 0.95,
  "repeat_penalty": 1.00,
  "presence_penalty": 0.00,
  "frequency_penalty": 0.00,
  "reasoning_mode": false,
  "reasoning_budget": 0,
  "seed": -1,

  // ─── KV Cache ───────────────────────
  "cache_type_k": "",
  "cache_type_v": "",

  // ─── Multimodal ─────────────────────
  "mmproj": "",

  // ─── Extra ──────────────────────────
  "alias": "",
  "cont_batching": true,
  "webui": true,
  "embedding": false,
  "slots": true,
  "metrics": false,
  "cache_prompt": true,
  "context_shift": true,
  "offline": false,

  // ─── GGUF metadata (read-only) ─────
  "display_name": "Qwen2.5-7B-Instruct",
  "size_label": "7B",
  "license": "apache-2.0",
  "author": "Qwen",
  "vocab_size": 152064,
  "embedding_length": 3584,
  "feed_forward_length": 18944,
  "head_count": 28,
  "head_count_kv": 4,
  "expert_count": 0,
  "expert_used_count": 0,
  "context_length": 131072,
  "chat_template": "{% for message in messages %}...",
  "has_vision": false,
  "has_audio": false
}
```

> Metadata fields (`display_name`, `vocab_size`, `sample_*`, etc.) are automatically read from the GGUF file. If the file lacks the field, an empty string or `0` is stored in JSON. Old presets without these fields continue to work (fallback to zero defaults). `general.sampling.*` overrides hardcoded sampling defaults when the value exceeds 0.001.

Editor sections on the model detail screen:

| Section | Parameters |
|---------|------------|
| **Generation** | Temperature (`--temp`), Context Size (`-c`), Max Tokens (`-n`), Min-P, Top-K, Top-P, Repeat/Presence/Frequency Penalty, Reasoning Mode/Budget, Seed |
| **Performance** | Threads (`-t`), Batch Threads (`-tb`), GPU Layers (`-ngl`), Batch Size (`-b`), UBatch Size (`-ub`), Parallel Slots (`-np`), Timeout (`-to`), Flash Attention (`-fa`), mlock, no-mmap |
| **KV Cache** | Cache Type K (`-ctk`), Cache Type V (`-ctv`) |
| **Multimodal** | MMProj (`-mm`) — project file for vision models |
| **Extra** | Alias (`-a`), Continuous Batching (`-cb`), WebUI, Embedding, Slots, Metrics, Cache Prompt, Context Shift, Offline Mode |
| **Speculative Decoding** | Spec Type, Draft Model, HF Repo, Draft GPU Layers, Draft N Max/Min, P Split/Range |
| **Custom CLI Arguments** | Free-form `--flag=value` list with per-item toggle |

## Technical notes

- **GCC 16 compatibility**: Saucer 8 callbacks use `std::function`, which triggers `consteval` errors in GCC 16. The codebase avoids this by using a virtual `ServerObserver` interface instead of function-based callbacks for server status updates.
- **Binary size**: Aggressive LTO + sections garbage collection + stripping brings the binary to ~888 KB (was 33 MB in debug).
- **Frontend embedding**: `saucer_embed()` in CMake bundles `dist/` into the binary; no external files at runtime.

## License

MIT
