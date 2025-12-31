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

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetaWeaverEditorSettings.generated.h"

/**
 * Per-user editor preferences for MetaWeaver UIs.
 * Stored in EditorPerProjectUserSettings so preferences are not shared across users.
 */
UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "MetaWeaver Editor"))
class UMetaWeaverEditorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    // Whether to initially lock the editors to the Content Browser selection.
    UPROPERTY(EditAnywhere, Config, Category = "MetaWeaver")
    bool bLockToSelectionDefault{ false };

    // Last pinned metadata keys for the bulk editor (global list).
    // Note: Keys that are not applicable to the current selection are ignored.
    UPROPERTY(EditAnywhere, Config, Category = "MetaWeaver")
    TArray<FName> LastPinnedKeys;
};
