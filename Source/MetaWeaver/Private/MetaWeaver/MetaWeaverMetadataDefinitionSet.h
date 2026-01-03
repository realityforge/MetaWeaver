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
#include "Engine/DataAsset.h"
#include "MetaWeaver/MetaWeaverValueTypes.h"
#include "MetaWeaverMetadataDefinitionSet.generated.h"

USTRUCT(BlueprintType)
struct FMetadataParameterSpec
{
    GENERATED_BODY()

    /** The MetaData key. */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver")
    FName Key{ NAME_None };

    /** The data type expected for the MetaData key. */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver")
    EMetaWeaverValueType Type{ EMetaWeaverValueType::String };

    /** Optional human-readable description shown in the editor UI as a tooltip. */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver", meta = (MultiLine = true))
    FString Description;

    /** Default value serialized in canonical string form */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver")
    FString DefaultValue;

    /** Whether this key is required for assets of the specified class. */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver")
    bool bRequired{ false };

    /** If Type is AssetReference, restrict to these declared subclasses */
    UPROPERTY(EditDefaultsOnly,
              Category = "MetaWeaver",
              meta = (AllowAbstract = true,
                      EditCondition = "EMetaWeaverValueType::AssetReference==Type",
                      EditConditionHides))
    TSubclassOf<UObject> AllowedClass;

    /** Allowed enum values when Type == Enum. Always treated as exhaustive; trimmed/uniqued in PreSave. */
    UPROPERTY(EditDefaultsOnly,
              Category = "MetaWeaver",
              meta = (EditCondition = "EMetaWeaverValueType::Enum==Type", EditConditionHides))
    TArray<FString> EnumValues;

private:
    friend struct FMetaWeaverObjectParameterSet;

    // Clean struct before saving
    void PreSave();

    // This is called from FMetaWeaverObjectParameterSet
    EDataValidationResult IsDataValid(FDataValidationContext& Context, const FString& ContextPath) const;
};

/**
 * Associates a target UObject class with its applicable parameter specs.
 */
USTRUCT(BlueprintType)
struct FMetaWeaverObjectParameterSet
{
    GENERATED_BODY()

    /** Asset Class this parameter set applies to (soft reference for editor friendliness). */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver", meta = (AllowAbstract = true))
    TSubclassOf<UObject> ObjectType{ nullptr };

    /** Parameter specifications for this asset class. */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver")
    TArray<FMetadataParameterSpec> Parameters;

private:
    friend class UMetaWeaverMetadataDefinitionSet;

    // Sort certain properties before saving
    void PreSave();

    // This is called from UMetaWeaverMetadataDefinitionSet
    EDataValidationResult IsDataValid(FDataValidationContext& Context, const FString& ContextPath) const;
};

/**
 * Representation that associated MetaData parameter definitions with different ObjectTypes.
 * A set can aggregate other definition sets and may add additional parameter definitions.
 */
UCLASS(BlueprintType)
class UMetaWeaverMetadataDefinitionSet final : public UDataAsset
{
    GENERATED_BODY()

public:
    /** MetadataDefinitionSets that are aggregated into this asset. */
    UPROPERTY(EditDefaultsOnly,
              Category = "MetaWeaver",
              meta = (DisplayThumbnail = "false", ForceShowPluginContent = "true"))
    TArray<TSoftObjectPtr<UMetaWeaverMetadataDefinitionSet>> MetadataDefinitionSets;

    /** Metadata structure that declares parameters applicable to specific classes. */
    UPROPERTY(EditDefaultsOnly, Category = "MetaWeaver")
    TArray<FMetaWeaverObjectParameterSet> ParameterSets;

    // Sort certain properties before saving
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;

    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};
