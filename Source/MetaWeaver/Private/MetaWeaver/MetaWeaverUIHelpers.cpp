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
#include "MetaWeaverUIHelpers.h"
#include "MetaWeaver/MetaWeaverMetadataDefinitionSet.h"

namespace MetaWeaver::UIHelpers
{
    void BuildEnumOptions(const TArray<FString>& Values, TArray<TSharedPtr<FString>>& OutOptions, const bool bSort)
    {
        OutOptions.Reset();
        OutOptions.Reserve(Values.Num());
        for (const auto& V : Values)
        {
            OutOptions.Add(MakeShared<FString>(V));
        }
        if (bSort)
        {
            OutOptions.Sort([](const auto& A, const auto& B) {
                return (A.IsValid() ? *A : TEXT("")) < (B.IsValid() ? *B : TEXT(""));
            });
        }
    }

    void BuildEnumOptionsFromSet(const TSet<FString>& Values, TArray<TSharedPtr<FString>>& OutOptions, const bool bSort)
    {
        OutOptions.Reset();
        OutOptions.Reserve(Values.Num());
        for (const auto& V : Values)
        {
            OutOptions.Add(MakeShared<FString>(V));
        }
        if (bSort)
        {
            OutOptions.Sort([](const auto& A, const auto& B) {
                return (A.IsValid() ? *A : TEXT("")) < (B.IsValid() ? *B : TEXT(""));
            });
        }
    }

    const TArray<TSharedPtr<FString>>& GetOrBuildEnumOptions(TMap<FName, TArray<TSharedPtr<FString>>>& Cache,
                                                             const FMetadataParameterSpec& Spec,
                                                             const bool bSort)
    {
        if (const auto Found = Cache.Find(Spec.Key))
        {
            return *Found;
        }
        else
        {
            TArray<TSharedPtr<FString>> Options;
            BuildEnumOptions(Spec.EnumValues, Options, bSort);
            return Cache.Add(Spec.Key, MoveTemp(Options));
        }
    }
} // namespace MetaWeaver::UIHelpers
