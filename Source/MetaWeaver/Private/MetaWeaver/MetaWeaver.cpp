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
#include "MetaWeaver.h"
#include "ContentBrowserMenuContexts.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Docking/TabManager.h"
#include "MetaWeaverCommands.h"
#include "MetaWeaverLogging.h"
#include "MetaWeaverStyle.h"
#include "SMetaWeaverBulkEditor.h"
#include "SMetaWeaverEditor.h"
#include "Styling/AppStyle.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace
{
    static const FName MetaWeaverMenuName("MetaWeaver");
    static const FName MetaWeaverTabName("MetaWeaverTab");
    static const FName MetaWeaverBulkTabName("MetaWeaverBulkTab");
} // namespace

#define LOCTEXT_NAMESPACE "FMetaWeaverModule"

void FMetaWeaverModule::StartupModule()
{
    UE_LOG(LogMetaWeaver, Log, TEXT("MetaWeaver module starting up"));

    FMetaWeaverStyle::Initialize();
    FMetaWeaverStyle::ReloadTextures();

    FMetaWeaverCommands::Register();

    PluginCommands = MakeShared<FUICommandList>();
    PluginCommands->MapAction(FMetaWeaverCommands::Get().OpenMetaWeaver,
                              FExecuteAction::CreateRaw(this, &FMetaWeaverModule::PluginButtonClicked));

    MetaWeaverMenuItem = WorkspaceMenu::GetMenuStructure().GetToolsCategory()->AddGroup(
        "MetaWeaverTabGroup",
        LOCTEXT("MetaWeaverTabTitle_Category", "MetaWeaver"),
        LOCTEXT("MetaWeaverTabTitle_CategoryMenuTooltipText", "MetaWeaver Operations."),
        FMetaWeaverStyle::GetMenuGroupIcon());

    // Register a Nomad tab spawner for the editor window
    FGlobalTabmanager::Get()
        ->RegisterNomadTabSpawner(MetaWeaverTabName, FOnSpawnTab::CreateRaw(this, &FMetaWeaverModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("MetaWeaverTabTitle", "MetaWeaver"))
        .SetTooltipText(LOCTEXT("MetaWeaverTabTooltip", "MetaWeaver Metadata Editor"))
        .SetGroup(MetaWeaverMenuItem.ToSharedRef())
        .SetIcon(FMetaWeaverStyle::GetNomadTabIcon());

    // Register the bulk editor tab (multi-selection)
    FGlobalTabmanager::Get()
        ->RegisterNomadTabSpawner(MetaWeaverBulkTabName,
                                  FOnSpawnTab::CreateRaw(this, &FMetaWeaverModule::OnSpawnBulkTab))
        .SetDisplayName(LOCTEXT("MetaWeaverBulkTabTitle", "MetaWeaver Bulk"))
        .SetTooltipText(LOCTEXT("MetaWeaverBulkTabTooltip", "MetaWeaver Bulk Metadata Editor"))
        .SetGroup(MetaWeaverMenuItem.ToSharedRef())
        .SetIcon(FMetaWeaverStyle::GetNomadTabIcon());

    if (UToolMenus::IsToolMenuUIEnabled())
    {
        ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMetaWeaverModule::RegisterMenus));
    }

    // Ensure our tabs are closed before major subsystems tear down to avoid late delegate removals
    PreExitHandle = FCoreDelegates::OnPreExit.AddRaw(this, &FMetaWeaverModule::OnPreExit);
}

void FMetaWeaverModule::ShutdownModule()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        if (ToolMenusStartupHandle.IsValid())
        {
            UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
        }
        if (const auto ToolMenus = UToolMenus::Get())
        {
            ToolMenus->RemoveMenu(MetaWeaverMenuName);
        }
        UToolMenus::UnregisterOwner(this);
    }

    FMetaWeaverCommands::Unregister();
    FMetaWeaverStyle::Shutdown();

    // Unregister tab spawners
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MetaWeaverTabName);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MetaWeaverBulkTabName);

    if (PreExitHandle.IsValid())
    {
        FCoreDelegates::OnPreExit.Remove(PreExitHandle);
    }

    UE_LOG(LogMetaWeaver, Log, TEXT("MetaWeaver module shutting down"));
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void FMetaWeaverModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(MetaWeaverTabName);
}

void FMetaWeaverModule::RegisterMenus()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        if (const auto ToolMenus = UToolMenus::Get())
        {
            // Prefer placing under the standard "Asset Actions" submenu
            if (const auto Menu = ToolMenus->ExtendMenu(TEXT("ContentBrowser.AssetContextMenu.AssetActionsSubMenu")))
            {
                auto& Section = Menu->FindOrAddSection(TEXT("AssetContextAdvancedActions"));

                auto Entry = FToolMenuEntry::InitMenuEntry(
                    TEXT("MetaWeaver_EditMetadata"),
                    LOCTEXT("MetaWeaver_EditMetadata_Label", "Edit Metadata..."),
                    LOCTEXT("MetaWeaver_EditMetadata_Tooltip",
                            "Open MetaWeaver to edit metadata for the selected asset."),
                    FMetaWeaverStyle::GetNomadTabIcon(),
                    FToolMenuExecuteAction::CreateRaw(this, &FMetaWeaverModule::EditMetadataSingleFromContext));
                // Try to place just below the existing "Show Metadata" entry
                Entry.InsertPosition = FToolMenuInsert(TEXT("ShowAssetMetaData"), EToolMenuInsertType::After);
                Section.AddEntry(Entry);
                auto BulkEntry = FToolMenuEntry::InitMenuEntry(
                    TEXT("MetaWeaver_BulkEditMetadata"),
                    LOCTEXT("MetaWeaver_BulkEditMetadata_Label", "Bulk Edit Metadata..."),
                    LOCTEXT("MetaWeaver_BulkEditMetadata_Tooltip",
                            "Open the MetaWeaver Bulk editor for the selected assets."),
                    FMetaWeaverStyle::GetNomadTabIcon(),
                    FToolMenuExecuteAction::CreateRaw(this, &FMetaWeaverModule::EditMetadataBulkFromContext));
                BulkEntry.InsertPosition = FToolMenuInsert(TEXT("ShowAssetMetaData"), EToolMenuInsertType::After);
                Section.AddEntry(BulkEntry);
            }
        }
    }
}

void FMetaWeaverModule::FillMetaWeaverSubMenu(UToolMenu* SubMenu) const
{
    if (SubMenu)
    {
        auto& SubSection = SubMenu->FindOrAddSection("MetaWeaverActions", LOCTEXT("MetaWeaverActions", "MetaWeaver"));
        SubSection.AddMenuEntryWithCommandList(FMetaWeaverCommands::Get().OpenMetaWeaver, PluginCommands);
    }
}

TSharedRef<SDockTab> FMetaWeaverModule::OnSpawnPluginTab(const FSpawnTabArgs&) const
{
    return SNew(SDockTab).TabRole(NomadTab)[SNew(SMetaWeaverEditor).SelectedAssets(PendingSelectedAssets)];
}

TSharedRef<SDockTab> FMetaWeaverModule::OnSpawnBulkTab(const FSpawnTabArgs&) const
{
    return SNew(SDockTab).TabRole(NomadTab)[SNew(SMetaWeaverBulkEditor).SelectedAssets(PendingSelectedAssets)];
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void FMetaWeaverModule::OnPreExit()
{
    // Close our tabs early so any widget delegate cleanup happens while engine systems are still valid
    if (const auto Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(MetaWeaverTabName))
    {
        Tab->RequestCloseTab();
    }
    if (const auto Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(MetaWeaverBulkTabName))
    {
        Tab->RequestCloseTab();
    }
}

void FMetaWeaverModule::EditMetadataSingleFromContext(const FToolMenuContext& Context)
{
    PendingSelectedAssets.Reset();
    if (const auto MenuContext = Context.FindContext<UContentBrowserAssetContextMenuContext>())
    {
        if (MenuContext->SelectedAssets.Num() > 0)
        {
            PendingSelectedAssets.Add(MenuContext->SelectedAssets[0]);
        }
    }
    FGlobalTabmanager::Get()->TryInvokeTab(MetaWeaverTabName);
}

void FMetaWeaverModule::EditMetadataBulkFromContext(const FToolMenuContext& Context)
{
    PendingSelectedAssets.Reset();
    if (const auto MenuContext = Context.FindContext<UContentBrowserAssetContextMenuContext>())
    {
        PendingSelectedAssets = MenuContext->SelectedAssets;
    }
    FGlobalTabmanager::Get()->TryInvokeTab(MetaWeaverBulkTabName);
}

IMPLEMENT_MODULE(FMetaWeaverModule, MetaWeaver)

#undef LOCTEXT_NAMESPACE
