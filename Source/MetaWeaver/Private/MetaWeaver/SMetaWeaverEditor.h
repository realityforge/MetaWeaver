/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "MetaWeaver/MetaWeaverMetadataDefinitionSet.h"
#include "MetaWeaver/Validation/MetaWeaverValidationTypes.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SSearchBox;
template <typename ItemType>
class SListView;
class ITableRow;
class STableViewBase;

struct FTagItem
{
    FName Key;
    FString Value;
    FMetadataParameterSpec Spec;
    // Persistent options for enum editors. SComboBox requires OptionsSource to
    // outlive the widget so can NOT build a temporary array in OnGenerateRow.
    // Only populated when Spec.Type == Enum.
    TArray<TSharedPtr<FString>> EnumOptions;

    // Whether the metadata tag currently exists on the asset.
    bool bHasTag{ false };

    // Validation state for this row (optional)
    TOptional<EMetaWeaverIssueSeverity> Severity;
    FString ValidationMessage;

    bool IsUnsaved() const;
};

/**
 * Minimal editor widget wired to the metadata store.
 * Operates on the first selected asset (if any).
 */
class SMetaWeaverEditor final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMetaWeaverEditor) {}
    SLATE_ARGUMENT(TArray<FAssetData>, SelectedAssets)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SMetaWeaverEditor() override;
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

    // Public helpers used by row widgets
    UObject* GetFirstAssetForUI() const { return ResolveFirstAsset(); }
    void RevalidateUI() { RefreshValidation(); }
    void RefreshListUI() const { RefreshListView(); }
    void RebuildTagListUI() { RebuildTagListItems(); }

    // Add-row validation helpers
    bool IsAddEnabled() const;
    FText GetAddErrorText() const;

private:
    UObject* ResolveFirstAsset() const;
    void RebuildTagListItems();
    void ClearAddFields() const;
    void RefreshListView() const;
    void RefreshValidation();
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FTagItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void ApplyFilter();
    void OnFilterChanged(const FText& NewText);

    void ShowSelectedInContentBrowser();
    void OpenAssetEditor();

    // Save any other keys that have defaults defined but are not yet saved on the asset.
    void SaveAnyUnsavedDefaults(TOptional<FName> ExcludeKey = TOptional<FName>());
    void MarkAssetDirty(const UObject* Asset);

    // Methods that perform actions on metadata tags
    void OnAddMetadataTag();
    void OnResetMetadataTag(FTagItem& Item);
    void OnEditMetadataTag(FTagItem& Item, const FString& NewValue);
    void OnRemoveMetadataTag(const FTagItem& Item);

    const FSlateBrush* GetSelectedAssetBrush();
    FText GetSelectedAssetToolTip();

    // Selection change handlers
    void OnContentBrowserAssetSelectionChanged(const TArray<FAssetData>& NewSelectedAssets, bool bIsPrimaryBrowser);
    void OnAssetRegistryAssetRemoved(const FAssetData& RemovedAsset);
    void OnAssetRegistryAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
    void OnObjectModified(UObject* Object);
    FText BuildSelectionSummaryText() const;
    FText BuildSelectionMessageText() const;
    TSharedRef<SWidget> BuildTopBar();
    EVisibility ControlsVisibility() const;

    TArray<FAssetData> SelectedAssets;
    TSharedPtr<SEditableTextBox> KeyText;
    TSharedPtr<SEditableTextBox> ValueText;
    TSharedPtr<SVerticalBox> TagListBox;
    TArray<TSharedPtr<FTagItem>> TagItems;
    TSharedPtr<SListView<TSharedPtr<FTagItem>>> TagListView;
    TArray<TSharedPtr<FTagItem>> FilteredTagItems;
    TSharedPtr<SSearchBox> SearchBox;
    FString CurrentFilter;
    TSharedPtr<SEditableTextBox> NewKeyText;
    TSharedPtr<SEditableTextBox> NewValueText;

    // Validation cache for current selection
    int32 ValidationErrorCount{ 0 };
    int32 ValidationWarningCount{ 0 };

    // Keys sourced from the definition set for the current asset class (plus existing tags)
    TSet<FName> DefinedKeys;

    // Selection sync state
    bool bLockToSelection{ false };
    enum class ESelectionViewState : uint8
    {
        None,
        Single,
        Multiple
    };
    ESelectionViewState CurrentViewState{ ESelectionViewState::None };

    // External refresh trigger
    bool bPendingExternalRefresh{ false };
    double NextExternalRefreshTime{ -1.0 };
    FDelegateHandle ObjectModifiedHandle;
    FDelegateHandle ContentBrowserSelectionHandle;
    FDelegateHandle AssetRemovedHandle;
    FDelegateHandle AssetRenamedHandle;
    FDelegateHandle DefinitionSetsChangedHandle;

    friend class SMetaWeaverRow;

    void OnDefinitionSetsChanged();
};
