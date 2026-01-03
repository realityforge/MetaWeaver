# MetaWeaver Unreal Plugin

Typed, validated, bulk‑editable metadata for the Unreal Editor.

MetaWeaver is a powerful Unreal Engine plugin for authoring, validating, and bulk‑editing structured metadata on assets. Define typed metadata parameters once, enforce them across asset classes, and edit them through a clean, intuitive editor and matrix view.

---

**MetaWeaver** is a full-featured metadata authoring system for Unreal Engine that brings clarity, structure, and scalability to asset metadata workflows. It introduces a dedicated, artist-friendly **Metadata Editor Window** and a powerful **bulk-editing Property Matrix**, allowing teams to manage metadata for one or thousands of assets with ease.

Use **MetadataDefinitionSet** assets to declare per‑class parameter specifications. Each spec defines a key, type (Integer, Float, String, Bool, Enum via string values, or Asset Reference), default value, required flag, and any constraints (e.g., AllowedClass for asset references). MetaWeaver enforces formatting and validation at edit time.

Perfect for game teams, virtual production, enterprise pipelines, and any project that relies on rich, reliable asset metadata.

### **Key Features**

* **Dedicated Metadata Editor Window**
  Edit key/value pairs in an intuitive UI designed for clarity and speed.

* **Bulk Editing with Metadata Matrix**
  Modify metadata on multiple assets simultaneously using an enhanced property matrix view.

* **Typed Metadata Parameters**
  Enforce integers, floats, enums, booleans, strings, and asset references via a structured definition system.

* **Metadata Definitions & Sets**
  Create modular metadata definitions and aggregate them into reusable sets based on asset types.

* **Context Menu Integration**
  Right-click any asset in the Content Browser to instantly open its metadata editor.

* **Arbitrary Custom Metadata**
  Add ad-hoc key/value pairs or build comprehensive, structured metadata schemas.

* **Pipeline-Ready**
  Ideal for artistic workflows, technical pipelines, automation tasks, and asset catalogs.

MetaWeaver brings powerful metadata tooling into the Unreal Editor — making your metadata as organized and reliable as the assets it describes.

# Extended Feature Description

### **Editor Features**

* **Standalone Metadata Editor Panel**
  Works like any other Unreal editor tool tab. Supports add/remove of keys, inline validation, quick filtering, and typed widgets (checkbox, numeric fields, text, enum dropdown, asset picker).

* **Context Menu Integration**
  Select assets → Right-click → “Edit Metadata” opens the editor with the selected items loaded.

* **Bulk Editing Matrix View**
  Spreadsheet-like editor for modifying metadata across many assets at once.

### **Metadata Modeling Features**

* **Definition Model (Struct‑only)**
  Parameter specifications are stored inside a `UMetaWeaverMetadataDefinitionSet` and associated to classes:

  * `FMetadataParameterSpec` fields: `Key`, `Type` (Integer, Float, String, Bool, Enum, AssetReference), `Description`, `DefaultValue`, `bRequired`, `AllowedClass` (AssetReference), `EnumValues` (string list; trimmed/unique).

* **MetadataDefinitionSet Assets**
  Collections of parameter definitions, with support for:

  * Aggregation from multiple sets
  * Class-based associations (e.g., all materials, all skeletal meshes)

* **Automatic UI Generation**
  Editor widgets adapt based on type: text, numeric entry, checkbox, enum dropdown, and asset picker.

### **Pipeline / Automation Friendly**

* Designed to work with:

  * Unreal Python
  * Editor Utility Widgets
  * Build systems
  * External asset management tools

## Requirements

- Unreal Engine 5.6+
- Editor‑only (no runtime modules)
- Windows or macOS (Editor)

## Install

- Copy the repository into `Plugins/MetaWeaver/` in your project (or add as a submodule/subtree), then restart the Editor. Enable the plugin if prompted.
- Project Settings → MetaWeaver → add your active `MetadataDefinitionSet` assets to “Active Definition Sets”.

## User Guides

- Read Getting Started for step‑by‑step usage with screenshots: [Getting Started](docs/GettingStarted.md)
- Read Validation for enum rules, required keys, and asset reference constraints: [Validation](docs/Validation.md)

## Definition Model

- `UMetaWeaverMetadataDefinitionSet` aggregates other sets and declares parameter specs for classes.
- `FMetadataParameterSpec` defines:
  - `Key` and `Type` (Integer, Float, String, Bool, Enum, AssetReference)
  - `DefaultValue`, `bRequired`, `Description`
  - `AllowedClass` (AssetReference only)
  - `EnumValues` (string list; trimmed, unique; enforced during validation)

## Validation API (for other editor modules)

```
#include "MetaWeaver/Validation/MetaWeaverValidationSubsystem.h"

if (GEditor)
{
    if (auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
    {
        const auto Report = Subsystem->ValidateAsset(Asset);
        // Inspect Report.Issues …
    }
}
```

## Limitations

- Editor‑only (no runtime querying)
- UE 5.6+ only
- Enums are string lists, not UEnum; values must match the configured list

## Contributor Resources

- Review [CONTRIBUTING.md](CONTRIBUTING.md) for repository guidelines covering layout, build commands, coding style, and review expectations.

### Local Lint Checks

Run repository linters locally before pushing:

```
bash Scripts/run_checks.sh
```

The script runs:
- `Scripts/check_inline_generated_cpp_includes.py` - verifies UE inline-generated include macro placement.
- `Scripts/check_license_banner.py` - enforces the exact license banner at the top of source files.
- `Scripts/format.py` - runs clang-format and normalizes file encodings/line endings.
- `Scripts/sync_documentation_files.py` - copies documentation files from base directory into the `docs/` folder.

Optional: install a pre-commit hook to run these checks automatically:

```
cp Scripts/git-pre-commit .git/hooks/pre-commit
chmod u+x .git/hooks/pre-commit
```

## License & Support

- License: Apache 2.0 (see `LICENSE`)
- Support and issues: open a ticket on GitHub Issues
