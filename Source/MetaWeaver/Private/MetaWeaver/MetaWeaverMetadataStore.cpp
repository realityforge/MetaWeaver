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
#include "MetaWeaverMetadataStore.h"
#include "Editor.h"
#include "MetaWeaverAggregation.h"
#include "MetaWeaverMetadataDefinitionSet.h"
#include "MetaWeaverProjectSettings.h"
#include "ScopedTransaction.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UObject/MetaData.h"

UEditorAssetSubsystem* FMetaWeaverMetadataStore::GetEditorAssetSubsystem()
{
    return GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
}

bool FMetaWeaverMetadataStore::GetMetadataTag(const UObject* Asset, const FName Key, FString& OutValue)
{
    if (const auto Subsystem = GetEditorAssetSubsystem())
    {
        UObject* MutableAsset = const_cast<UObject*>(Asset);
        OutValue = Subsystem->GetMetadataTag(MutableAsset, Key);
        return !OutValue.IsEmpty();
    }
    return false;
}

bool FMetaWeaverMetadataStore::SetMetadataTag(UObject* Asset, const FName Key, const FString& Value)
{
    if (const auto Subsystem = GetEditorAssetSubsystem())
    {
        Subsystem->SetMetadataTag(Asset, Key, Value);
        return true;
    }
    return false;
}

bool FMetaWeaverMetadataStore::RemoveMetadataTag(UObject* Asset, const FName Key)
{
    if (const auto Subsystem = GetEditorAssetSubsystem())
    {
        Subsystem->RemoveMetadataTag(Asset, Key);
        return true;
    }
    return false;
}

bool FMetaWeaverMetadataStore::ListMetadataTags(const UObject* Asset, TMap<FName, FString>& OutTags)
{
    OutTags.Reset();
    if (Asset)
    {
        if (const auto Package = Asset->GetOutermost())
        {
            if (const auto Map = Package->GetMetaData().GetMapForObject(Asset))
            {
                OutTags = *Map;
                return OutTags.Num() > 0;
            }
        }
    }
    return false;
}

void FMetaWeaverMetadataStore::GatherSpecsForClass(const UClass* Class, TArray<FMetadataParameterSpec>& OutSpecs)
{
    OutSpecs.Reset();
    if (Class)
    {
        if (const auto Settings = GetDefault<UMetaWeaverProjectSettings>())
        {
            // Flatten active sets (recursive, cycle-safe) with precedence
            TArray<UMetaWeaverMetadataDefinitionSet*> OrderedSets;
            MetaWeaver::Aggregation::FlattenActiveSets(Settings->ActiveDefinitionSets, OrderedSets);
            // Build effective specs per class from ordered sets
            MetaWeaver::Aggregation::BuildEffectiveSpecsForClass(Class, OrderedSets, OutSpecs);
        }
    }
}
