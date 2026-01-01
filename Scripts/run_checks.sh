#!/usr/bin/env bash
set -euo pipefail

# Run MetaWeaver repository lint checks locally.
# Usage: bash Scripts/run_checks.sh

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${here%/Scripts}"

cd "$root"

if command -v python3 >/dev/null 2>&1; then
  PY=python3
elif command -v python >/dev/null 2>&1; then
  PY=python
else
  PY=""
fi

if [ -z "$PY" ]; then
  echo "WARN: python not found; skipping python checks" >&2
else
  echo "[lint] Checking inline-generated includes..."
  $PY Scripts/check_inline_generated_cpp_includes.py

  echo "[lint] Checking license banners..."
  $PY Scripts/check_license_banner.py

  echo "[lint] Checking source code formatting..."
  $PY Scripts/format.py

  echo "[lint] Checking documentation files synchronized..."
  $PY Scripts/sync_documentation_files.py
fi

echo "[lint] OK"
