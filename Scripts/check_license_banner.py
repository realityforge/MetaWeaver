#!/usr/bin/env python3
"""
Check that all source files start with the exact Apache 2.0 license banner.

By default, scans under <repo>/Source/ for files with extensions:
  - .h, .hpp, .hh, .hxx
  - .c, .cc, .cpp, .cxx
  - .cs (including *.Build.cs)

Requires the full, exact multi-line Apache 2.0 banner used in this repo as a strict prefix (no leading blank lines).

Usage:
  python3 Scripts/check_license_banner.py [--root <path>]

Exits with code 1 if any violations are found; prints offending files.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
import re


DEFAULT_EXTS = {
    ".h", ".hpp", ".hh", ".hxx",
    ".c", ".cc", ".cpp", ".cxx",
    ".cs",
}

EXPECTED_PREFIX = (
    "/*\n"
    " * Licensed under the Apache License, Version 2.0 (the \"License\");\n"
    " * you may not use this file except in compliance with the License.\n"
    " * You may obtain a copy of the License at\n"
    " *\n"
    " *     http://www.apache.org/licenses/LICENSE-2.0\n"
    " *\n"
    " * Unless required by applicable law or agreed to in writing, software\n"
    " * distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    " * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
    " * See the License for the specific language governing permissions and\n"
    " * limitations under the License.\n"
    " */\n"
)



def read_start(path: Path, max_bytes: int) -> str:
    try:
        with path.open("r", encoding="utf-8", errors="ignore") as f:
            return f.read(max_bytes)
    except Exception:
        return ""


def looks_like_license_banner(text: str) -> bool:
    # Exact prefix match; no leading whitespace or BOM allowed
    return text.startswith(EXPECTED_PREFIX)


def is_candidate(path: Path, exts: set[str]) -> bool:
    if any(part in {"Binaries", "Intermediate", "DerivedDataCache", "vendor"} for part in path.parts):
        return False
    if "Source" not in path.parts:
        return False
    if path.suffix.lower() in exts:
        return True
    # Special-case Build.cs (endswith .Build.cs)
    if path.name.endswith(".Build.cs"):
        return True
    return False


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent,
                    help="Repo root (defaults to parent of Scripts/)")
    ap.add_argument("--fix", action="store_true", help="Prepend the exact banner to offending files")
    args = ap.parse_args()

    root: Path = args.root
    source_root = root / "Source"
    if not source_root.exists():
        print(f"[WARN] No Source/ directory under {root}")
        return 0

    exts = DEFAULT_EXTS
    violations: list[Path] = []

    for path in source_root.rglob("*"):
        if not path.is_file():
            continue
        if not is_candidate(path, exts):
            continue
        head = read_start(path, len(EXPECTED_PREFIX))
        if not looks_like_license_banner(head):
            violations.append(path)

    if violations:
        if args.fix:
            fixed = 0
            for p in violations:
                try:
                    # Read full file (strip BOM if present)
                    content = p.read_text(encoding="utf-8-sig", errors="ignore")
                    if content.startswith(EXPECTED_PREFIX):
                        continue
                    p.write_text(EXPECTED_PREFIX + content, encoding="utf-8")
                    fixed += 1
                except Exception as e:
                    print(f"Failed to fix {p}: {e}")
            print(f"Fixed license banner on {fixed} file(s).")
            return 0
        else:
            print("License banner check failed on these files:")
            for v in violations:
                print(f" - {v}")
            return 1

    print("OK: All source files start with the license banner.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
