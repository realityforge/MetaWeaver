# Repository Guidelines

## Authoritative Repository

The canonical source for this plugin is maintained at https://github.com/realityforge/MetaWeaver.  
Please open pull requests and issues on that repository.

This plugin may also be integrated into other repositories via git subtree merge or distributed as a direct download  
(unzip into `Plugins/MetaWeaver/`). Those copies are mirrors for consumption; treat them as downstream integrations and
avoid filing issues or PRs there.

## User Interaction

When asked to perform a task, gather context progressively. Ask for one piece of information at a time until the
requirements are clear. Make reasonable assumptions based on established patterns in the MetaWeaver codebase, and ask
the user to confirm if multiple valid interpretations exist.

## Project Structure & Module Organization

- `MetaWeaver.uplugin` declares a **single, editor-only module** named `MetaWeaverEditor`.

- The entire plugin lives under `Source/MetaWeaverEditor/`, which contains both the public API surface and private
  implementation:
  - `Source/MetaWeaverEditor/Public/MetaWeaverEditor/` — Public headers for any components or utilities intended for use outside of the module.
  - `Source/MetaWeaverEditor/Private/MetaWeaverEditor/` — Implementation files, UI widgets, validation logic, metadata
    editors, asset helpers, and internal systems.

- MetaWeaver does not include runtime modules. All functionality is **Editor-only**, including:
  - The Metadata Editor window
  - Bulk-editing metadata matrix
  - MetadataParameterDefinition & MetadataDefinitionSet asset types
  - Context-menu integration for Content Browser assets
  - All typed metadata widgets and validation routines

- Raw source files (such as temporary schema tables or importable metadata definitions) belong in `SourceContent/`.
  `Content/` is reserved only for runtime assets shipped with the plugin if needed (though MetaWeaver typically does not
  require any).

- Generated binaries and build artifacts must remain out of version control.

- Keep `README.md` aligned with new features or workflow changes so downstream users stay informed.

## Tooling & Engine Version

- Target Unreal Engine **5.6** for development and validation.  
  Earlier engine versions are unsupported.

## General Principles

- **Readability:** Code should be clear and easy to understand. Avoid unnecessarily clever or obscure solutions.
- **Consistency:** Follow consistent naming, layout, and architectural conventions throughout the plugin.
- **Simplicity (KISS):** Favor simple solutions when they fully address the problem.
- **DRY:** Avoid duplication; use reusable functions, components, editors, or panels wherever possible.
- **Commenting:**
  - Document non-obvious or critical logic.
  - Focus comments on *why* something is implemented a certain way when the *what* is already clear.
  - Keep comments aligned with code changes.
- **Modularity:** Each component should be self-contained and reusable. Definitions, editors, validators, and metadata
  rules should remain decoupled where possible.
- **Performance:** Consider performance impact for metadata queries and validation routines, especially when operating
  across large asset sets during bulk editing. Optimize hot paths.
- **Error Handling:** Provide clear logging and helpful editor feedback when metadata validation or editing fails.

## Coding Style & Naming Conventions

- Follow Unreal Engine defaults:
  - 4-space indentation
  - PascalCase for types, functions, variables
  - Prefixes: `FStruct`, `UClass`, `EEnum`, `bBooleanFlag`
- Prefer `auto` where intent is clear and remains compatible with Unreal Engine 5.6’s supported C++ language level.
- Public headers belong under `Source/<Module>/Public/<Module>/`, with corresponding implementation files in `Private/`.
- Use minimal includes; prefer forward declarations in headers. Always use `#pragma once`.
- If a header includes `MyFile.generated.h`, the matching `.cpp` must include `#include UE_INLINE_GENERATED_CPP_BY_NAME(MyFile)` placed after all other include directives.
- Use UE logging macros with the `LogMetaWeaver` category; define new log categories in module-private headers if needed.
- Use Doxygen-style comments where useful for API documentation or for editor-facing classes.
- Match existing MetaWeaver code patterns when unclear.
- Use `FScopedTransaction` for every metadata apply path; name transactions consistently.

## Testing Guidelines

- Automated testing for MetaWeaver is aspirational and will expand over time. Unit-style tests will live under
  `Source/<Module>/Private/Tests/` once the harness is added.
- Until then, document manual test steps in pull requests, particularly:
  - Metadata editing in the standalone editor
  - Bulk metadata changes using the matrix tool
  - Parameter type enforcement and validation messages
  - MetadataDefinitionSet aggregation behavior
- Provide sample maps, test assets, or screenshots when editor UX is impacted.

## Commit & Pull Request Guidelines

- Use imperative, present-tense commit messages under **72 characters**.
- Squash noisy or temporary work-in-progress commits; each commit should represent a meaningful change.
- Pull requests should include:
  - A clear summary of the change
  - Any reproduction steps or testing notes
  - Visuals (screenshots or GIFs) for UI or editor-workflow changes
  - Updates to documentation if required
