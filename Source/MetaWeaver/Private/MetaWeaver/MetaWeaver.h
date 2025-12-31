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
#include "Modules/ModuleManager.h"

struct FToolMenuContext;
class UToolMenu;
class SDockTab;
class FSpawnTabArgs;
class FUICommandList;
class FWorkspaceItem;

class FMetaWeaverModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void PluginButtonClicked();
    void RegisterMenus();
    void EditMetadataSingleFromContext(const FToolMenuContext& Context);
    void EditMetadataBulkFromContext(const FToolMenuContext& Context);
    void FillMetaWeaverSubMenu(UToolMenu* SubMenu) const;
    TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs) const;
    TSharedRef<SDockTab> OnSpawnBulkTab(const FSpawnTabArgs& SpawnTabArgs) const;
    void OnPreExit();

    TSharedPtr<FUICommandList> PluginCommands;
    TArray<FAssetData> PendingSelectedAssets;

    /** Global MetaWeaver workspace menu item */
    TSharedPtr<FWorkspaceItem> MetaWeaverMenuItem;

    FDelegateHandle ToolMenusStartupHandle;
    FDelegateHandle PreExitHandle;
};
