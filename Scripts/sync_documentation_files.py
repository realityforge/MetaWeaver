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
            # For README.md, compare against the transformed content that will be written.
            if os.path.basename(src) == "README.md":
                with open(src, "r", encoding="utf-8") as f:
                    src_content = f.read()
                transformed = src_content.replace("(docs/", "(")

                if not os.path.exists(dst):
                    print(f"Out of sync: {src} -> {dst}")
                    success = False
                else:
                    with open(dst, "r", encoding="utf-8") as f:
                        dst_content = f.read()
                    if dst_content != transformed:
                        print(f"Out of sync: {src} -> {dst}")
                        success = False
                    else:
                        print(f"In sync: {src} -> {dst}")
            else:
                if not os.path.exists(dst) or not filecmp.cmp(src, dst, shallow=False):
                    print(f"Out of sync: {src} -> {dst}")
                    success = False
                else:
                    print(f"In sync: {src} -> {dst}")
        else:
            print(f"Copying {src} to {dst}...")
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            # For README.md, write transformed content; else do a direct copy.
            if os.path.basename(src) == "README.md":
                with open(src, "r", encoding="utf-8") as f:
                    content = f.read()
                content = content.replace("(docs/", "(")
                with open(dst, "w", encoding="utf-8", newline="") as f:
                    f.write(content)
            else:
                shutil.copy2(src, dst)

    if not success:
        if args.dry_run:
            print("\nError: Documentation files are out of sync. Run 'python Scripts/sync_documentation_files.py' to fix.")
        sys.exit(1)

    if args.dry_run:
        print("\nAll documentation files are in sync.")


if __name__ == "__main__":
    main()
