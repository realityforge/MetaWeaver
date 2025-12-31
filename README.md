# MetaWeaver Unreal Plugin

MetaWeaver is a powerful Unreal Engine plugin for authoring, validating, and bulk-editing structured metadata on assets. Define typed metadata parameters once, enforce them across asset classes, and edit them through a clean, intuitive editor and matrix view.

---

**MetaWeaver** is a full-featured metadata authoring system for Unreal Engine that brings clarity, structure, and scalability to asset metadata workflows. It introduces a dedicated, artist-friendly **Metadata Editor Window** and a powerful **bulk-editing Property Matrix**, allowing teams to manage metadata for one or thousands of assets with ease.

With **MetadataParameterDefinition** assets, you can declare strongly-typed metadata keys — integers, floats, strings, enums, asset references, and more. Organize these into reusable **MetadataDefinitionSets** and associate them with specific asset classes. MetaWeaver ensures that metadata is both structured and validated, preventing errors and ensuring consistency across your project.

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
  Works like any other Unreal editor tool tab. Supports add/remove of keys, reordering, validation errors, quick filtering, and typed widgets (e.g., dropdowns, sliders, asset pickers).

* **Context Menu Integration**
  Select assets → Right-click → “Edit Metadata” opens the editor with the selected items loaded.

* **Bulk Editing Matrix View**
  Spreadsheet-like editor for modifying metadata across many assets at once.

### **Metadata Modeling Features**

* **MetadataParameterDefinition Assets**
  Define:

  * Name
  * DataType
  * Validation rules (ranges, enums, allowed types)
  * Display options (category, editor widget, sorting)

* **MetadataDefinitionSet Assets**
  Collections of parameter definitions, with support for:

  * Aggregation from multiple sets
  * Class-based associations (e.g., all materials, all skeletal meshes)

* **Automatic UI Generation**
  Editor widgets adapt based on type: text boxes, number fields, enum dropdowns, asset pickers, color pickers, toggle switches, etc.

### **Pipeline / Automation Friendly**

* Designed to work with:

  * Unreal Python
  * Editor Utility Widgets
  * Build systems
  * External asset management tools

## Contributor Resources

- Review [AGENTS.md](AGENTS.md) for repository guidelines covering layout, build commands, coding style, and review expectations.

### Local Lint Checks

Run repository linters locally before pushing:

```
bash Scripts/run_checks.sh
```

The script runs:
- `Scripts/check_inline_generated_cpp_includes.py` — verifies UE inline-generated include macro placement.
- `Scripts/check_license_banner.py` — enforces the exact license banner at the top of source files.
- `Scripts/format.py` — Runs clang-format across the source files.
