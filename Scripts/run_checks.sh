#!/usr/bin/env bash
set -euo pipefail

# Run MetaWeaver repository lint checks locally.
# Usage: bash Scripts/run_checks.sh

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="${here%/Scripts}"

cd "$root"

echo "[lint] Checking inline-generated includes..."
python3 Scripts/check_inline_generated_cpp_includes.py

echo "[lint] Checking license banners..."
python3 Scripts/check_license_banner.py

echo "[lint] OK"

