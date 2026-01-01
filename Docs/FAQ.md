# FAQ

## What Unreal versions are supported?
Unreal Engine 5.6+ (Editor‑only). Earlier versions are unsupported.

## Are enums Unreal UEnums?
No. Enums are string lists defined per parameter. Values are trimmed, non‑empty, unique; optional Exclusive mode enforces membership.

## Can other modules validate assets?
Yes. Use `UMetaWeaverValidationSubsystem` in the editor to validate assets and retrieve a report of issues.

## Does MetaWeaver work at runtime?
No. The plugin is Editor‑only. There is no runtime module.

## Where is the Bulk Editor?
Content Browser → Asset Actions → Bulk Edit Metadata…

## How are undo/redo handled?
Edits are transacted. Bulk column operations group into a single, descriptive transaction.
