# Validation

MetaWeaver validates values against parameter specifications defined in `UMetaWeaverMetadataDefinitionSet`.

<figure>
  <img src="Images/Inline-Errors.png" alt="A metadata editor (single or bulk) showing an inline error state for a cell or header, with a tooltip explaining the validation error (e.g., missing required key, bad format).">
  <figcaption>Inline error feedback on commit: capture a clear example of a failed edit and the explanatory tooltip.</figcaption>
</figure>

## Parameter Types
- Bool, Int, Float, String
- Enum (string list)
  - Values are trimmed, non‑empty, and unique per enum.
  - An enum can be marked Exclusive; when enabled, only listed values are valid.
- Asset Reference
  - Can restrict to an allowed base class.

<figure>
  <img src="Images/Enum-Editor.png" alt="Enum dropdown open in the metadata editor, showing string‑based choices defined in the active definition set; one choice selected.">
  <figcaption>Enum values are strings from the definition set. Capture a dropdown with several choices to illustrate.</figcaption>
</figure>

<figure>
  <img src="Images/AssetRef-Picker.png" alt="Asset Reference picker dialog with a class filter active (e.g., Allowed Class = Material), only compatible assets listed.">
  <figcaption>Asset Reference type with Allowed Class constraint. Show the picker filtered to the allowed base class.</figcaption>
</figure>

## Default Value Canonicalization
- Default values are canonicalized (trimmed/normalized) to ensure consistent comparison.

## Validation Flow
1) Definitions describe required keys and constraints.
2) Editors enforce constraints inline and surface errors on commit.
3) `UMetaWeaverValidationSubsystem` exposes `ValidateAsset()` for programmatic checks.

## Error Reporting
- Missing required keys
- Invalid format/type
- Exclusive enum value not in list
- Asset reference not assignable to allowed class
