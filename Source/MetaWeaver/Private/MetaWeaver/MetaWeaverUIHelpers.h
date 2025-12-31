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

struct FMetadataParameterSpec;

namespace MetaWeaver::UIHelpers
{
    /**
     * Populate an Options array for SComboBox from a list of enum strings.
     * Resets OutOptions and appends shared string entries. Optionally sorts.
     */
    void BuildEnumOptions(const TArray<FString>& Values, TArray<TSharedPtr<FString>>& OutOptions, bool bSort = true);

    /**
     * Populate an Options array for SComboBox from a set of enum strings.
     * Resets OutOptions and appends shared string entries. Optionally sorts.
     */
    void
    BuildEnumOptionsFromSet(const TSet<FString>& Values, TArray<TSharedPtr<FString>>& OutOptions, bool bSort = true);

    /**
     * Get or build cached enum options for the given spec key.
     * Ensures the returned reference refers to an array stored in Cache (stable for SComboBox OptionsSource).
     */
    const TArray<TSharedPtr<FString>>& GetOrBuildEnumOptions(TMap<FName, TArray<TSharedPtr<FString>>>& Cache,
                                                             const FMetadataParameterSpec& Spec,
                                                             bool bSort = true);
} // namespace MetaWeaver::UIHelpers
