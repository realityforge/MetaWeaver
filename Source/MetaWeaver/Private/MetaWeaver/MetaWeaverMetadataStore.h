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

struct FMetadataParameterSpec;
class UObject;
class UEditorAssetSubsystem;

/**
 * Thin adapter over UEditorAssetSubsystem for asset metadata tags.
 */
class FMetaWeaverMetadataStore final
{
public:
    // Single-asset operations
    static bool GetMetadataTag(const UObject* Asset, FName Key, FString& OutValue);
    static bool SetMetadataTag(UObject* Asset, FName Key, const FString& Value);
    static bool RemoveMetadataTag(UObject* Asset, FName Key);

    // Enumerate all metadata tags via UMetaData
    static bool ListMetadataTags(const UObject* Asset, TMap<FName, FString>& OutTags);

    static void GatherSpecsForClass(const UClass* Class, TArray<FMetadataParameterSpec>& OutSpecs);

private:
    // Subsystem access
    static UEditorAssetSubsystem* GetEditorAssetSubsystem();
};
