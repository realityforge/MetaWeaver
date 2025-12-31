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
#include "Widgets/SCompoundWidget.h"

class SSearchBox;
class ITableRow;
class STableViewBase;
template <typename ItemType>
class SListView;
class SMetaWeaverBulkRow;

/**
 * The bulk metadata editor.
 */
class SMetaWeaverBulkEditor final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMetaWeaverBulkEditor) {}
    SLATE_ARGUMENT(TArray<FAssetData>, SelectedAssets)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SMetaWeaverBulkEditor() override;

    // Accessors for row rendering
    int32 IndexOfAsset(const FAssetData& Asset) const;
    void GetCellState(int32 RowIndex, FName Key, bool& bOutApplicable, bool& bOutHasTag, FString& OutValue) const;
    // Editing helpers (used by row/cell editors)
    void CommitCellValue(int32 RowIndex, FName Key, const FString& NewValue);
    bool GetSpecFor(int32 RowIndex, FName Key, FMetadataParameterSpec& OutSpec) const;
    const TArray<TSharedPtr<FString>>& EnsureEnumOptions(const FMetadataParameterSpec& Spec);

    static void MarkAssetDirty(const UObject* Asset);

    /**
     * Asset at RowIndex has been changed. Make sure we rebuild relevant caches and reset errors.
     *
     * @param RowIndex The RowIndex.
     */
    void SyncAssetMetaDataState(int32 RowIndex);

    /**
     * MetaData key for Asset at RowIndex has been changed.
     * Mark the asset as Dirty a make sure we rebuild relevant caches and reset errors.
     *
     * @param Asset The Asset.
     * @param RowIndex The RowIndex.
     * @param Key The MetaData Key.
     */
    void UpdateAssetMetaDataState(const UObject* Asset, int32 RowIndex, FName Key);

    // Column-level batch operations
    void ApplyColumnValueToAll(FName Key, const FString& NewValue);
    void ValidateThenSetMetaDataTag(UObject* Asset, int32 RowIndex, FName Key, const FString& Value);
    void ResetColumnForAll(FName Key);
    void RemoveColumnForAll(FName Key);

private:
    friend class SMetaWeaverBulkRow;
    TArray<FAssetData> SelectedAssets;

    // Per-asset computed state
    struct FPerAssetComputed
    {
        TMap<FName, FMetadataParameterSpec> Specs; // effective specs per key
        TMap<FName, FString> Tags;                 // current tags per key
    };

// UI state for which metadata keys are available and pinned
#pragma region Metadata Keys UI
    // Candidate keys and pinned columns
    struct FMetaDataColumnDefinition
    {
        FName Key;
    };

    TSharedPtr<SListView<TSharedPtr<FMetaDataColumnDefinition>>> CandidateColumnListView;
    TArray<FName> PinnedKeys;
    TArray<TSharedPtr<FMetaDataColumnDefinition>> CandidateColumns;
    TArray<TSharedPtr<FMetaDataColumnDefinition>> FilteredCandidateColumns;
    void ApplyCandidateColumnFilter();
    void RebuildCandidateColumnListView();
    void OnCandidateColumnFilterChanged(const FText& NewText);
    void TogglePinned(const FName Key, bool bPinned);
#pragma endregion

    // Matrix (rows = assets, columns = pinned keys)
    TSharedPtr<SListView<TSharedPtr<FAssetData>>> ListView;
    TSharedPtr<SBox> MatrixContainer;
    TSharedPtr<SSearchBox> KeySearchBox;
    TArray<TSharedPtr<FAssetData>> AssetItems; // backing store for asset rows
    TArray<FPerAssetComputed> PerAsset;        // parallel to SelectedAssets
    TMap<FName, TArray<TSharedPtr<FString>>> EnumOptionsCache;
    bool bLockToSelection{ false };

#pragma region Cell Error Handlers
    // Inline cell error feedback storage: RowIndex -> Key -> Message
    TMap<int32, TMap<FName, FText>> CellErrors;

    // Inline cell error helpers
    bool GetCellError(int32 RowIndex, FName Key, FText& OutMessage) const;
    void SetCellError(int32 RowIndex, FName Key, const FText& Message);
    void ClearCellError(int32 RowIndex, FName Key);
    void ClearAllErrorsForRow(int32 RowIndex);
    void ClearAllErrors();
    bool ValidateMetaDataValue(const UObject* Asset, int32 RowIndex, FName Key, const FString& Value);
#pragma endregion

    void RefreshListView() const;
    void UpdateAssetItemAtIndex(int32 RowIndex);
    void UpdatePerAssetData(int32 RowIndex);
    TArray<int32> GetRowIndexesForAsset(const UObject* Object);

    // Build and update helpers
    void BuildUI();
    void RecomputeCandidateColumnsAndPerAsset();
    void RebuildMatrix();
    TSharedRef<ITableRow> OnGenerateAssetRow(TSharedPtr<FAssetData> Item, const TSharedRef<STableViewBase>& OwnerTable);

    FString DeriveColumnDescription(const FName& Key);
    TOptional<EMetaWeaverValueType> DeriveColumnType(const FName& Key) const;
    const TArray<TSharedPtr<FString>>& BuildHeaderEnumOptions(const FName& Key);
    TMap<FName, TArray<TSharedPtr<FString>>> HeaderEnumOptionsCache;
    UClass* DeriveHeaderAllowedClass(const FName& Key) const;

    bool IsApplyEnabled(FName Key, const FString& Value) const;
    bool IsResetEnabled(FName Key) const;
    bool IsDeleteEnabled(FName Key) const;

    // Row actions
    void ShowInContentBrowser(const FAssetData& Asset) const;
    void OpenAssetEditor(const FAssetData& Asset) const;

#pragma region Event handlers
    FDelegateHandle ObjectModifiedHandle;
    FDelegateHandle AssetRemovedHandle;
    FDelegateHandle AssetRenamedHandle;
    FDelegateHandle AssetUpdatedHandle;
    FDelegateHandle ContentBrowserSelectionHandle;
    FDelegateHandle DefinitionSetsChangedHandle;

    void OnObjectModified(UObject* Object);
    void OnAssetRegistryAssetRemoved(const FAssetData& RemovedAsset);
    void OnAssetRegistryAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
    void OnAssetRegistryAssetUpdated(const FAssetData& UpdatedAsset);
    void OnContentBrowserAssetSelectionChanged(const TArray<FAssetData>& NewSelectedAssets, bool bIsPrimaryBrowser);
    void OnDefinitionSetsChanged();
#pragma endregion
};
