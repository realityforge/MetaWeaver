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

        const auto SetNames = FString::JoinBy(OutSets, TEXT(", "), [](auto Set) { return Set->GetName(); });
        UE_LOG(LogMetaWeaver, Verbose, TEXT("Active MetaWeaverDefinitionSets: %d [%s]"), OutSets.Num(), *SetNames);
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
            check(Set);
            int Index{ 0 };
            for (const auto& ParameterSet : Set->ParameterSets)
            {
                UE_LOG(LogMetaWeaver,
                       Verbose,
                       TEXT("MetaWeaverMetadataDefinitionSet[%s].ParameterSet[%d] with "
                            "ObjectType='%s' attempting to match class '%s'"),
                       *GetNameSafe(Set),
                       Index,
                       *GetNameSafe(ParameterSet.ObjectType),
                       *GetNameSafe(Class));

                if (!ParameterSet.ObjectType || Class == ParameterSet.ObjectType.Get()
                    || Class->IsChildOf(ParameterSet.ObjectType))
                {
                    int ParameterIndex{ 0 };
                    for (const auto& Parameter : ParameterSet.Parameters)
                    {
                        if (!Parameter.Key.IsNone())
                        {
                            if (ByKey.Contains(Parameter.Key))
                            {
                                UE_LOG(LogMetaWeaver,
                                       Verbose,
                                       TEXT("MetaWeaverMetadataDefinitionSet[%s].ParameterSet[%d].Parameters[%d].Key "
                                            "'%s' is overriding an earlier key in specs."),
                                       *Set->GetName(),
                                       Index,
                                       ParameterIndex,
                                       *Parameter.Key.ToString());
                            }
                            else
                            {
                                UE_LOG(LogMetaWeaver,
                                       Verbose,
                                       TEXT("MetaWeaverMetadataDefinitionSet[%s].ParameterSet[%d].Parameters[%d].Key "
                                            "'%s' has been added to specs."),
                                       *Set->GetName(),
                                       Index,
                                       ParameterIndex,
                                       *Parameter.Key.ToString());
                            }
                            // Last writer wins according to OrderedSets traversal order
                            ByKey.FindOrAdd(Parameter.Key) = Parameter;
                        }
                        else
                        {
                            UE_LOG(LogMetaWeaver,
                                   Warning,
                                   TEXT("MetaWeaverMetadataDefinitionSet[%s].ParameterSet[%d].Parameters[%d].Key "
                                        "is Empty. Ignoring."),
                                   *Set->GetName(),
                                   Index,
                                   ParameterIndex);
                        }
                        ParameterIndex++;
                    }
                }
                Index++;
            }
        }
        ByKey.GenerateValueArray(OutSpecs);
        const auto KeyNames = FString::JoinBy(OutSpecs, TEXT(", "), [](const auto& Set) { return Set.Key.ToString(); });
        UE_LOG(LogMetaWeaver,
               Verbose,
               TEXT("MetadataParameterSpec's gathered for class '%s': [%s]"),
               *GetNameSafe(Class),
               *KeyNames);
    }
} // namespace MetaWeaver::Aggregation
