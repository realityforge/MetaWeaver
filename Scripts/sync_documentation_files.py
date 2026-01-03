import shutil
import os
import argparse
import sys
import filecmp

def main():
    parser = argparse.ArgumentParser(description="Sync README and LICENSE to docs folder.")
    parser.add_argument("--dry-run", action="store_true", help="Check if files are in sync without copying.")
    args = parser.parse_args()

    # Define source and destination paths relative to project root
    # Assuming script is in Scripts/ and run from project root
    tasks = [
        ("README.md", "docs/index.md"),
        ("LICENSE", "docs/LICENSE"),
    ]

    success = True
    for src, dst in tasks:
        if not os.path.exists(src):
            print(f"Error: Source file {src} not found.")
            success = False
            continue

        if args.dry_run:
            if not os.path.exists(dst) or not filecmp.cmp(src, dst, shallow=False):
                print(f"Out of sync: {src} -> {dst}")
                success = False
            else:
                print(f"In sync: {src} -> {dst}")
        else:
            print(f"Copying {src} to {dst}...")
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)

    if not success:
        if args.dry_run:
            print("\nError: Documentation files are out of sync. Run 'python Scripts/sync_documentation_files.py' to fix.")
        sys.exit(1)

    if args.dry_run:
        print("\nAll documentation files are in sync.")


if __name__ == "__main__":
    main()
