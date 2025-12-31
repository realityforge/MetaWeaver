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
#include "MetaWeaver/Validation/MetaWeaverValidationTypes.h"
#include "MetaWeaverValidationSubsystem.generated.h"

class UMetaWeaverMetadataDefinitionSet;

/**
 * Public validation API for other editor modules to consume.
 * Abstracts over MetaWeaver's internal definition assets.
 */
UCLASS()
class UMetaWeaverValidationSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // Fired when any definition set is edited or saved, so UIs can refresh specs.
    DECLARE_MULTICAST_DELEGATE(FOnDefinitionSetsChanged);

    // Validate a single asset using active project definition sets
    UFUNCTION(BlueprintCallable, Category = "MetaWeaver|Validation")
    FMetaWeaverValidationReport ValidateAsset(UObject* Asset) const;

    // Validate a single key/value for the specified class
    UFUNCTION(BlueprintCallable, Category = "MetaWeaver|Validation")
    FMetaWeaverValidationReport ValidateKeyValue(TSubclassOf<UObject> Class, FName Key, const FString& Value) const;

    // Notify listeners that definition sets changed; used by asset classes on edits/saves
    void NotifyDefinitionSetsChanged() const;

    // Accessor for the definition-changed event
    FOnDefinitionSetsChanged& GetOnDefinitionSetsChanged() { return DefinitionSetsChangedEvent; }

private:
    void GatherActiveDefinitionSets(TArray<UMetaWeaverMetadataDefinitionSet*>& OutSets) const;
    void ResolveEffectiveParametersForClass(const UClass* Class, TArray<struct FMetadataParameterSpec>& OutSpecs) const;
    void ValidateAgainstSpecs(UObject* Asset,
                              const TArray<FMetadataParameterSpec>& Specs,
                              FMetaWeaverValidationReport& OutReport) const;

    FOnDefinitionSetsChanged DefinitionSetsChangedEvent;
};
