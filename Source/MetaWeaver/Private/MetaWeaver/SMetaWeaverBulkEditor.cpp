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
#include "SMetaWeaverBulkEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "IContentBrowserSingleton.h"
#include "MetaWeaver/MetaWeaverMetadataStore.h"
#include "MetaWeaver/Validation/MetaWeaverValidationSubsystem.h"
#include "MetaWeaver/Validation/MetaWeaverValidationTypes.h"
#include "MetaWeaverEditorSettings.h"
#include "MetaWeaverLogging.h"
#include "MetaWeaverStyle.h"
#include "MetaWeaverUIHelpers.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

static const FName NAME_Show("Show");
static const FName NAME_Open("Open");
static const FName NAME_Asset("Asset");

void SMetaWeaverBulkEditor::Construct(const FArguments& InArgs)
{
    SelectedAssets = InArgs._SelectedAssets;

    // Load user preferences before building the UI so initial state is reflected
    if (const auto Settings = GetMutableDefault<UMetaWeaverEditorSettings>())
    {
        bLockToSelection = Settings->bLockToSelectionDefault;
        PinnedKeys = Settings->LastPinnedKeys;
    }
    BuildUI();
    RecomputeCandidateColumnsAndPerAsset();
    RebuildCandidateColumnListView();
    RebuildMatrix();

    // External change notifications
    ObjectModifiedHandle =
        FCoreUObjectDelegates::OnObjectModified.AddSP(this, &SMetaWeaverBulkEditor::OnObjectModified);

    // AssetRegistry updates/removal/rename
    {
        const auto& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        AssetRemovedHandle =
            AssetRegistryModule.Get().OnAssetRemoved().AddSP(this, &SMetaWeaverBulkEditor::OnAssetRegistryAssetRemoved);
        AssetRenamedHandle =
            AssetRegistryModule.Get().OnAssetRenamed().AddSP(this, &SMetaWeaverBulkEditor::OnAssetRegistryAssetRenamed);
        AssetUpdatedHandle =
            AssetRegistryModule.Get().OnAssetUpdated().AddSP(this, &SMetaWeaverBulkEditor::OnAssetRegistryAssetUpdated);
    }

    // Content Browser selection change (primary browser only)
    {
        if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
        {
            auto& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
            ContentBrowserSelectionHandle = CBModule.GetOnAssetSelectionChanged().AddSP(
                this,
                &SMetaWeaverBulkEditor::OnContentBrowserAssetSelectionChanged);
        }
    }

    // Definition set changes
    if (GEditor)
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
        {
            DefinitionSetsChangedHandle =
                Subsystem->GetOnDefinitionSetsChanged().AddSP(this, &SMetaWeaverBulkEditor::OnDefinitionSetsChanged);
        }
    }
}

SMetaWeaverBulkEditor::~SMetaWeaverBulkEditor()
{
    if (ObjectModifiedHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectModified.Remove(ObjectModifiedHandle);
    }
    if (AssetRemovedHandle.IsValid() || AssetRenamedHandle.IsValid() || AssetUpdatedHandle.IsValid())
    {
        if (const auto Module = FModuleManager::Get().GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
        {
            if (AssetRemovedHandle.IsValid())
            {
                Module->Get().OnAssetRemoved().Remove(AssetRemovedHandle);
            }
            if (AssetRenamedHandle.IsValid())
            {
                Module->Get().OnAssetRenamed().Remove(AssetRenamedHandle);
            }
            if (AssetUpdatedHandle.IsValid())
            {
                Module->Get().OnAssetUpdated().Remove(AssetUpdatedHandle);
            }
        }
    }
    if (ContentBrowserSelectionHandle.IsValid())
    {
        if (const auto Module = FModuleManager::Get().GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
        {
            Module->GetOnAssetSelectionChanged().Remove(ContentBrowserSelectionHandle);
        }
    }
    // Persist preferences on teardown
    if (const auto Settings = GetMutableDefault<UMetaWeaverEditorSettings>())
    {
        Settings->bLockToSelectionDefault = bLockToSelection;
        Settings->LastPinnedKeys = PinnedKeys;
        Settings->SaveConfig();
    }
    if (DefinitionSetsChangedHandle.IsValid())
    {
        if (GEditor)
        {
            if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
            {
                Subsystem->GetOnDefinitionSetsChanged().Remove(DefinitionSetsChangedHandle);
            }
        }
    }
}

void SMetaWeaverBulkEditor::BuildUI()
{
    ChildSlot
        [SNew(SVerticalBox)
         // Header bar
         + SVerticalBox::Slot().AutoHeight().Padding(
             8.f)[SNew(SHorizontalBox)
                  + SHorizontalBox::Slot().AutoWidth().VAlign(
                      VAlign_Center)[SNew(SImage).Image(FMetaWeaverStyle::GetDocumentBrush())]
                  + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f).VAlign(
                      VAlign_Center)[SNew(STextBlock).Text_Lambda([this] {
                        return FText::FromString(FString::Printf(TEXT("Bulk edit — %d assets"), SelectedAssets.Num()));
                    })]
                  + SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f).VAlign(
                      VAlign_Center)[SNew(SButton)
                                         .ToolTipText(FText::FromString(TEXT("Lock to Content Browser selection")))
                                         .ButtonStyle(&FMetaWeaverStyle::GetButtonStyle())
                                         .OnClicked_Lambda([this] {
                                             bLockToSelection = !bLockToSelection;
                                             return FReply::Handled();
                                         })[SNew(SImage).Image_Lambda([this] {
                                             return FMetaWeaverStyle::GetLockBrush(bLockToSelection);
                                         })]]
                  + SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]]

         // Body: Sidebar (Pinned Columns) + Matrix
         + SVerticalBox::Slot().FillHeight(1.f).Padding(
             8.f)[SNew(SHorizontalBox)
                  // Sidebar
                  + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
                        [SNew(SVerticalBox)
                         + SVerticalBox::Slot()
                               .AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Pinned Columns")))]
                         + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
                               [SAssignNew(KeySearchBox, SSearchBox)
                                    .OnTextChanged_Raw(this, &SMetaWeaverBulkEditor::OnCandidateColumnFilterChanged)]
                         + SVerticalBox::Slot().FillHeight(
                             1.f)[SAssignNew(CandidateColumnListView, SListView<TSharedPtr<FMetaDataColumnDefinition>>)
                                      .ListItemsSource(&FilteredCandidateColumns)
                                      .OnGenerateRow_Lambda([this](TSharedPtr<FMetaDataColumnDefinition> Item,
                                                                   const TSharedRef<STableViewBase>& Owner) {
                                          return SNew(
                                              STableRow<TSharedPtr<FMetaDataColumnDefinition>>,
                                              Owner)[SNew(SHorizontalBox)
                                                     + SHorizontalBox::Slot().AutoWidth().VAlign(
                                                         VAlign_Center)[SNew(SCheckBox)
                                                                            .IsChecked_Lambda([this, Item] {
                                                                                return PinnedKeys.Contains(Item->Key)
                                                                                    ? ECheckBoxState::Checked
                                                                                    : ECheckBoxState::Unchecked;
                                                                            })
                                                                            .OnCheckStateChanged_Lambda(
                                                                                [this, Item](auto NewState) {
                                                                                    TogglePinned(Item->Key,
                                                                                                 ECheckBoxState::Checked
                                                                                                     == NewState);
                                                                                })]
                                                     + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f).VAlign(
                                                         VAlign_Center)[SNew(STextBlock)
                                                                            .Text(FText::FromName(Item->Key))
                                                                            .ToolTipText_Lambda([this, Item] {
                                                                                const auto Desc =
                                                                                    DeriveColumnDescription(Item->Key);
                                                                                return Desc.IsEmpty()
                                                                                    ? FText::GetEmpty()
                                                                                    : FText::FromString(Desc);
                                                                            })]];
                                      })]]

                  // Matrix container
                  + SHorizontalBox::Slot().FillWidth(1.f)[SAssignNew(MatrixContainer, SBox)]]];
}

void SMetaWeaverBulkEditor::RecomputeCandidateColumnsAndPerAsset()
{
    const auto Count = SelectedAssets.Num();
    PerAsset.Reset();
    PerAsset.SetNum(Count);
    CandidateColumns.Reset();
    TSet<FName> Keys;

    for (auto i = 0; i < Count; ++i)
    {
        UpdatePerAssetData(i);
        for (const auto& Pair : PerAsset[i].Specs)
        {
            Keys.Add(Pair.Key);
        }
        for (const auto& Pair : PerAsset[i].Tags)
        {
            Keys.Add(Pair.Key);
        }
    }

    for (const auto& Key : Keys)
    {
        CandidateColumns.Add(MakeShared<FMetaDataColumnDefinition>(Key));
    }
}

void SMetaWeaverBulkEditor::RebuildMatrix()
{
    const auto Header = SNew(SHeaderRow)
        + SHeaderRow::Column(NAME_Show).FixedWidth(28.f).DefaultLabel(FText::FromString(TEXT("")))
        + SHeaderRow::Column(NAME_Open).FixedWidth(28.f).DefaultLabel(FText::FromString(TEXT("")))
        + SHeaderRow::Column(NAME_Asset).FillWidth(0.3f).DefaultLabel(FText::FromString(TEXT("Asset")));
    const float KeyCols = PinnedKeys.Num() > 0 ? 0.7f / PinnedKeys.Num() : 0.7f;
    for (const auto& Key : PinnedKeys)
    {
        TSharedPtr<SEditableTextBox> HeaderText;
        TSharedPtr<SCheckBox> HeaderBool;
        TSharedPtr<SNumericEntryBox<int64>> HeaderInt;
        TSharedPtr<int64> HeaderIntValue;
        TSharedPtr<SNumericEntryBox<double>> HeaderFloat;
        TSharedPtr<double> HeaderFloatValue;
        TArray<TSharedPtr<FString>> HeaderEnumValues;
        TSharedPtr<SComboBox<TSharedPtr<FString>>> HeaderEnum;
        TSharedPtr<FString> HeaderEnumSelected;
        TSharedPtr<FString> HeaderAssetPath;
        const auto Description = DeriveColumnDescription(Key);

        // Derive a coherent header editor type if possible
        const auto TypeOpt = DeriveColumnType(Key);
        const bool bMixedTypes = !TypeOpt.IsSet();
        const auto HeaderType = TypeOpt.Get(EMetaWeaverValueType::String);

        Header->AddColumn(
            SHeaderRow::Column(Key).FillWidth(KeyCols).HeaderContent()
                [SNew(SVerticalBox)
                 + SVerticalBox::Slot().AutoHeight()
                       [SNew(STextBlock)
                            .Text(FText::FromName(Key))
                            .ToolTipText(Description.IsEmpty() ? FText::GetEmpty() : FText::FromString(Description))]
                 + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[SNew(SBox).HAlign(
                     HAlign_Fill)[bMixedTypes ? SAssignNew(HeaderText, SEditableTextBox)
                                                    .HintText(FText::FromString(TEXT("Value")))
                                              : [&]() -> TSharedRef<SWidget> {
                       switch (HeaderType)
                       {
                           case EMetaWeaverValueType::Bool:
                           {
                               SAssignNew(HeaderBool, SCheckBox).IsChecked(ECheckBoxState::Unchecked);
                               return HeaderBool.ToSharedRef();
                           }
                           case EMetaWeaverValueType::Integer:
                           {
                               HeaderIntValue = MakeShared<int64>(0);
                               SAssignNew(HeaderInt, SNumericEntryBox<int64>)
                                   .AllowSpin(true)
                                   .MinDesiredValueWidth(60.f)
                                   .Value_Lambda([HeaderIntValue]() -> TOptional<int64> {
                                       return HeaderIntValue.IsValid() ? TOptional(*HeaderIntValue)
                                                                       : TOptional<int64>();
                                   })
                                   .OnValueChanged_Lambda(
                                       [HeaderIntValue](const int64 NewVal) { *HeaderIntValue = NewVal; });
                               return HeaderInt.ToSharedRef();
                           }
                           case EMetaWeaverValueType::Float:
                           {
                               HeaderFloatValue = MakeShared<double>(0.0);
                               SAssignNew(HeaderFloat, SNumericEntryBox<double>)
                                   .AllowSpin(true)
                                   .MinDesiredValueWidth(60.f)
                                   .Value_Lambda([HeaderFloatValue]() -> TOptional<double> {
                                       return HeaderFloatValue.IsValid() ? TOptional(*HeaderFloatValue)
                                                                         : TOptional<double>();
                                   })
                                   .OnValueChanged_Lambda(
                                       [HeaderFloatValue](double NewVal) { *HeaderFloatValue = NewVal; });
                               return HeaderFloat.ToSharedRef();
                           }
                           case EMetaWeaverValueType::Enum:
                           {
                               const auto& OptionsRef = BuildHeaderEnumOptions(Key);
                               HeaderEnumSelected = MakeShared<FString>(TEXT(""));
                               SAssignNew(HeaderEnum, SComboBox<TSharedPtr<FString>>)
                                   .OptionsSource(&OptionsRef)
                                   .OnGenerateWidget_Lambda([](const TSharedPtr<FString>& InItem) {
                                       return SNew(STextBlock)
                                           .Text(FText::FromString(InItem.IsValid() ? *InItem : TEXT("")));
                                   })
                                   .OnSelectionChanged_Lambda(
                                       [HeaderEnumSelected](const TSharedPtr<FString>& NewItem, ESelectInfo::Type) {
                                           if (NewItem.IsValid())
                                           {
                                               *HeaderEnumSelected = *NewItem;
                                           }
                                       })[SNew(STextBlock).Text_Lambda([HeaderEnumSelected]() -> FText {
                                       return HeaderEnumSelected.IsValid() ? FText::FromString(*HeaderEnumSelected)
                                                                           : FText();
                                   })];
                               return HeaderEnum.ToSharedRef();
                           }
                           case EMetaWeaverValueType::AssetReference:
                           {
                               const auto Allowed = DeriveHeaderAllowedClass(Key);
                               HeaderAssetPath = MakeShared<FString>();
                               return SNew(SObjectPropertyEntryBox)
                                   .AllowedClass(Allowed)
                                   .AllowClear(true)
                                   .DisplayUseSelected(true)
                                   .DisplayBrowse(true)
                                   .OnObjectChanged_Lambda([HeaderAssetPath](const FAssetData& NewAsset) {
                                       *HeaderAssetPath =
                                           NewAsset.IsValid() ? NewAsset.ToSoftObjectPath().ToString() : FString();
                                   });
                           }
                           case EMetaWeaverValueType::String:
                           default:
                           {
                               return SAssignNew(HeaderText, SEditableTextBox)
                                   .HintText(FText::FromString(TEXT("Value")));
                           }
                       }
                   }()]]
                 + SVerticalBox::Slot().AutoHeight()
                       [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
                              [SNew(SButton)
                                   .ToolTipText(FText::FromString(TEXT("Apply Value Change")))
                                   .IsEnabled_Lambda([this,
                                                      Key,
                                                      HeaderText,
                                                      HeaderBool,
                                                      HeaderIntValue,
                                                      HeaderFloatValue,
                                                      HeaderEnumSelected,
                                                      HeaderAssetPath,
                                                      bMixedTypes,
                                                      HeaderType] {
                                       auto CurrentVal = [&]() -> FString {
                                           if (bMixedTypes)
                                           {
                                               return HeaderText.IsValid() ? HeaderText->GetText().ToString()
                                                                           : FString();
                                           }
                                           switch (HeaderType)
                                           {
                                               case EMetaWeaverValueType::Bool:
                                                   return HeaderBool.IsValid()
                                                           && HeaderBool->GetCheckedState() == ECheckBoxState::Checked
                                                       ? TEXT("True")
                                                       : TEXT("False");
                                               case EMetaWeaverValueType::Integer:
                                                   return HeaderIntValue.IsValid() ? LexToString(*HeaderIntValue)
                                                                                   : FString();
                                               case EMetaWeaverValueType::Float:
                                                   return HeaderFloatValue.IsValid() ? LexToString(*HeaderFloatValue)
                                                                                     : FString();
                                               case EMetaWeaverValueType::Enum:
                                                   return HeaderEnumSelected.IsValid() ? *HeaderEnumSelected
                                                                                       : FString();
                                               case EMetaWeaverValueType::AssetReference:
                                                   return HeaderAssetPath.IsValid() ? *HeaderAssetPath : FString();
                                               case EMetaWeaverValueType::String:
                                               default:
                                                   return HeaderText.IsValid() ? HeaderText->GetText().ToString()
                                                                               : FString();
                                           }
                                       }();
                                       return IsApplyEnabled(Key, CurrentVal);
                                   })
                                   .OnClicked_Lambda([this,
                                                      Key,
                                                      HeaderText,
                                                      HeaderBool,
                                                      HeaderIntValue,
                                                      HeaderFloatValue,
                                                      HeaderEnumSelected,
                                                      HeaderAssetPath,
                                                      bMixedTypes,
                                                      HeaderType] {
                                       FString NewVal;
                                       if (bMixedTypes)
                                       {
                                           NewVal = HeaderText.IsValid() ? HeaderText->GetText().ToString() : FString();
                                       }
                                       else
                                       {
                                           switch (HeaderType)
                                           {
                                               case EMetaWeaverValueType::Bool:
                                                   NewVal = HeaderBool.IsValid()
                                                           && HeaderBool->GetCheckedState() == ECheckBoxState::Checked
                                                       ? TEXT("True")
                                                       : TEXT("False");
                                                   break;
                                               case EMetaWeaverValueType::Integer:
                                                   NewVal = HeaderIntValue.IsValid() ? LexToString(*HeaderIntValue)
                                                                                     : FString();
                                                   break;
                                               case EMetaWeaverValueType::Float:
                                                   NewVal = HeaderFloatValue.IsValid() ? LexToString(*HeaderFloatValue)
                                                                                       : FString();
                                                   break;
                                               case EMetaWeaverValueType::Enum:
                                                   NewVal =
                                                       HeaderEnumSelected.IsValid() ? *HeaderEnumSelected : FString();
                                                   break;
                                               case EMetaWeaverValueType::AssetReference:
                                                   NewVal = HeaderAssetPath.IsValid() ? *HeaderAssetPath : FString();
                                                   break;
                                               case EMetaWeaverValueType::String:
                                               default:
                                                   NewVal = HeaderText.IsValid() ? HeaderText->GetText().ToString()
                                                                                 : FString();
                                                   break;
                                           }
                                       }
                                       ApplyColumnValueToAll(Key, NewVal);
                                       return FReply::Handled();
                                   })[SNew(SBox)
                                          .HAlign(HAlign_Center)
                                          .VAlign(VAlign_Center)
                                          .Padding(0.f, 2.f, 0.f, 2.f)[SNew(SImage).Image(
                                              FMetaWeaverStyle::GetCheckBrush())]]]
                        + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
                              [SNew(SButton)
                                   .ToolTipText(FText::FromString(TEXT("Reset all values to default")))
                                   .IsEnabled_Lambda([this, Key] { return IsResetEnabled(Key); })
                                   .OnClicked_Lambda([this, Key] {
                                       ResetColumnForAll(Key);
                                       return FReply::Handled();
                                   })[SNew(SBox)
                                          .HAlign(HAlign_Center)
                                          .VAlign(VAlign_Center)
                                          .Padding(0.f, 2.f, 0.f, 2.f)[SNew(SImage).Image(
                                              FMetaWeaverStyle::GetResetToDefaultBrush())]]]
                        + SHorizontalBox::Slot()
                              .AutoWidth()[SNew(SButton)
                                               .ToolTipText(FText::FromString(TEXT("Remove all")))
                                               .IsEnabled_Lambda([this, Key] { return IsDeleteEnabled(Key); })
                                               .OnClicked_Lambda([this, Key] {
                                                   RemoveColumnForAll(Key);
                                                   return FReply::Handled();
                                               })[SNew(SBox)
                                                      .HAlign(HAlign_Center)
                                                      .VAlign(VAlign_Center)
                                                      .Padding(0.f, 2.f, 0.f, 2.f)[SNew(SImage).Image(
                                                          FMetaWeaverStyle::GetDeleteBrush())]]]]]);
    }

    // Asset rows backing store (must outlive the ListView)
    AssetItems.Reset();
    AssetItems.Reserve(SelectedAssets.Num());
    for (const auto& A : SelectedAssets)
    {
        AssetItems.Add(MakeShared<FAssetData>(A));
    }

    const auto NewList = SNew(SListView<TSharedPtr<FAssetData>>)
                             .ListItemsSource(&AssetItems)
                             .OnGenerateRow_Raw(this, &SMetaWeaverBulkEditor::OnGenerateAssetRow)
                             .HeaderRow(Header);

    ListView = NewList;
    if (MatrixContainer.IsValid())
    {
        MatrixContainer->SetContent(NewList);
    }
}

void SMetaWeaverBulkEditor::RebuildCandidateColumnListView()
{
    ApplyCandidateColumnFilter();
    if (CandidateColumnListView.IsValid())
    {
        CandidateColumnListView->RequestListRefresh();
    }
}

void SMetaWeaverBulkEditor::ApplyCandidateColumnFilter()
{
    FilteredCandidateColumns.Reset();
    const auto Filter =
        KeySearchBox.IsValid() ? KeySearchBox->GetText().ToString().TrimStartAndEnd().ToLower() : FString();
    if (Filter.IsEmpty())
    {
        FilteredCandidateColumns = CandidateColumns;
    }
    else
    {
        for (const auto& Key : CandidateColumns)
        {
            if (Key.IsValid() && Key->Key.ToString().ToLower().Contains(Filter))
            {
                FilteredCandidateColumns.Add(Key);
            }
        }
    }
}

void SMetaWeaverBulkEditor::OnCandidateColumnFilterChanged(const FText&)
{
    RebuildCandidateColumnListView();
}

void SMetaWeaverBulkEditor::TogglePinned(const FName Key, const bool bPinned)
{
    if (bPinned)
    {
        if (!PinnedKeys.Contains(Key))
        {
            PinnedKeys.Add(Key);
        }
    }
    else
    {
        PinnedKeys.Remove(Key);
    }
    RebuildMatrix();
}

int32 SMetaWeaverBulkEditor::IndexOfAsset(const FAssetData& Asset) const
{
    return SelectedAssets.IndexOfByKey(Asset);
}

void SMetaWeaverBulkEditor::GetCellState(const int32 RowIndex,
                                         const FName Key,
                                         bool& bOutApplicable,
                                         bool& bOutHasTag,
                                         FString& OutValue) const
{
    bOutApplicable = false;
    bOutHasTag = false;
    OutValue.Reset();
    if (RowIndex < 0 || RowIndex >= PerAsset.Num())
    {
        return;
    }
    const auto& Per = PerAsset[RowIndex];
    const bool bSpec = Per.Specs.Contains(Key);
    if (const auto Found = Per.Tags.Find(Key))
    {
        bOutHasTag = true;
        OutValue = *Found;
    }
    bOutApplicable = bSpec || bOutHasTag;
}

class SMetaWeaverBulkRow final : public SMultiColumnTableRow<TSharedPtr<FAssetData>>
{
public:
    SLATE_BEGIN_ARGS(SMetaWeaverBulkRow) {}
    SLATE_ARGUMENT(TSharedPtr<FAssetData>, Item)
    SLATE_ARGUMENT(TWeakPtr<SMetaWeaverBulkEditor>, Editor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
    {
        Item = InArgs._Item;
        Editor = InArgs._Editor;
        SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InOwnerTableView);
    }

    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
    {
        auto PinnedEditor = Editor.Pin();
        if (!PinnedEditor.IsValid())
        {
            return SNew(STextBlock).Text(FText());
        }

        const int32 RowIndex = PinnedEditor->IndexOfAsset(*Item);
        if (NAME_Show == ColumnName)
        {
            return SNew(SButton)
                .ButtonStyle(&FMetaWeaverStyle::GetButtonStyle())
                .IsEnabled_Lambda([PinnedEditor, RowIndex] {
                    if (!PinnedEditor.IsValid() || !PinnedEditor->SelectedAssets.IsValidIndex(RowIndex))
                    {
                        return false;
                    }
                    return PinnedEditor->SelectedAssets[RowIndex].GetAsset() != nullptr;
                })
                .ToolTipText_Lambda([PinnedEditor, RowIndex] {
                    const bool bOk = PinnedEditor.IsValid() && PinnedEditor->SelectedAssets.IsValidIndex(RowIndex)
                        && PinnedEditor->SelectedAssets[RowIndex].GetAsset() != nullptr;
                    return bOk ? FText::FromString(TEXT("Show in Content Browser"))
                               : FText::FromString(TEXT("Asset not loaded"));
                })
                .OnClicked_Lambda([PinnedEditor, RowIndex]() {
                    if (PinnedEditor.IsValid() && PinnedEditor->SelectedAssets.IsValidIndex(RowIndex))
                    {
                        PinnedEditor->ShowInContentBrowser(PinnedEditor->SelectedAssets[RowIndex]);
                    }
                    return FReply::Handled();
                })[SNew(SImage).Image(FMetaWeaverStyle::GetSearchBrush())];
        }
        else if (NAME_Open == ColumnName)
        {
            return SNew(SButton)
                .ButtonStyle(&FMetaWeaverStyle::GetButtonStyle())
                .IsEnabled_Lambda([PinnedEditor, RowIndex] {
                    if (!PinnedEditor.IsValid() || !PinnedEditor->SelectedAssets.IsValidIndex(RowIndex))
                    {
                        return false;
                    }
                    return PinnedEditor->SelectedAssets[RowIndex].GetAsset() != nullptr;
                })
                .ToolTipText_Lambda([PinnedEditor, RowIndex] {
                    const bool bOk = PinnedEditor.IsValid() && PinnedEditor->SelectedAssets.IsValidIndex(RowIndex)
                        && PinnedEditor->SelectedAssets[RowIndex].GetAsset() != nullptr;
                    return bOk ? FText::FromString(TEXT("Open Asset Editor"))
                               : FText::FromString(TEXT("Asset not loaded"));
                })
                .OnClicked_Lambda([PinnedEditor, RowIndex]() {
                    if (PinnedEditor.IsValid() && PinnedEditor->SelectedAssets.IsValidIndex(RowIndex))
                    {
                        PinnedEditor->OpenAssetEditor(PinnedEditor->SelectedAssets[RowIndex]);
                    }
                    return FReply::Handled();
                })[SNew(SImage).Image(FMetaWeaverStyle::GetEditBrush())];
        }
        else if (NAME_Asset == ColumnName)
        {
            // Show live asset name to reflect renames without forcing a full rebuild
            return SNew(STextBlock)
                .Text_Lambda([PinnedEditor, RowIndex]() -> FText {
                    return !PinnedEditor.IsValid() || !PinnedEditor->SelectedAssets.IsValidIndex(RowIndex)
                        ? FText()
                        : FText::FromName(PinnedEditor->SelectedAssets[RowIndex].AssetName);
                })
                .ToolTipText_Lambda([PinnedEditor, RowIndex]() -> FText {
                    return !PinnedEditor.IsValid() || !PinnedEditor->SelectedAssets.IsValidIndex(RowIndex)
                        ? FText()
                        : FText::FromString(PinnedEditor->SelectedAssets[RowIndex].ToSoftObjectPath().ToString());
                });
        }
        else if (INDEX_NONE != RowIndex)
        {
            const FName Key = ColumnName;
            bool bApplicable = false;
            bool bHasTag = false;
            FString Value;
            PinnedEditor->GetCellState(RowIndex, Key, bApplicable, bHasTag, Value);

            if (!bApplicable)
            {
                return SNew(STextBlock)
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
                    .ToolTipText(FText::FromString(TEXT("Not applicable for this asset")))
                    .Text(FText::FromString(TEXT("—")));
            }

            // Determine spec (type)
            FMetadataParameterSpec Spec;
            const bool bHasSpec = PinnedEditor->GetSpecFor(RowIndex, Key, Spec);
            // ReSharper disable once CppTooWideScope
            const auto Type = bHasSpec ? Spec.Type : EMetaWeaverValueType::String;

            switch (Type)
            {
                case EMetaWeaverValueType::Bool:
                {
                    return SNew(SCheckBox)
                        .IsChecked_Lambda([PinnedEditor, RowIndex, Key]() -> ECheckBoxState {
                            if (!PinnedEditor.IsValid())
                            {
                                return ECheckBoxState::Undetermined;
                            }
                            bool bApp = false, bHas = false;
                            FString Cur;
                            PinnedEditor->GetCellState(RowIndex, Key, bApp, bHas, Cur);
                            const bool bOn = Cur.Equals(TEXT("True"), ESearchCase::CaseSensitive);
                            return bOn ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                        })
                        .OnCheckStateChanged_Lambda([PinnedEditor, RowIndex, Key](ECheckBoxState NewState) {
                            const bool bNew = NewState == ECheckBoxState::Checked;
                            PinnedEditor->CommitCellValue(RowIndex, Key, bNew ? TEXT("True") : TEXT("False"));
                        });
                }
                case EMetaWeaverValueType::Integer:
                {
                    return SNew(SNumericEntryBox<int64>)
                        .AllowSpin(true)
                        .Value_Lambda([PinnedEditor, RowIndex, Key]() -> TOptional<int64> {
                            if (!PinnedEditor.IsValid())
                            {
                                return TOptional<int64>();
                            }
                            bool bApp = false, bHas = false;
                            FString Cur;
                            PinnedEditor->GetCellState(RowIndex, Key, bApp, bHas, Cur);
                            return TOptional(FCString::Atoi64(*Cur));
                        })
                        .OnValueCommitted_Lambda([PinnedEditor, RowIndex, Key](int64 NewVal, ETextCommit::Type) {
                            PinnedEditor->CommitCellValue(RowIndex, Key, LexToString(NewVal));
                        });
                }
                case EMetaWeaverValueType::Float:
                {
                    return SNew(SNumericEntryBox<double>)
                        .AllowSpin(true)
                        .Value_Lambda([PinnedEditor, RowIndex, Key]() -> TOptional<double> {
                            if (!PinnedEditor.IsValid())
                            {
                                return TOptional<double>();
                            }
                            bool bApp = false, bHas = false;
                            FString Cur;
                            PinnedEditor->GetCellState(RowIndex, Key, bApp, bHas, Cur);
                            return TOptional(FCString::Atod(*Cur));
                        })
                        .OnValueCommitted_Lambda([PinnedEditor, RowIndex, Key](double NewVal, ETextCommit::Type) {
                            PinnedEditor->CommitCellValue(RowIndex, Key, LexToString(NewVal));
                        });
                }
                case EMetaWeaverValueType::Enum:
                {
                    const auto& Options = PinnedEditor->EnsureEnumOptions(Spec);
                    return SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&Options)
                        .OnGenerateWidget_Lambda([](const TSharedPtr<FString>& InItem) {
                            return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : TEXT("")));
                        })
                        .OnSelectionChanged_Lambda(
                            [PinnedEditor, RowIndex, Key](const TSharedPtr<FString>& NewItem, auto) {
                                if (NewItem.IsValid())
                                {
                                    PinnedEditor->CommitCellValue(RowIndex, Key, *NewItem);
                                }
                            })
                        .Content()[SNew(STextBlock).Text_Lambda([PinnedEditor, RowIndex, Key]() -> FText {
                            if (!PinnedEditor.IsValid())
                            {
                                return FText();
                            }
                            bool bApp = false, bHas = false;
                            FString Cur;
                            PinnedEditor->GetCellState(RowIndex, Key, bApp, bHas, Cur);
                            return FText::FromString(Cur);
                        })];
                }
                case EMetaWeaverValueType::AssetReference:
                {
                    const auto Allowed = Spec.AllowedClass ? Spec.AllowedClass.Get() : UObject::StaticClass();
                    return SNew(SObjectPropertyEntryBox)
                        .AllowedClass(Allowed)
                        .AllowClear(true)
                        .DisplayUseSelected(true)
                        .DisplayBrowse(true)
                        .ObjectPath_Lambda([PinnedEditor, RowIndex, Key]() -> FString {
                            if (!PinnedEditor.IsValid())
                            {
                                return FString();
                            }
                            bool bApp = false, bHas = false;
                            FString Cur;
                            PinnedEditor->GetCellState(RowIndex, Key, bApp, bHas, Cur);
                            return Cur;
                        })
                        .OnObjectChanged_Lambda([PinnedEditor, RowIndex, Key](const FAssetData& NewAssetData) {
                            const auto NewVal =
                                NewAssetData.IsValid() ? NewAssetData.ToSoftObjectPath().ToString() : FString();
                            PinnedEditor->CommitCellValue(RowIndex, Key, NewVal);
                        });
                }
                case EMetaWeaverValueType::String:
                default:
                {
                    return SNew(SEditableTextBox)
                        .Text_Lambda([PinnedEditor, RowIndex, Key]() -> FText {
                            if (!PinnedEditor.IsValid())
                            {
                                return FText();
                            }
                            bool bApp = false, bHas = false;
                            FString Cur;
                            PinnedEditor->GetCellState(RowIndex, Key, bApp, bHas, Cur);
                            return FText::FromString(Cur);
                        })
                        .OnTextCommitted_Lambda([PinnedEditor, RowIndex, Key](const FText& NewText, ETextCommit::Type) {
                            PinnedEditor->CommitCellValue(RowIndex, Key, NewText.ToString());
                        });
                }
            }
        }

        return SNew(STextBlock).Text(FText());
    }

private:
    TSharedPtr<FAssetData> Item;
    TWeakPtr<SMetaWeaverBulkEditor> Editor;
};

// ReSharper disable once CppPassValueParameterByConstReference
TSharedRef<ITableRow> SMetaWeaverBulkEditor::OnGenerateAssetRow(TSharedPtr<FAssetData> Item,
                                                                const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SMetaWeaverBulkRow, OwnerTable).Item(Item).Editor(SharedThis(this));
}

FString SMetaWeaverBulkEditor::DeriveColumnDescription(const FName& Key)
{
    if (Key.IsValid())
    {
        for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
        {
            if (PerAsset.IsValidIndex(Row))
            {
                if (const auto Spec = PerAsset[Row].Specs.Find(Key))
                {
                    return Spec->Description;
                }
            }
        }
    }
    return FString();
}

TOptional<EMetaWeaverValueType> SMetaWeaverBulkEditor::DeriveColumnType(const FName& Key) const
{
    TOptional<EMetaWeaverValueType> Result;
    for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
    {
        if (PerAsset.IsValidIndex(Row))
        {
            if (const auto Spec = PerAsset[Row].Specs.Find(Key))
            {
                if (!Result.IsSet())
                {
                    Result = Spec->Type;
                }
                else if (Result.GetValue() != Spec->Type)
                {
                    return TOptional<EMetaWeaverValueType>(); // mixed types
                }
            }
        }
    }
    if (!Result.IsSet())
    {
        // No spec found across selection; default to String
        Result = EMetaWeaverValueType::String;
    }
    return Result;
}

const TArray<TSharedPtr<FString>>& SMetaWeaverBulkEditor::BuildHeaderEnumOptions(const FName& Key)
{
    TArray<TSharedPtr<FString>>& Options = HeaderEnumOptionsCache.FindOrAdd(Key);
    Options.Reset();

    // The full set will contain all the options that are valid across all the specs for this columns
    TSet<FString> FullSet;
    bool bInit = false;
    for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
    {
        if (PerAsset.IsValidIndex(Row))
        {
            if (const auto Spec = PerAsset[Row].Specs.Find(Key))
            {
                if (EMetaWeaverValueType::Enum == Spec->Type)
                {
                    TSet<FString> Local;
                    for (const auto& V : Spec->EnumValues)
                    {
                        Local.Add(V);
                    }
                    if (!bInit)
                    {
                        FullSet = MoveTemp(Local);
                        bInit = true;
                    }
                    else
                    {
                        FullSet = FullSet.Intersect(Local);
                    }
                }
            }
        }
    }
    if (!FullSet.IsEmpty())
    {
        MetaWeaver::UIHelpers::BuildEnumOptionsFromSet(FullSet, Options, /*bSort*/ true);
    }
    return Options;
}

UClass* SMetaWeaverBulkEditor::DeriveHeaderAllowedClass(const FName& Key) const
{
    UClass* Common = nullptr;
    for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
    {
        if (PerAsset.IsValidIndex(Row))
        {
            if (const auto Spec = PerAsset[Row].Specs.Find(Key))
            {
                if (Spec->Type == EMetaWeaverValueType::AssetReference)
                {
                    UClass* Allowed = Spec->AllowedClass ? Spec->AllowedClass.Get() : UObject::StaticClass();
                    if (!Common)
                    {
                        Common = Allowed;
                    }
                    else
                    {
                        if (UClass* Found = UClass::FindCommonBase(Common, Allowed))
                        {
                            Common = Found;
                        }
                        else
                        {
                            Common = UObject::StaticClass();
                        }
                    }
                }
            }
        }
    }
    return Common ? Common : UObject::StaticClass();
}
bool SMetaWeaverBulkEditor::IsApplyEnabled(const FName Key, const FString& Value) const
{
    if (Key.IsValid() && !Value.IsEmpty())
    {
        for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
        {
            if (PerAsset.IsValidIndex(Row))
            {
                // ReSharper disable once CppTooWideScopeInitStatement
                const auto Existing = PerAsset[Row].Tags.Find(Key);
                if (!Existing || !Existing->Equals(Value, ESearchCase::CaseSensitive))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool SMetaWeaverBulkEditor::IsResetEnabled(const FName Key) const
{
    if (Key.IsValid())
    {
        for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
        {
            if (PerAsset.IsValidIndex(Row))
            {
                if (const auto Spec = PerAsset[Row].Specs.Find(Key))
                {
                    // ReSharper disable once CppTooWideScopeInitStatement
                    const auto Value = PerAsset[Row].Tags.Find(Key);
                    if (Value && !Spec->DefaultValue.IsEmpty())
                    {
                        if (!Value->Equals(Spec->DefaultValue, ESearchCase::CaseSensitive))
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool SMetaWeaverBulkEditor::IsDeleteEnabled(const FName Key) const
{
    if (Key.IsValid())
    {
        for (int32 Row = 0; Row < PerAsset.Num(); ++Row)
        {
            if (PerAsset.IsValidIndex(Row))
            {
                const auto Spec = PerAsset[Row].Specs.Find(Key);
                // ReSharper disable once CppTooWideScopeInitStatement
                const auto Value = PerAsset[Row].Tags.Find(Key);
                if (Value && (!Spec || !Spec->bRequired))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool SMetaWeaverBulkEditor::GetSpecFor(const int32 RowIndex, const FName Key, FMetadataParameterSpec& OutSpec) const
{
    if (RowIndex < 0 || RowIndex >= PerAsset.Num())
    {
        return false;
    }
    else if (const auto Found = PerAsset[RowIndex].Specs.Find(Key))
    {
        OutSpec = *Found;
        return true;
    }
    else
    {
        return false;
    }
}

const TArray<TSharedPtr<FString>>& SMetaWeaverBulkEditor::EnsureEnumOptions(const FMetadataParameterSpec& Spec)
{
    return MetaWeaver::UIHelpers::GetOrBuildEnumOptions(EnumOptionsCache, Spec, /*bSort*/ true);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SMetaWeaverBulkEditor::MarkAssetDirty(const UObject* Asset)
{
    if (Asset)
    {
        if (const auto Package = Asset->GetOutermost())
        {
            Package->SetDirtyFlag(true);
        }
    }
}

void SMetaWeaverBulkEditor::SyncAssetMetaDataState(const int32 RowIndex)
{
    UpdateAssetItemAtIndex(RowIndex);
    ClearAllErrorsForRow(RowIndex);
}

void SMetaWeaverBulkEditor::UpdateAssetMetaDataState(const UObject* Asset, const int32 RowIndex, const FName Key)
{
    MarkAssetDirty(Asset);
    ClearCellError(RowIndex, Key);
    UpdateAssetItemAtIndex(RowIndex);
    if (PerAsset.IsValidIndex(RowIndex))
    {
        if (const auto Value = PerAsset[RowIndex].Tags.Find(Key))
        {
            ValidateMetaDataValue(Asset, RowIndex, Key, *Value);
        }
    }
}

void SMetaWeaverBulkEditor::OnDefinitionSetsChanged()
{
    // Specs or defaults changed; clear caches and rebuild matrix & candidates.
    EnumOptionsCache.Reset();
    HeaderEnumOptionsCache.Reset();
    RecomputeCandidateColumnsAndPerAsset();
    RebuildCandidateColumnListView();
    RebuildMatrix();
    ClearAllErrors();
}

void SMetaWeaverBulkEditor::CommitCellValue(const int32 RowIndex, const FName Key, const FString& NewValue)
{
    if (RowIndex >= 0 && RowIndex < SelectedAssets.Num())
    {
        if (const auto Asset = SelectedAssets[RowIndex].GetAsset())
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            const auto Existing = PerAsset.IsValidIndex(RowIndex) ? PerAsset[RowIndex].Tags.Find(Key) : nullptr;
            // Skip no-op edits (value equals existing)
            if (Existing && Existing->Equals(NewValue, ESearchCase::CaseSensitive))
            {
                ClearCellError(RowIndex, Key);
            }
            else
            {
                const auto TxText = FText::Format(NSLOCTEXT("MetaWeaver", "BulkEditCellTransactionFmt", "Edit '{0}'"),
                                                  FText::FromName(Key));
                FScopedTransaction Tx(TxText);
                ValidateThenSetMetaDataTag(Asset, RowIndex, Key, NewValue);
            }
        }
    }
}

void SMetaWeaverBulkEditor::ApplyColumnValueToAll(const FName Key, const FString& NewValue)
{
    TUniquePtr<FScopedTransaction> Tx;
    for (int32 RowIndex = 0; RowIndex < SelectedAssets.Num(); ++RowIndex)
    {
        if (const auto Asset = SelectedAssets[RowIndex].GetAsset())
        {
            if (PerAsset.IsValidIndex(RowIndex))
            {
                const bool bHasSpec = PerAsset[RowIndex].Specs.Contains(Key);
                // ReSharper disable once CppTooWideScopeInitStatement
                const bool bHasTag = PerAsset[RowIndex].Tags.Contains(Key);
                if (bHasSpec || bHasTag)
                {
                    // ReSharper disable once CppTooWideScopeInitStatement
                    const auto Existing = PerAsset[RowIndex].Tags.Find(Key);
                    // Skip no-op edits where existing equals NewValue
                    if (Existing && Existing->Equals(NewValue, ESearchCase::CaseSensitive))
                    {
                        ClearCellError(RowIndex, Key);
                    }
                    else
                    {
                        if (!Tx.IsValid())
                        {
                            Tx = MakeUnique<FScopedTransaction>(
                                FText::Format(NSLOCTEXT("MetaWeaver", "BulkApplyFmt", "Apply '{0}' to selection"),
                                              FText::FromName(Key)));
                        }
                        ValidateThenSetMetaDataTag(Asset, RowIndex, Key, NewValue);
                    }
                }
            }
        }
    }
}

void SMetaWeaverBulkEditor::ValidateThenSetMetaDataTag(UObject* Asset,
                                                       const int32 RowIndex,
                                                       const FName Key,
                                                       const FString& Value)
{
    if (ValidateMetaDataValue(Asset, RowIndex, Key, Value))
    {
        if (FMetaWeaverMetadataStore::SetMetadataTag(Asset, Key, Value))
        {
            UpdateAssetMetaDataState(Asset, RowIndex, Key);
        }
    }
}

void SMetaWeaverBulkEditor::ResetColumnForAll(const FName Key)
{
    TUniquePtr<FScopedTransaction> Tx;
    for (int32 RowIndex = 0; RowIndex < SelectedAssets.Num(); ++RowIndex)
    {
        if (const auto Asset = SelectedAssets[RowIndex].GetAsset())
        {
            if (PerAsset.IsValidIndex(RowIndex))
            {
                if (const auto Spec = PerAsset[RowIndex].Specs.Find(Key))
                {
                    const auto& DefaultValue = Spec->DefaultValue;
                    // ReSharper disable once CppTooWideScopeInitStatement
                    const auto Existing = PerAsset[RowIndex].Tags.Find(Key);
                    // Skip if existing already equals default (or both absent and default empty)
                    if ((Existing && Existing->Equals(DefaultValue, ESearchCase::CaseSensitive))
                        || (!Existing && DefaultValue.IsEmpty()))
                    {
                        ClearCellError(RowIndex, Key);
                    }
                    else
                    {
                        if (!Tx.IsValid())
                        {
                            Tx = MakeUnique<FScopedTransaction>(
                                FText::Format(NSLOCTEXT("MetaWeaver", "BulkResetFmt", "Reset '{0}' for selection"),
                                              FText::FromName(Key)));
                        }
                        if (DefaultValue.IsEmpty())
                        {
                            if (FMetaWeaverMetadataStore::RemoveMetadataTag(Asset, Key))
                            {
                                UpdateAssetMetaDataState(Asset, RowIndex, Key);
                            }
                        }
                        else
                        {
                            ValidateThenSetMetaDataTag(Asset, RowIndex, Key, DefaultValue);
                        }
                    }
                }
            }
        }
    }
}

void SMetaWeaverBulkEditor::RemoveColumnForAll(const FName Key)
{
    TUniquePtr<FScopedTransaction> Tx;
    for (int32 RowIndex = 0; RowIndex < SelectedAssets.Num(); ++RowIndex)
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const auto Asset = SelectedAssets[RowIndex].GetAsset();
        if (Asset && PerAsset.IsValidIndex(RowIndex) && Key.IsValid())
        {
            const auto Spec = PerAsset[RowIndex].Specs.Find(Key);
            const bool bAdHoc = !Spec || !Spec->bRequired;
            // ReSharper disable once CppTooWideScopeInitStatement
            const bool bHasTag = PerAsset[RowIndex].Tags.Contains(Key);
            if (bAdHoc && bHasTag)
            {
                // Create transaction before we make changes
                if (!Tx.IsValid())
                {
                    Tx = MakeUnique<FScopedTransaction>(
                        FText::Format(NSLOCTEXT("MetaWeaver", "BulkRemoveFmt", "Remove '{0}' from selection"),
                                      FText::FromName(Key)));
                }
                if (FMetaWeaverMetadataStore::RemoveMetadataTag(Asset, Key))
                {
                    UpdateAssetMetaDataState(Asset, RowIndex, Key);
                }
            }
        }
    }
}

TArray<int32> SMetaWeaverBulkEditor::GetRowIndexesForAsset(const UObject* Object)
{
    TArray<int32> AffectedRows;
    if (Object)
    {
        for (int32 i = 0; i < SelectedAssets.Num(); ++i)
        {
            if (SelectedAssets[i].GetAsset() == Object)
            {
                AffectedRows.AddUnique(i);
            }
        }
    }
    return AffectedRows;
}

void SMetaWeaverBulkEditor::OnObjectModified(UObject* Object)
{
    if (Object)
    {
        // Build list of affected row indices: direct asset hits, and assets in the same package
        TArray<int32> AffectedRows = GetRowIndexesForAsset(Object);
        const auto Pkg = Cast<UPackage>(Object);
        const auto ModifiedPackageName = Pkg ? Pkg->GetFName()
            : Object->GetOutermost()         ? Object->GetOutermost()->GetFName()
                                             : NAME_None;
        if (NAME_None != ModifiedPackageName)
        {
            for (int32 i = 0; i < SelectedAssets.Num(); ++i)
            {
                if (SelectedAssets[i].PackageName == ModifiedPackageName)
                {
                    AffectedRows.AddUnique(i);
                }
            }
        }
        if (0 == AffectedRows.Num())
        {
            return;
        }

        bool bAnyKeyChange = false;
        for (const int32 RowIndex : AffectedRows)
        {
            if (const auto Asset = SelectedAssets[RowIndex].GetAsset())
            {
                // Detect key-set changes
                TArray<FName> OldKeys;
                PerAsset[RowIndex].Tags.GenerateKeyArray(OldKeys);
                TArray<FName> NewKeys;
                {
                    TMap<FName, FString> LatestTags;
                    FMetaWeaverMetadataStore::ListMetadataTags(Asset, LatestTags);
                    LatestTags.GenerateKeyArray(NewKeys);
                }
                const TSet OldSet(OldKeys);
                const TSet NewSet(NewKeys);
                const bool bKeysDifferForRow = OldSet.Num() != NewSet.Num() || !OldSet.Difference(NewSet).IsEmpty()
                    || !NewSet.Difference(OldSet).IsEmpty();

                bAnyKeyChange |= bKeysDifferForRow;
                SyncAssetMetaDataState(RowIndex);
            }
        }

        if (bAnyKeyChange)
        {
            RecomputeCandidateColumnsAndPerAsset();
            RebuildCandidateColumnListView();
            RebuildMatrix();
        }

        RefreshListView();
    }
}

void SMetaWeaverBulkEditor::OnAssetRegistryAssetRemoved(const FAssetData& RemovedAsset)
{
    const auto RemovedPath = RemovedAsset.ToSoftObjectPath().ToString();
    UE_LOGFMT(LogMetaWeaver, Verbose, "OnAssetRegistryAssetRemoved {RemovedPath}", RemovedPath);
    int32 FoundIndex{ INDEX_NONE };
    for (int32 i = 0; i < SelectedAssets.Num(); ++i)
    {
        if (SelectedAssets[i].ToSoftObjectPath().ToString() == RemovedPath)
        {
            FoundIndex = i;
            break;
        }
    }
    if (INDEX_NONE != FoundIndex)
    {
        SelectedAssets.RemoveAt(FoundIndex);
        if (PerAsset.IsValidIndex(FoundIndex))
        {
            PerAsset.RemoveAt(FoundIndex);
        }
        if (AssetItems.IsValidIndex(FoundIndex))
        {
            AssetItems.RemoveAt(FoundIndex);
        }
        ClearAllErrors();
        RecomputeCandidateColumnsAndPerAsset();
        RebuildCandidateColumnListView();
        RebuildMatrix();
    }
}

void SMetaWeaverBulkEditor::RefreshListView() const
{
    if (ListView.IsValid())
    {
        ListView->RebuildList();
        ListView->RequestListRefresh();
    }
    else
    {
        UE_LOGFMT(LogMetaWeaver, Warning, "ListView invalid. Unable to RebuildList()");
    }
}

void SMetaWeaverBulkEditor::OnAssetRegistryAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
    UE_LOGFMT(LogMetaWeaver, Verbose, "OnAssetRegistryAssetRenamed {OldObjectPath}", OldObjectPath);
    for (int32 i = 0; i < SelectedAssets.Num(); ++i)
    {
        if (SelectedAssets[i].ToSoftObjectPath().ToString() == OldObjectPath)
        {
            SelectedAssets[i] = AssetData;
            SyncAssetMetaDataState(i);
            return;
        }
    }
}

void SMetaWeaverBulkEditor::OnContentBrowserAssetSelectionChanged(const TArray<FAssetData>& NewSelectedAssets,
                                                                  const bool bIsPrimaryBrowser)
{
    if (bIsPrimaryBrowser && !bLockToSelection)
    {
        // If selection is identical, exit early
        if (SelectedAssets.Num() == NewSelectedAssets.Num())
        {
            bool bSame = true;
            for (int32 i = 0; i < SelectedAssets.Num(); ++i)
            {
                if (SelectedAssets[i] != NewSelectedAssets[i])
                {
                    bSame = false;
                    break;
                }
            }
            if (bSame)
            {
                return;
            }
        }

        SelectedAssets = NewSelectedAssets;
        ClearAllErrors();
        RecomputeCandidateColumnsAndPerAsset();
        RebuildCandidateColumnListView();
        AssetItems.Reset();
        AssetItems.Reserve(SelectedAssets.Num());
        for (const auto& A : SelectedAssets)
        {
            AssetItems.Add(MakeShared<FAssetData>(A));
        }
        RefreshListView();
    }
}

void SMetaWeaverBulkEditor::UpdatePerAssetData(const int32 RowIndex)
{
    check(SelectedAssets.IsValidIndex(RowIndex));
    check(PerAsset.IsValidIndex(RowIndex));

    if (const auto Asset = SelectedAssets[RowIndex].GetAsset())
    {
        TArray<FMetadataParameterSpec> Specs;
        FMetaWeaverMetadataStore::GatherSpecsForClass(Asset->GetClass(), Specs);
        for (const auto& Spec : Specs)
        {
            PerAsset[RowIndex].Specs.Add(Spec.Key, Spec);
        }
        FMetaWeaverMetadataStore::ListMetadataTags(Asset, PerAsset[RowIndex].Tags);
    }
}

void SMetaWeaverBulkEditor::UpdateAssetItemAtIndex(const int32 RowIndex)
{
    if (AssetItems.IsValidIndex(RowIndex) && ListView->GetItems().IsValidIndex(RowIndex))
    {
        const auto OldItem = ListView->GetItems().GetData()[RowIndex];

        if (const auto Row = ListView->WidgetFromItem(OldItem))
        {
            Row->AsWidget()->Invalidate(EInvalidateWidget::PaintAndVolatility | EInvalidateWidget::Visibility
                                        | EInvalidateWidget::Layout);
        }
        AssetItems[RowIndex] = MakeShared<FAssetData>(SelectedAssets[RowIndex]);
        UpdatePerAssetData(RowIndex);
        RefreshListView();
    }
}

void SMetaWeaverBulkEditor::OnAssetRegistryAssetUpdated(const FAssetData& UpdatedAsset)
{
    const FString UpdatedPath = UpdatedAsset.ToSoftObjectPath().ToString();
    UE_LOGFMT(LogMetaWeaver, Verbose, "OnAssetRegistryAssetUpdated {UpdatedPath}", UpdatedPath);

    for (int32 i = 0; i < SelectedAssets.Num(); ++i)
    {
        if (SelectedAssets[i].ToSoftObjectPath().ToString() == UpdatedPath)
        {
            SyncAssetMetaDataState(i);
        }
    }
}

bool SMetaWeaverBulkEditor::GetCellError(const int32 RowIndex, const FName Key, FText& OutMessage) const
{
    if (const auto RowMap = CellErrors.Find(RowIndex))
    {
        if (const auto Message = RowMap->Find(Key))
        {
            OutMessage = *Message;
            return true;
        }
    }
    return false;
}

void SMetaWeaverBulkEditor::SetCellError(const int32 RowIndex, const FName Key, const FText& Message)
{
    CellErrors.FindOrAdd(RowIndex).Add(Key, Message);
    RefreshListView();
}

void SMetaWeaverBulkEditor::ClearCellError(const int32 RowIndex, const FName Key)
{
    if (const auto RowMap = CellErrors.Find(RowIndex))
    {
        RowMap->Remove(Key);
        if (0 == RowMap->Num())
        {
            CellErrors.Remove(RowIndex);
        }
    }
}

void SMetaWeaverBulkEditor::ClearAllErrorsForRow(const int32 RowIndex)
{
    CellErrors.Remove(RowIndex);
}

void SMetaWeaverBulkEditor::ClearAllErrors()
{
    CellErrors.Reset();
}

bool SMetaWeaverBulkEditor::ValidateMetaDataValue(const UObject* Asset,
                                                  const int32 RowIndex,
                                                  const FName Key,
                                                  const FString& Value)
{
    if (Asset && GEditor)
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            const auto Report = Subsystem->ValidateKeyValue(Asset->GetClass(), Key, Value);
            if (Report.bHasErrors)
            {
                const auto Message =
                    Report.Issues.Num() > 0 ? Report.Issues[0].Message : FText::FromString(TEXT("Invalid value"));
                SetCellError(RowIndex, Key, Message);
                return false;
            }
        }
    }
    return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SMetaWeaverBulkEditor::ShowInContentBrowser(const FAssetData& Asset) const
{
    if (const auto Object = Asset.GetAsset())
    {
        const auto& Module = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
        Module.Get().SyncBrowserToAssets(TArray{ Object });
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SMetaWeaverBulkEditor::OpenAssetEditor(const FAssetData& Asset) const
{
    if (GEditor)
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            if (const auto Object = Asset.GetAsset())
            {
                Subsystem->OpenEditorForAsset(Object);
            }
        }
    }
}
