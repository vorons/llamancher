#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "==> Starting Vite dev server …"
cd "$ROOT/frontend"
pnpm dev &
VITE_PID=$!

# Wait for Vite to be ready
echo "==> Waiting for Vite dev server …"
for i in $(seq 1 30); do
  if curl -sf http://127.0.0.1:5173 >/dev/null 2>&1; then
    break
  fi
  sleep 1
done

echo "==> Building and running backend in dev mode …"
cmake -S "$ROOT" -B "$ROOT/build" --preset debug 2>/dev/null || cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$ROOT/build" --parallel
"$ROOT/build/llamancher" --dev

# Cleanup
kill $VITE_PID 2>/dev/null || true
