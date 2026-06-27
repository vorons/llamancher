#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"

echo "==> Building frontend …"
cd "$ROOT/frontend"
pnpm install --frozen-lockfile 2>/dev/null || pnpm install
pnpm run build

echo ""
echo "==> Building backend (Release) …"
cmake -S "$ROOT" -B "$BUILD_DIR" --preset release 2>/dev/null || cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel

# Strip the binary explicitly (belt-and-suspenders)
if command -v strip &>/dev/null; then
  strip --strip-all "$BUILD_DIR/llamancher" 2>/dev/null || true
fi

echo ""
echo "==> Binary: $BUILD_DIR/llamancher"
echo "==> Run with --dev to load from Vite dev server"
