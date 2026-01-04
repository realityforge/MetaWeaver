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
#include "MetaWeaver/MetaWeaverMetadataDefinitionSet.h"
#include "MetaWeaverLogging.h"

namespace MetaWeaver::Aggregation
{
    inline void GatherIncludedSetsRecursive(UMetaWeaverMetadataDefinitionSet* Root,
                                            TArray<UMetaWeaverMetadataDefinitionSet*>& OutSets,
                                            TSet<const UMetaWeaverMetadataDefinitionSet*>& Visited)
    {
        if (Root && !Visited.Contains(Root))
        {
            Visited.Add(Root);

            // Include referenced sets first so that the including set can override them
            int Index = 0;
            for (const auto& SoftIncluded : Root->MetadataDefinitionSets)
            {
                if (const auto Included = SoftIncluded.LoadSynchronous())
                {
                    GatherIncludedSetsRecursive(Included, OutSets, Visited);
                }
                else
                {
                    UE_LOG(LogMetaWeaver,
                           Error,
                           TEXT("Failed to load referenced set %s at Index=%d in asset %s"),
                           *SoftIncluded.ToString(),
                           Index,
                           *Root->GetPathName());
                }
                Index++;
            }

            // Then include the root itself
            OutSets.Add(Root);
        }
    }

    inline void FlattenActiveSets(const TArray<TSoftObjectPtr<UMetaWeaverMetadataDefinitionSet>>& ActiveSets,
                                  TArray<UMetaWeaverMetadataDefinitionSet*>& OutSets)
    {
        OutSets.Reset();
        TSet<const UMetaWeaverMetadataDefinitionSet*> Visited;

        // Precedence: later active sets override earlier. So append expansions in order.
        for (const auto& SoftRoot : ActiveSets)
        {
            if (const auto Root = SoftRoot.LoadSynchronous())
            {
                GatherIncludedSetsRecursive(Root, OutSets, Visited);
            }
        }
    }

    inline void BuildEffectiveSpecsForClass(const UClass* Class,
                                            const TArray<UMetaWeaverMetadataDefinitionSet*>& OrderedSets,
                                            TArray<FMetadataParameterSpec>& OutSpecs)
    {
        check(Class);

        OutSpecs.Reset();
        TMap<FName, FMetadataParameterSpec> ByKey;
        for (const auto Set : OrderedSets)
        {
            if (Set)
            {
                for (const auto& Group : Set->ParameterSets)
                {
                    if (!Group.ObjectType || Class->IsChildOf(Group.ObjectType))
                    {
                        for (const auto& Spec : Group.Parameters)
                        {
                            if (!Spec.Key.IsNone())
                            {
                                // Last writer wins according to OrderedSets traversal order
                                ByKey.FindOrAdd(Spec.Key) = Spec;
                            }
                        }
                    }
                }
            }
        }
        ByKey.GenerateValueArray(OutSpecs);
    }
} // namespace MetaWeaver::Aggregation
