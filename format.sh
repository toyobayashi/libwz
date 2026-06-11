#!/usr/bin/env bash
set -euo pipefail

PJ_ROOT="$(cd "$(dirname "$0")" && pwd)"

CLANG_FORMAT="$(command -v clang-format 2>/dev/null || true)"
if [[ -z "$CLANG_FORMAT" ]]; then
  echo "ERROR: clang-format not found on PATH." >&2
  exit 1
fi

echo "Formatting C++ sources with $CLANG_FORMAT..."

"$CLANG_FORMAT" -i --style=file \
  "$PJ_ROOT/src"/*.cpp \
  "$PJ_ROOT/src"/**/*.cpp \
  "$PJ_ROOT/include"/**/*.h \
  "$PJ_ROOT/tests"/*.cpp

echo "Done."
