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
#include "SMetaWeaverEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "IContentBrowserSingleton.h"
#include "MetaWeaver/MetaWeaverMetadataDefinitionSet.h"
#include "MetaWeaver/MetaWeaverMetadataStore.h"
#include "MetaWeaver/Validation/MetaWeaverValidationSubsystem.h"
#include "MetaWeaver/Validation/MetaWeaverValidationTypes.h"
#include "MetaWeaverEditorSettings.h"
#include "MetaWeaverStyle.h"
#include "MetaWeaverUIHelpers.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

class SMetaWeaverRow final : public SMultiColumnTableRow<TSharedPtr<FTagItem>>
{
public:
    SLATE_BEGIN_ARGS(SMetaWeaverRow) {}
    SLATE_ARGUMENT(TSharedPtr<FTagItem>, Item)
    SLATE_ARGUMENT(TWeakPtr<SMetaWeaverEditor>, Editor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
    {
        Item = InArgs._Item;
        Editor = InArgs._Editor;
        SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), InOwnerTableView);
    }

    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
    {
        if ("Status" == ColumnName)
        {
            return GenerateWidgetForStatusColumn();
        }
        else if ("Key" == ColumnName)
        {
            return GenerateWidgetForKeyColumn();
        }
        else if ("Value" == ColumnName)
        {
            return GenerateWidgetForValueColumn();
        }
        else if ("ResetAction" == ColumnName)
        {
            return GenerateWidgetForResetActionColumn();
        }
        else if ("DeleteAction" == ColumnName)
        {
            return GenerateWidgetForDeleteActionColumn();
        }
        else
        {
            return SNew(STextBlock).Text(FText::GetEmpty());
        }
    }

private:
    TSharedPtr<FTagItem> Item;
    TWeakPtr<SMetaWeaverEditor> Editor;

    TSharedRef<SWidget> GenerateWidgetForStatusColumn()
    {
        return SNew(SBox)
            .WidthOverride(16.f)
            .HeightOverride(16.f)
            .Padding(6.f, 2.f, 6.f, 2.f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)[SNew(SImage)
                                       .DesiredSizeOverride(FVector2D(16.f, 16.f))
                                       .Visibility_Lambda([Item = Item] {
                                           return Item.IsValid() && Item->Severity.IsSet() ? EVisibility::Visible
                                                                                           : EVisibility::Collapsed;
                                       })
                                       .Image_Lambda([Item = Item] {
                                           return !Item.IsValid() || !Item->Severity.IsSet()
                                               ? nullptr
                                               : FMetaWeaverStyle::GetBrushForIssueSeverity(Item->Severity.GetValue());
                                       })];
    }

    TSharedRef<SWidget> GenerateWidgetForKeyColumn()
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(
                VAlign_Center)[SNew(STextBlock).Text(FText::FromName(Item->Key)).ToolTipText_Lambda([Item = Item] {
                  return FText::FromString(Item.IsValid() ? Item->Spec.Description : FString());
              })]
            + SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f).VAlign(
                VAlign_Center)[SNew(STextBlock)
                                   .Visibility_Lambda([Item = Item]() {
                                       return Item.IsValid() && Item->IsUnsaved() ? EVisibility::Visible
                                                                                  : EVisibility::Collapsed;
                                   })
                                   .Text(FText::FromString(TEXT("*")))
                                   .ToolTipText(FText::FromString(
                                       TEXT("Default value exists but has not been saved to this asset.")))];
    }

    TSharedRef<SWidget> GenerateWidgetForValueColumn()
    {
        auto Pinned = Editor.Pin();
        check(Item.IsValid() && Pinned.IsValid());

        // Build the same value widget used previously
        TSharedRef<SWidget> ValueWidget = SNew(STextBlock).Text(FText::FromString(Item->Value));
        switch (Item->Spec.Type)
        {
            case EMetaWeaverValueType::Bool:
            {
                const ECheckBoxState State = Item->Value.Equals(TEXT("True"), ESearchCase::CaseSensitive)
                    ? ECheckBoxState::Checked
                    : ECheckBoxState::Unchecked;
                ValueWidget =
                    SNew(SCheckBox).IsChecked(State).OnCheckStateChanged_Lambda([Pinned, Item = Item](auto NewState) {
                        const auto NewVal = ECheckBoxState::Checked == NewState ? TEXT("True") : TEXT("False");
                        Pinned->OnEditMetadataTag(*Item, NewVal);
                    });
                break;
            }
            case EMetaWeaverValueType::Integer:
            {
                ValueWidget = SNew(SNumericEntryBox<int64>)
                                  .AllowSpin(true)
                                  .Value_Lambda([Item = Item] {
                                      int64 Parsed{ 0 };
                                      if (Item.IsValid())
                                      {
                                          LexTryParseString(Parsed, *Item->Value);
                                      }
                                      return Parsed;
                                  })
                                  .OnValueCommitted_Lambda([Pinned, Item = Item](const auto NewValue, auto) {
                                      Pinned->OnEditMetadataTag(*Item, LexToString(NewValue));
                                  });
                break;
            }
            case EMetaWeaverValueType::Float:
            {
                ValueWidget = SNew(SNumericEntryBox<double>)
                                  .AllowSpin(true)
                                  .Value_Lambda([Item = Item] {
                                      double Parsed{ 0.0 };
                                      if (Item.IsValid())
                                      {
                                          LexTryParseString(Parsed, *Item->Value);
                                      }
                                      return Parsed;
                                  })
                                  .OnValueCommitted_Lambda([Pinned, Item = Item](const auto NewValue, auto) {
                                      Pinned->OnEditMetadataTag(*Item, LexToString(NewValue));
                                  });
                break;
            }
            case EMetaWeaverValueType::Enum:
            {
                int32 InitialIndex = INDEX_NONE;
                const auto Trim = Item->Value.TrimStartAndEnd();
                for (auto i = 0; i < Item->EnumOptions.Num(); ++i)
                {
                    // ReSharper disable once CppTooWideScopeInitStatement
                    auto& Opt = Item->EnumOptions[i];
                    if (Opt.IsValid() && Trim.Equals(*Opt, ESearchCase::CaseSensitive))
                    {
                        InitialIndex = i;
                        break;
                    }
                }
                const auto CurrentSelection =
                    INDEX_NONE != InitialIndex ? Item->EnumOptions[InitialIndex] : TSharedPtr<FString>();
                ValueWidget =
                    SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&Item->EnumOptions)
                        .InitiallySelectedItem(CurrentSelection)
                        .OnGenerateWidget_Lambda([](const auto& InItem) {
                            return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : TEXT("")));
                        })
                        .OnSelectionChanged_Lambda([Pinned, Item = Item](const auto& NewItem, auto) {
                            if (NewItem.IsValid())
                            {
                                Pinned->OnEditMetadataTag(*Item, *NewItem);
                            }
                        })[SNew(STextBlock).Text_Lambda([Item = Item] {
                            return FText::FromString(Item.IsValid() ? Item->Value : FString());
                        })];
                break;
            }
            case EMetaWeaverValueType::AssetReference:
            {
                const auto Allowed = Item->Spec.AllowedClass ? Item->Spec.AllowedClass.Get() : UObject::StaticClass();
                ValueWidget = SNew(SObjectPropertyEntryBox)
                                  .AllowedClass(Allowed)
                                  .AllowClear(true)
                                  .DisplayUseSelected(true)
                                  .DisplayBrowse(true)
                                  .ObjectPath_Lambda([Item = Item] { return Item.IsValid() ? Item->Value : FString(); })
                                  .OnObjectChanged_Lambda([Pinned, Item = Item](const auto& NewAssetData) {
                                      const auto NewVal = NewAssetData.IsValid()
                                          ? NewAssetData.ToSoftObjectPath().ToString()
                                          : FString();
                                      Pinned->OnEditMetadataTag(*Item, NewVal);
                                  });
                break;
            }
            case EMetaWeaverValueType::String:
            default:
            {
                ValueWidget = SNew(SEditableTextBox)
                                  .Text(FText::FromString(Item->Value))
                                  .OnTextCommitted_Lambda([Pinned, Item = Item](const auto& NewText, auto) {
                                      Pinned->OnEditMetadataTag(*Item, NewText.ToString());
                                  });
                break;
            }
        }

        return SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[ValueWidget]
            + SVerticalBox::Slot().AutoHeight().Padding(
                FMargin(0.f, 2.f, 0.f, 0.f))[SNew(STextBlock)
                                                 .Visibility_Lambda([Item = Item] {
                                                     return Item.IsValid() && Item->Severity.IsSet()
                                                         ? EVisibility::Visible
                                                         : EVisibility::Collapsed;
                                                 })
                                                 .Text_Lambda([Item = Item] {
                                                     return FText::FromString(Item.IsValid() ? Item->ValidationMessage
                                                                                             : FString());
                                                 })
                                                 .ColorAndOpacity_Lambda([Item = Item] {
                                                     return !Item.IsValid() || !Item->Severity.IsSet()
                                                         ? FSlateColor::UseForeground()
                                                         : FMetaWeaverStyle::GetColorForIssueSeverity(
                                                               Item->Severity.GetValue());
                                                 })];
    }

    TSharedRef<SWidget> GenerateWidgetForResetActionColumn()
    {
        auto Pinned = Editor.Pin();
        check(Item.IsValid() && Pinned.IsValid());

        if (!Item->Spec.Key.IsNone())
        {
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                      .Padding(0.f, 2.f, 0.f, 2.f)
                      .AutoWidth()[SNew(SButton)
                                       .ToolTipText(FText::FromString(TEXT("Reset value to default")))
                                       .HAlign(HAlign_Center)
                                       .VAlign(VAlign_Center)
                                       .ContentPadding(0)
                                       .IsEnabled_Lambda([Item = Item] {
                                           const auto& DefaultVal = Item->Spec.DefaultValue;
                                           // Disable if current value exists and matches the default
                                           return DefaultVal.IsEmpty()
                                               ? Item->bHasTag
                                               : !Item->Value.Equals(DefaultVal, ESearchCase::CaseSensitive);
                                       })
                                       .OnClicked_Lambda([Pinned, Item = Item] {
                                           Pinned->OnResetMetadataTag(*Item);
                                           return FReply::Handled();
                                       })[SNew(SBox)
                                              .HAlign(HAlign_Center)
                                              .VAlign(VAlign_Center)
                                              .Padding(0.f, 2.f, 0.f, 2.f)[SNew(SImage).Image(
                                                  FMetaWeaverStyle::GetResetToDefaultBrush())]]];
        }
        else
        {
            return SNew(SHorizontalBox) + SHorizontalBox::Slot().Padding(0.f, 2.f, 0.f, 2.f).AutoWidth();
        }
    }

    TSharedRef<SWidget> GenerateWidgetForDeleteActionColumn()
    {
        auto Pinned = Editor.Pin();
        check(Item.IsValid() && Pinned.IsValid());

        // ReSharper disable once CppTooWideScopeInitStatement
        const bool bAdHocTag = Item->Spec.Key.IsNone();
        if (bAdHocTag || !Item->Spec.bRequired)
        {
            const auto Tooltip =
                bAdHocTag ? TEXT("Delete this metadata key") : TEXT("Remove this value from the asset");
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                      .Padding(0.f, 2.f, 0.f, 2.f)
                      .AutoWidth()
                          [SNew(SButton)
                               .ToolTipText(FText::FromString(Tooltip))
                               .HAlign(HAlign_Center)
                               .VAlign(VAlign_Center)
                               .ContentPadding(0)
                               .IsEnabled_Lambda([Item = Item, bAdHocTag] { return bAdHocTag || Item->bHasTag; })
                               .OnClicked_Lambda([Pinned, Item = Item] {
                                   Pinned->OnRemoveMetadataTag(*Item);
                                   return FReply::Handled();
                               })[SNew(SBox)
                                      .HAlign(HAlign_Center)
                                      .VAlign(VAlign_Center)
                                      .Padding(0.f, 4.f, 0.f, 4.f)[SNew(SImage)
                                                                       .DesiredSizeOverride(FVector2D(12.f, 12.f))
                                                                       .Image(FMetaWeaverStyle::GetDeleteBrush())]]];
        }
        else
        {
            return SNew(SHorizontalBox) + SHorizontalBox::Slot().Padding(0.f, 2.f, 0.f, 2.f).AutoWidth();
        }
    }
};

SMetaWeaverEditor::~SMetaWeaverEditor()
{
    if (ObjectModifiedHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectModified.Remove(ObjectModifiedHandle);
    }
    if (ContentBrowserSelectionHandle.IsValid())
    {
        if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
        {
            auto& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
            CBModule.GetOnAssetSelectionChanged().Remove(ContentBrowserSelectionHandle);
        }
    }
    if (AssetRemovedHandle.IsValid() || AssetRenamedHandle.IsValid())
    {
        if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
        {
            const auto& AssetRegistryModule =
                FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
            if (AssetRemovedHandle.IsValid())
            {
                AssetRegistryModule.Get().OnAssetRemoved().Remove(AssetRemovedHandle);
            }
            if (AssetRenamedHandle.IsValid())
            {
                AssetRegistryModule.Get().OnAssetRenamed().Remove(AssetRenamedHandle);
            }
        }
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

    // Persist preferences
    if (const auto Settings = GetMutableDefault<UMetaWeaverEditorSettings>())
    {
        Settings->bLockToSelectionDefault = bLockToSelection;
        Settings->SaveConfig();
    }
}

bool FTagItem::IsUnsaved() const
{
    const bool bDefined = !Spec.Key.IsNone();
    const bool bHasDefault = bDefined && !Spec.DefaultValue.IsEmpty();
    return bHasDefault && !bHasTag;
}

void SMetaWeaverEditor::Construct(const FArguments& InArgs)
{
    SelectedAssets = InArgs._SelectedAssets;

    // Load user preferences
    if (const auto Settings = GetMutableDefault<UMetaWeaverEditorSettings>())
    {
        bLockToSelection = Settings->bLockToSelectionDefault;
    }

    ChildSlot
        [SNew(SVerticalBox)

         + SVerticalBox::Slot().AutoHeight().Padding(8.f)[BuildTopBar()]

         + SVerticalBox::Slot().AutoHeight().Padding(8.f)[SNew(SBox).Visibility_Lambda([this] {
               return ControlsVisibility();
           })[SNew(SHorizontalBox)
              + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(0.f, 0.f, 8.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("New:")))]
              + SHorizontalBox::Slot().FillWidth(
                  0.35f)[SAssignNew(NewKeyText, SEditableTextBox)
                             .HintText(FText::FromString(TEXT("New key")))
                             .OnTextChanged_Lambda([this](const auto&) { RefreshListView(); })]
              + SHorizontalBox::Slot().FillWidth(0.55f).Padding(8.f, 0.f)
                    [SAssignNew(NewValueText, SEditableTextBox).HintText(FText::FromString(TEXT("New value")))]
              + SHorizontalBox::Slot().AutoWidth()[SNew(SButton)
                                                       .Text(FText::FromString(TEXT("Add")))
                                                       .IsEnabled_Lambda([this] { return IsAddEnabled(); })
                                                       .OnClicked_Lambda([this] {
                                                           OnAddMetadataTag();
                                                           return FReply::Handled();
                                                       })]]]

         // Inline error for Add row
         + SVerticalBox::Slot().AutoHeight().Padding(8.f)[SNew(SBox).Visibility_Lambda([this] {
               return ControlsVisibility();
           })[SNew(STextBlock)
                  .Visibility_Lambda([this] {
                      const auto Text = GetAddErrorText();
                      return Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
                  })
                  .ColorAndOpacity(FMetaWeaverStyle::GetErrorColor())
                  .Text_Lambda([this] { return GetAddErrorText(); })]]

         + SVerticalBox::Slot().AutoHeight().Padding(8.f)[SNew(SBox).Visibility_Lambda([this] {
               return ControlsVisibility();
           })[SNew(SHorizontalBox)
              + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(0.f, 0.f, 8.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Filter:")))]
              + SHorizontalBox::Slot().FillWidth(
                  1.f)[SAssignNew(SearchBox, SSearchBox).OnTextChanged(this, &SMetaWeaverEditor::OnFilterChanged)]]]

         + SVerticalBox::Slot().FillHeight(1.f).Padding(8.f)[SNew(SBox).Visibility_Lambda([this] {
               return ControlsVisibility();
           })[SAssignNew(TagListView, SListView<TSharedPtr<FTagItem>>)
                  .ListItemsSource(&FilteredTagItems)
                  .OnGenerateRow(this, &SMetaWeaverEditor::OnGenerateRow)
                  .SelectionMode(ESelectionMode::None)
                  .HeaderRow(
                      SNew(SHeaderRow) + SHeaderRow::Column("Status").FixedWidth(28.f).DefaultLabel(FText::GetEmpty())
                      + SHeaderRow::Column("Key").DefaultLabel(FText::FromString(TEXT("Key"))).FillWidth(0.33f)
                      + SHeaderRow::Column("Value").DefaultLabel(FText::FromString(TEXT("Value"))).FillWidth(0.52f)
                      + SHeaderRow::Column("ResetAction").DefaultLabel(FText::GetEmpty()).FixedWidth(45.f)
                      + SHeaderRow::Column("DeleteAction").DefaultLabel(FText::GetEmpty()).FixedWidth(45.f))]

    ]];

    RebuildTagListItems();
    RefreshListView();

    // External change notifications
    ObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddSP(this, &SMetaWeaverEditor::OnObjectModified);

    // Content Browser selection change (primary browser only)
    {
        auto& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
        ContentBrowserSelectionHandle =
            CBModule.GetOnAssetSelectionChanged().AddSP(this,
                                                        &SMetaWeaverEditor::OnContentBrowserAssetSelectionChanged);
    }

    // AssetRegistry remove/rename handling
    {
        const auto& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        AssetRemovedHandle =
            AssetRegistryModule.Get().OnAssetRemoved().AddSP(this, &SMetaWeaverEditor::OnAssetRegistryAssetRemoved);
        AssetRenamedHandle =
            AssetRegistryModule.Get().OnAssetRenamed().AddSP(this, &SMetaWeaverEditor::OnAssetRegistryAssetRenamed);
    }

    // Definition set changes
    if (GEditor)
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
        {
            DefinitionSetsChangedHandle =
                Subsystem->GetOnDefinitionSetsChanged().AddSP(this, &SMetaWeaverEditor::OnDefinitionSetsChanged);
        }
    }
}

UObject* SMetaWeaverEditor::ResolveFirstAsset() const
{
    return 0 == SelectedAssets.Num() ? nullptr : SelectedAssets[0].GetAsset();
}

void SMetaWeaverEditor::OnAddMetadataTag()
{
    if (IsAddEnabled() && NewKeyText.IsValid())
    {
        if (const auto Asset = ResolveFirstAsset())
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            const auto KeyStr = NewKeyText->GetText().ToString().TrimStartAndEnd();
            if (!KeyStr.IsEmpty())
            {
                FScopedTransaction Tx(NSLOCTEXT("MetaWeaver", "AddTagTransaction", "Add Metadata Tag"));
                // ReSharper disable once CppTooWideScopeInitStatement
                if (FMetaWeaverMetadataStore::SetMetadataTag(Asset, FName(*KeyStr), NewValueText->GetText().ToString()))
                {
                    MarkAssetDirty(Asset);
                    RebuildTagListItems();
                    SaveAnyUnsavedDefaults();
                    ClearAddFields();
                    RefreshListView();
                }
            }
        }
    }
}

void SMetaWeaverEditor::ApplyFilter()
{
    FilteredTagItems.Reset();
    if (CurrentFilter.IsEmpty())
    {
        FilteredTagItems = TagItems;
    }
    else
    {
        const auto FilterLower{ CurrentFilter.ToLower() };
        for (const auto& Item : TagItems)
        {
            if (Item.IsValid())
            {
                // ReSharper disable once CppTooWideScopeInitStatement
                const auto KeyLower = Item->Key.ToString().ToLower();
                if (KeyLower.Contains(FilterLower))
                {
                    FilteredTagItems.Add(Item);
                }
            }
        }
    }
}

void SMetaWeaverEditor::OnFilterChanged(const FText& NewText)
{
    CurrentFilter = NewText.ToString();
    ApplyFilter();
    RefreshListView();
}

void SMetaWeaverEditor::ShowSelectedInContentBrowser()
{
    if (SelectedAssets.Num() >= 1)
    {
        if (const auto Asset = SelectedAssets[0].GetAsset())
        {
            const auto& Module = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
            Module.Get().SyncBrowserToAssets(TArray{ Asset });
        }
    }
}

void SMetaWeaverEditor::OpenAssetEditor()
{
    if (GEditor && SelectedAssets.Num() >= 1)
    {
        if (const auto AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            if (const auto Asset = SelectedAssets[0].GetAsset())
            {
                AES->OpenEditorForAsset(Asset);
            }
        }
    }
}

void SMetaWeaverEditor::OnResetMetadataTag(FTagItem& Item)
{
    if (const auto Asset = GetFirstAssetForUI())
    {
        const auto& DefaultVal = Item.Spec.DefaultValue;
        const bool bRemove = DefaultVal.IsEmpty();
        FScopedTransaction Transaction((NSLOCTEXT("MetaWeaver", "ResetTagTransaction", "Reset Metadata Tag")));
        const bool bOk = bRemove ? FMetaWeaverMetadataStore::RemoveMetadataTag(Asset, Item.Key)
                                 : FMetaWeaverMetadataStore::SetMetadataTag(Asset, Item.Key, DefaultVal);
        if (bOk)
        {
            MarkAssetDirty(Asset);
            Item.Value = DefaultVal;
            Item.bHasTag = !DefaultVal.IsEmpty();
            RebuildTagListUI();
            RefreshListUI();
            SaveAnyUnsavedDefaults(Item.Key);
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SMetaWeaverEditor::MarkAssetDirty(const UObject* Asset)
{
    if (Asset)
    {
        if (const auto Package = Asset->GetOutermost())
        {
            Package->SetDirtyFlag(true);
        }
    }
}

void SMetaWeaverEditor::SaveAnyUnsavedDefaults(TOptional<FName> ExcludeKey)
{
    if (const auto Asset = ResolveFirstAsset())
    {
        TMap<FName, FString> Pending;
        for (const auto& It : TagItems)
        {
            if (It.IsValid() && It->IsUnsaved() && (!ExcludeKey.IsSet() || It->Key != ExcludeKey.GetValue()))
            {
                Pending.Add(It->Key, It->Spec.DefaultValue);
            }
        }
        if (Pending.Num() > 0)
        {
            for (const auto& Pair : Pending)
            {
                FMetaWeaverMetadataStore::SetMetadataTag(Asset, Pair.Key, Pair.Value);
            }
            MarkAssetDirty(Asset);
            for (const auto& It : TagItems)
            {
                if (It.IsValid() && Pending.Contains(It->Key))
                {
                    It->bHasTag = true;
                    It->Value = It->Spec.DefaultValue;
                }
            }
            RevalidateUI();
            RefreshListUI();
        }
    }
}

void SMetaWeaverEditor::OnEditMetadataTag(FTagItem& Item, const FString& NewValue)
{
    if (const auto Asset = GetFirstAssetForUI())
    {
        // Pre-commit validation for defined keys
        if (GEditor)
        {
            if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
            {
                // ReSharper disable once CppTooWideScopeInitStatement
                const auto Report = Subsystem->ValidateKeyValue(Asset->GetClass(), Item.Key, NewValue);
                if (Report.bHasErrors)
                {
                    // Surface the first error inline on this row and abort the write
                    FString Message;
                    for (const auto& Issue : Report.Issues)
                    {
                        if (Issue.Key == Item.Key && Issue.Severity == EMetaWeaverIssueSeverity::Error)
                        {
                            Message = Issue.Message.ToString();
                            break;
                        }
                    }
                    if (Message.IsEmpty() && Report.Issues.Num() > 0)
                    {
                        Message = Report.Issues[0].Message.ToString();
                    }
                    Item.Severity = EMetaWeaverIssueSeverity::Error;
                    Item.ValidationMessage = MoveTemp(Message);
                    RefreshListUI();
                    return;
                }
            }
        }

        const auto NewVal = LexToString(NewValue);
        FScopedTransaction Tx(NSLOCTEXT("MetaWeaver", "EditTagTransaction", "Edit Metadata Tag"));
        if (FMetaWeaverMetadataStore::SetMetadataTag(Asset, Item.Key, NewVal))
        {
            MarkAssetDirty(Asset);
            Item.Value = NewVal;
            Item.bHasTag = true;
            RevalidateUI();
            RefreshListUI();
            SaveAnyUnsavedDefaults(Item.Key);
        }
    }
}

void SMetaWeaverEditor::OnRemoveMetadataTag(const FTagItem& Item)
{
    if (const auto Asset = GetFirstAssetForUI())
    {
        FScopedTransaction Tx(NSLOCTEXT("MetaWeaver", "DeleteTagTransaction", "Delete Metadata Tag"));
        if (FMetaWeaverMetadataStore::RemoveMetadataTag(Asset, Item.Key))
        {
            MarkAssetDirty(Asset);
            RebuildTagListUI();
            RefreshListUI();
            SaveAnyUnsavedDefaults(Item.Key);
        }
    }
}

const FSlateBrush* SMetaWeaverEditor::GetSelectedAssetBrush()
{
    if (SelectedAssets.Num() >= 1)
    {
        if (const auto Asset = SelectedAssets[0].GetAsset())
        {
            return FMetaWeaverStyle::GetBrushForAsset(Asset);
        }
    }
    return FMetaWeaverStyle::GetDocumentBrush();
}

FText SMetaWeaverEditor::GetSelectedAssetToolTip()
{
    if (SelectedAssets.Num() >= 1)
    {
        if (const auto Asset = SelectedAssets[0].GetAsset())
        {
            return Asset->GetClass()->GetDisplayNameText();
        }
    }
    return FText();
}

void SMetaWeaverEditor::Tick(const FGeometry&, const double InCurrentTime, const float)
{
    // External refresh
    if (bPendingExternalRefresh && NextExternalRefreshTime > 0.0 && InCurrentTime >= NextExternalRefreshTime)
    {
        bPendingExternalRefresh = false;
        NextExternalRefreshTime = -1.0;
        RebuildTagListItems();
        ClearAddFields();
        RefreshListView();
    }
}

void SMetaWeaverEditor::OnContentBrowserAssetSelectionChanged(const TArray<FAssetData>& NewSelectedAssets,
                                                              const bool bIsPrimaryBrowser)
{
    if (!bIsPrimaryBrowser || bLockToSelection)
    {
        return;
    }

    const int32 AssetCount = NewSelectedAssets.Num();
    const auto NewState = 0 == AssetCount ? ESelectionViewState::None
        : 1 == AssetCount                 ? ESelectionViewState::Single
                                          : ESelectionViewState::Multiple;

    const bool bStateChanged = NewState != CurrentViewState;
    bool bAssetChanged = true;
    if (ESelectionViewState::Single == NewState && ESelectionViewState::Single == CurrentViewState)
    {
        if (1 == SelectedAssets.Num() && 1 == AssetCount)
        {
            bAssetChanged = SelectedAssets[0] != NewSelectedAssets[0];
        }
    }

    if (bStateChanged || bAssetChanged)
    {
        CurrentViewState = NewState;
        switch (NewState)
        {
            case ESelectionViewState::None:
            {
                SelectedAssets.Reset();
                TagItems.Reset();
                FilteredTagItems.Reset();
                break;
            }
            case ESelectionViewState::Multiple:
            {
                SelectedAssets = NewSelectedAssets;
                TagItems.Reset();
                FilteredTagItems.Reset();
                break;
            }
            case ESelectionViewState::Single:
            default:
            {
                SelectedAssets = NewSelectedAssets;
                RebuildTagListItems();
                break;
            }
        }
        ClearAddFields();
        RefreshListView();
    }
}

void SMetaWeaverEditor::OnAssetRegistryAssetRemoved(const FAssetData& RemovedAsset)
{
    if (SelectedAssets.Num() > 0)
    {
        const auto RemovedPath = RemovedAsset.ToSoftObjectPath().ToString();
        // ReSharper disable once CppTooWideScopeInitStatement
        const auto CurrentPath = SelectedAssets[0].ToSoftObjectPath().ToString();
        if (RemovedPath == CurrentPath)
        {
            CurrentViewState = ESelectionViewState::None;
            SelectedAssets.Reset();
            TagItems.Reset();
            FilteredTagItems.Reset();
            ClearAddFields();
            RefreshListView();
        }
    }
}

void SMetaWeaverEditor::OnAssetRegistryAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
    if (SelectedAssets.Num() > 0)
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const auto CurrentPath = SelectedAssets[0].ToSoftObjectPath().ToString();
        if (CurrentPath == OldObjectPath)
        {
            SelectedAssets[0] = AssetData;
            RefreshListView();
        }
    }
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void SMetaWeaverEditor::OnObjectModified(UObject* Object)
{
    if (Object)
    {
        if (SelectedAssets.Num() > 0)
        {
            if (const auto Shown = SelectedAssets[0].GetAsset(); Shown == Object)
            {
                bPendingExternalRefresh = true;
                NextExternalRefreshTime = FPlatformTime::Seconds() + 0.15;
            }
        }
    }
}

FText SMetaWeaverEditor::BuildSelectionSummaryText() const
{
    switch (CurrentViewState)
    {
        case ESelectionViewState::None:
            return FText::FromString(TEXT("No asset selected."));
        case ESelectionViewState::Multiple:
            return FText::FromString(TEXT("Multiple assets selected — open the Bulk Editor to edit many at once."));
        case ESelectionViewState::Single:
        default:
            return FText::FromString(FString::Printf(TEXT("1 selected asset — editing.")));
    }
}

FText SMetaWeaverEditor::BuildSelectionMessageText() const
{
    switch (CurrentViewState)
    {
        case ESelectionViewState::None:
            return FText::FromString(TEXT("No asset selected."));
        case ESelectionViewState::Multiple:
            return FText::FromString(TEXT("Multiple assets selected. Open the bulk editor to edit many at once."));
        case ESelectionViewState::Single:
        default:
            return FText();
    }
}

TSharedRef<SWidget> SMetaWeaverEditor::BuildTopBar()
{
    return SNew(SBorder)
        .Padding(FMargin(8.f, 6.f))
        .BorderImage(FMetaWeaverStyle::GetBorderImageBrush())
            [SNew(SWidgetSwitcher).WidgetIndex_Lambda([this] {
                return CurrentViewState == ESelectionViewState::Single && SelectedAssets.Num() >= 1 ? 0 : 1;
            })
             // Slot 0: Single asset header (icon + name + actions)
             + SWidgetSwitcher::Slot()
                 [SNew(SHorizontalBox)
                  // Class icon
                  + SHorizontalBox::Slot().AutoWidth().VAlign(
                      VAlign_Center)[SNew(SImage)
                                         .Image_Lambda([this] { return GetSelectedAssetBrush(); })
                                         .ToolTipText_Lambda([this] { return GetSelectedAssetToolTip(); })]
                  // Asset name
                  + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f).VAlign(
                      VAlign_Center)[SNew(STextBlock).Text_Lambda([this] {
                        return SelectedAssets.Num() >= 1 ? FText::FromName(SelectedAssets[0].AssetName)
                                                         : FText::GetEmpty();
                    })]
                  + SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
                  // Show in Content Browser
                  + SHorizontalBox::Slot().AutoWidth().VAlign(
                      VAlign_Center)[SNew(SButton)
                                         .ToolTipText(FText::FromString(TEXT("Show in Content Browser")))
                                         .ButtonStyle(&FMetaWeaverStyle::GetButtonStyle())
                                         .OnClicked_Lambda([this] {
                                             ShowSelectedInContentBrowser();
                                             return FReply::Handled();
                                         })[SNew(SImage).Image(FMetaWeaverStyle::GetSearchBrush())]]
                  // Open Asset Editor
                  + SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f).VAlign(
                      VAlign_Center)[SNew(SButton)
                                         .ToolTipText(FText::FromString(TEXT("Open Asset Editor")))
                                         .ButtonStyle(&FMetaWeaverStyle::GetButtonStyle())
                                         .OnClicked_Lambda([this] {
                                             OpenAssetEditor();
                                             return FReply::Handled();
                                         })[SNew(SImage).Image(FMetaWeaverStyle::GetEditBrush())]]
                  // Lock toggle
                  + SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f).VAlign(
                      VAlign_Center)[SNew(SButton)
                                         .ToolTipText(FText::FromString(TEXT("Lock to selection")))
                                         .ButtonStyle(&FMetaWeaverStyle::GetButtonStyle())
                                         .OnClicked_Lambda([this] {
                                             bLockToSelection = !bLockToSelection;
                                             return FReply::Handled();
                                         })[SNew(SImage).Image_Lambda([this] {
                                             return FMetaWeaverStyle::GetLockBrush(bLockToSelection);
                                         })]]]
             // Slot 1: Summary text for none/multiple selection
             + SWidgetSwitcher::Slot()[SNew(STextBlock).Text_Lambda([this] { return BuildSelectionSummaryText(); })]];
}

EVisibility SMetaWeaverEditor::ControlsVisibility() const
{
    return CurrentViewState == ESelectionViewState::Single ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SMetaWeaverEditor::IsAddEnabled() const
{
    if (!NewKeyText.IsValid())
    {
        return false;
    }
    else
    {
        const auto KeyStr = NewKeyText->GetText().ToString();
        const auto Trimmed = KeyStr.TrimStartAndEnd();
        if (Trimmed.IsEmpty())
        {
            return false;
        }
        else if (!KeyStr.Equals(Trimmed, ESearchCase::CaseSensitive))
        {
            return false;
        }
        else
        {
            return !DefinedKeys.Contains(FName(*Trimmed));
        }
    }
}

FText SMetaWeaverEditor::GetAddErrorText() const
{
    if (!NewKeyText.IsValid())
    {
        return FText();
    }
    else
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const auto KeyStr = NewKeyText->GetText().ToString();
        if (KeyStr.IsEmpty())
        {
            return FText();
        }
        else
        {
            const auto Trimmed = KeyStr.TrimStartAndEnd();
            if (!KeyStr.Equals(Trimmed, ESearchCase::CaseSensitive))
            {
                return FText::FromString(TEXT("Key must not start or end with whitespace."));
            }
            if (DefinedKeys.Contains(FName(*Trimmed)))
            {
                return FText::FromString(TEXT("Key is already defined in the metadata."));
            }
            return FText();
        }
    }
}

void SMetaWeaverEditor::RebuildTagListItems()
{
    TagItems.Reset();
    DefinedKeys.Reset();
    if (const auto Asset = ResolveFirstAsset())
    {
        // Effective specs for this asset's class
        TArray<FMetadataParameterSpec> Specs;
        FMetaWeaverMetadataStore::GatherSpecsForClass(Asset->GetClass(), Specs);

        // Current tags on asset
        TMap<FName, FString> Tags;
        FMetaWeaverMetadataStore::ListMetadataTags(Asset, Tags);
        // Treat all existing tag keys as reserved for Add-row duplicate prevention
        for (const auto& Pair : Tags)
        {
            DefinedKeys.Add(Pair.Key);
        }

        // Rows from specs (defined keys)
        for (const auto& Spec : Specs)
        {
            auto Item = MakeShared<FTagItem>();
            Item->Key = Spec.Key;
            Item->Spec = Spec;
            if (!Spec.Key.IsNone())
            {
                DefinedKeys.Add(Spec.Key);
            }
            if (const auto Found = Tags.Find(Spec.Key))
            {
                Item->Value = *Found;
                Item->bHasTag = true;
            }
            Item->EnumOptions.Reset();
            if (EMetaWeaverValueType::Enum == Spec.Type && Spec.EnumValues.Num() > 0)
            {
                MetaWeaver::UIHelpers::BuildEnumOptions(Spec.EnumValues, Item->EnumOptions, /*bSort*/ true);
            }
            TagItems.Add(Item);
        }

        // Add any tags not covered by definitions
        for (const auto& Pair : Tags)
        {
            const auto& K = Pair.Key;
            if (!Specs.ContainsByPredicate([&](const auto& S) { return S.Key == K; }))
            {
                auto Item = MakeShared<FTagItem>();
                Item->Key = K;
                Item->Value = Pair.Value;
                Item->bHasTag = true;
                TagItems.Add(Item);
            }
        }
    }
    FilteredTagItems = TagItems;
    RefreshValidation();
}

void SMetaWeaverEditor::ClearAddFields() const
{
    if (NewKeyText.IsValid())
    {
        NewKeyText->SetText(FText());
    }
    if (NewValueText.IsValid())
    {
        NewValueText->SetText(FText());
    }
}

void SMetaWeaverEditor::RefreshListView() const
{
    if (TagListView.IsValid())
    {
        TagListView->RequestListRefresh();
    }
}

void SMetaWeaverEditor::RefreshValidation()
{
    ValidationErrorCount = 0;
    ValidationWarningCount = 0;
    for (const auto& Item : TagItems)
    {
        if (Item.IsValid())
        {
            Item->Severity.Reset();
            Item->ValidationMessage.Reset();
        }
    }

    if (const auto Asset = ResolveFirstAsset())
    {
        if (const auto Subsystem = GEditor ? GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>() : nullptr)
        {
            auto Report = Subsystem->ValidateAsset(Asset);
            for (const FMetaWeaverIssue& Issue : Report.Issues)
            {
                for (const auto& Item : TagItems)
                {
                    if (Item.IsValid() && Item->Key == Issue.Key)
                    {
                        if (!Item->Severity.IsSet() || Issue.Severity == EMetaWeaverIssueSeverity::Error
                            || (Issue.Severity == EMetaWeaverIssueSeverity::Warning
                                && Item->Severity.GetValue() == EMetaWeaverIssueSeverity::Info))
                        {
                            Item->Severity = Issue.Severity;
                            Item->ValidationMessage = Issue.Message.ToString();
                        }
                        break;
                    }
                }
                if (EMetaWeaverIssueSeverity::Error == Issue.Severity)
                {
                    ++ValidationErrorCount;
                }
                else if (EMetaWeaverIssueSeverity::Warning == Issue.Severity)
                {
                    ++ValidationWarningCount;
                }
            }
        }
    }
}

void SMetaWeaverEditor::OnDefinitionSetsChanged()
{
    // Definition changes affect specs and default values; rebuild and refresh
    RebuildTagListItems();
    RefreshListView();
}

// ReSharper disable once CppPassValueParameterByConstReference
TSharedRef<ITableRow> SMetaWeaverEditor::OnGenerateRow(TSharedPtr<FTagItem> Item,
                                                       const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SMetaWeaverRow, OwnerTable).Item(Item).Editor(SharedThis(this));
}
