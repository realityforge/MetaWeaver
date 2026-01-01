# Getting Started

This guide walks through installing MetaWeaver, enabling definition sets, and using the single‑asset and bulk editors.

## Requirements

- Unreal Engine 5.6+
- Editor‑only plugin (no runtime modules)

## Install

1. Copy the `MetaWeaver` directory into your project’s `Plugins/` folder.
1. Restart the editor. Ensure the MetaWeaver plugin is enabled.

## Configure

1. Project Settings → MetaWeaver → Active Definition Sets
1. Add one or more `UMetaWeaverMetadataDefinitionSet` assets that define your metadata keys and types.

<figure>
  <img src="Images/ProjectSettings.png" alt="Project Settings window with MetaWeaver section selected, showing an array property called Active Definition Sets containing one UMetaWeaverMetadataDefinitionSet asset.">
  <figcaption>Project Settings → MetaWeaver: add one Definition Sets to “Active Definition Sets”.</figcaption>
  </figure>

## Single‑Asset Editing

1. In the Content Browser, right‑click an asset → Asset Actions → Edit Metadata…
    
    <figure>
      <img src="Images/ContextMenu.png" alt="Content Browser context menu open for an asset, Asset Actions submenu expanded showing 'Edit Metadata…' and 'Bulk Edit Metadata…' entries.">
      <figcaption>Content Browser → Asset Actions</figcaption>
    </figure>
1. Edit values using typed controls (Bool, Int, Float, String, Enum, Asset Reference).
1. Inline validation flags missing/invalid values.

<figure>
  <img src="Images/Editor-Single.png" alt="Single‑asset MetaWeaver editor showing a list of metadata keys with typed editors (checkbox for Bool, numeric fields for Int/Float, text box for String, dropdown for Enum, and asset picker for Asset Reference), plus row actions for Show in Content Browser and Open Asset Editor.">
  <figcaption>Single‑asset editor with typed controls and row actions (Show/Open). Include at least one of each type if possible.</figcaption>
</figure>

## Bulk Editing (Matrix)
- Select multiple assets in the Content Browser → Asset Actions → Bulk Edit Metadata…
- Enable columns for keys you want to view/edit. Apply/Reset/Remove per column.
- Row actions include Show in Content Browser and Open Asset Editor.

<figure>
  <img src="Images/Editor-Bulk.png" alt="Bulk metadata editor matrix view with an Assets column and multiple enabled metadata columns. Column headers show type‑specific editors (e.g., checkbox, numeric entry, enum dropdown). Each row shows asset name with Show and Open icons in separate columns to the left.">
  <figcaption>Bulk editor matrix: demonstrate enabling columns, header typed editors, and per‑row Show/Open icons. Avoid pinned columns; capture a simple, scrollable table.</figcaption>
</figure>

<figure>
  <img src="Images/AssetRef-Picker.png" alt="Asset Reference picker UI open from a cell or header, showing a class filter restricting the asset type (e.g., only Material or Texture).">
  <figcaption>Asset Reference picker with allowed class filter applied. Show the picker filtering to the permitted types.</figcaption>
</figure>

<figure>
  <img src="Images/Enum-Editor.png" alt="Enum dropdown open showing a list of string values defined in the metadata definition set.">
  <figcaption>Enum editor dropdown populated from the definition’s string list. Capture values with trimmed, unique entries.</figcaption>
</figure>

## Validation API
Other editor modules can validate assets via the validation subsystem:

```c++
if (GEditor)
{
    if (auto* Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
    {
        const auto Report = Subsystem->ValidateAsset(Asset);
        // Inspect Report.Issues …
    }
}
```

## Next Steps
- See [Validation](Validation.md) for enum/exclusive list semantics and error reporting.
- See [FAQ](FAQ.md) for common questions and troubleshooting.

<figure>
  <img src="Images/Inline-Errors.png" alt="A metadata cell showing inline validation error styling and an error tooltip indicating why a commit failed (e.g., missing required key or invalid enum choice).">
  <figcaption>Inline validation feedback: show a failed commit with visible error state and tooltip explaining the issue.</figcaption>
</figure>
