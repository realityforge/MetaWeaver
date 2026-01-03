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
#include "MetaWeaver/Validation/MetaWeaverValidationSubsystem.h"
#include "MetaWeaver/MetaWeaverAggregation.h"
#include "MetaWeaver/MetaWeaverMetadataDefinitionSet.h"
#include "MetaWeaver/MetaWeaverMetadataStore.h"
#include "MetaWeaver/MetaWeaverProjectSettings.h"
#include "MetaWeaver/MetaWeaverTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MetaWeaverValidationSubsystem)

// ReSharper disable once CppMemberFunctionMayBeStatic
void UMetaWeaverValidationSubsystem::GatherSpecsForClass(const UClass* Class,
                                                         TArray<FMetadataParameterSpec>& OutSpecs) const
{
    OutSpecs.Reset();
    if (Class)
    {
        TArray<UMetaWeaverMetadataDefinitionSet*> OrderedSets;
        if (const auto Settings = GetDefault<UMetaWeaverProjectSettings>())
        {
            MetaWeaver::Aggregation::FlattenActiveSets(Settings->ActiveDefinitionSets, OrderedSets);
        }
        MetaWeaver::Aggregation::BuildEffectiveSpecsForClass(Class, OrderedSets, OutSpecs);
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void UMetaWeaverValidationSubsystem::ValidateAgainstSpecs(UObject* Asset,
                                                          const TArray<FMetadataParameterSpec>& Specs,
                                                          FMetaWeaverValidationReport& OutReport) const
{
    if (Asset)
    {
        OutReport.Asset = Asset;

        // Build a map of current metadata
        TMap<FName, FString> Tags;
        FMetaWeaverMetadataStore::ListMetadataTags(Asset, Tags);

        for (const auto& Spec : Specs)
        {
            const auto Found = Tags.Find(Spec.Key);
            if (!Found)
            {
                if (Spec.bRequired)
                {
                    FMetaWeaverIssue Issue;
                    Issue.Key = Spec.Key;
                    Issue.Severity = EMetaWeaverIssueSeverity::Error;
                    Issue.Message = FText::FromString(TEXT("Required metadata key is missing."));
                    OutReport.Issues.Add(MoveTemp(Issue));
                    OutReport.bHasErrors = true;
                }
                continue;
            }

            // Validate formatting/types using typed value canonicalization
            if (FString Canonical; !FMetaWeaverValue::Canonicalize(Spec.Type, *Found, Canonical))
            {
                FMetaWeaverIssue Issue;
                Issue.Key = Spec.Key;
                Issue.Severity = EMetaWeaverIssueSeverity::Error;
                Issue.Message =
                    FText::FromString(TEXT("Metadata value is not correctly formatted for the expected type."));
                OutReport.Issues.Add(MoveTemp(Issue));
                OutReport.bHasErrors = true;
                continue;
            }

            if (EMetaWeaverValueType::AssetReference == Spec.Type && Spec.AllowedClass)
            {
                const FSoftObjectPath Path(*Found);
                if (auto Resolved = Path.TryLoad())
                {
                    if (!Resolved->IsA(Spec.AllowedClass))
                    {
                        FMetaWeaverIssue Issue;
                        Issue.Key = Spec.Key;
                        Issue.Severity = EMetaWeaverIssueSeverity::Error;
                        Issue.Message = FText::FromString(TEXT("Referenced asset is not of an allowed class."));
                        OutReport.Issues.Add(MoveTemp(Issue));
                        OutReport.bHasErrors = true;
                    }
                }
                else
                {
                    // If it can't resolve, still flag formatting ok but warn about unresolved path
                    FMetaWeaverIssue Issue;
                    Issue.Key = Spec.Key;
                    Issue.Severity = EMetaWeaverIssueSeverity::Warning;
                    Issue.Message = FText::FromString(TEXT("Asset reference could not be resolved in editor."));
                    OutReport.Issues.Add(MoveTemp(Issue));
                }
            }
            else if (EMetaWeaverValueType::Enum == Spec.Type)
            {
                bool bFoundInEnum = false;
                for (const auto& V : Spec.EnumValues)
                {
                    if (Found->Equals(V, ESearchCase::CaseSensitive))
                    {
                        bFoundInEnum = true;
                        break;
                    }
                }
                if (!bFoundInEnum)
                {
                    FMetaWeaverIssue Issue;
                    Issue.Key = Spec.Key;
                    Issue.Severity = EMetaWeaverIssueSeverity::Error;
                    Issue.Message = FText::FromString(TEXT("Value is not in the allowed enumeration list."));
                    OutReport.Issues.Add(MoveTemp(Issue));
                    OutReport.bHasErrors = true;
                }
            }
        }
    }
    else
    {
    }
}

FMetaWeaverValidationReport UMetaWeaverValidationSubsystem::ValidateAsset(UObject* Asset) const
{
    FMetaWeaverValidationReport Report;
    if (Asset)
    {
        TArray<FMetadataParameterSpec> Specs;
        GatherSpecsForClass(Asset->GetClass(), Specs);
        ValidateAgainstSpecs(Asset, Specs, Report);
    }
    return Report;
}

FMetaWeaverValidationReport
UMetaWeaverValidationSubsystem::ValidateKeyValue(TSubclassOf<UObject> Class, FName Key, const FString& Value) const
{
    FMetaWeaverValidationReport Report;
    if (Class)
    {
        TArray<FMetadataParameterSpec> Specs;
        GatherSpecsForClass(Class, Specs);

        // Perform a direct spec lookup
        for (const auto& Spec : Specs)
        {
            if (Spec.Key == Key)
            {
                // ReSharper disable once CppTooWideScopeInitStatement
                FString Canonical;
                // Basic formatting
                if (Value.IsEmpty() || !FMetaWeaverValue::Canonicalize(Spec.Type, Value, Canonical))
                {
                    FMetaWeaverIssue Issue;
                    Issue.Key = Spec.Key;
                    Issue.Severity = EMetaWeaverIssueSeverity::Error;
                    Issue.Message =
                        FText::FromString(TEXT("Metadata value is not correctly formatted for the expected type."));
                    Report.Issues.Add(MoveTemp(Issue));
                    Report.bHasErrors = true;
                }
                else if (EMetaWeaverValueType::Enum == Spec.Type)
                {
                    bool bFoundInEnum = false;
                    for (const auto& V : Spec.EnumValues)
                    {
                        if (Value.Equals(V, ESearchCase::CaseSensitive))
                        {
                            bFoundInEnum = true;
                            break;
                        }
                    }
                    if (!bFoundInEnum)
                    {
                        FMetaWeaverIssue Issue;
                        Issue.Key = Spec.Key;
                        Issue.Severity = EMetaWeaverIssueSeverity::Error;
                        Issue.Message = FText::FromString(TEXT("Value is not in the allowed enumeration list."));
                        Report.Issues.Add(MoveTemp(Issue));
                        Report.bHasErrors = true;
                    }
                }
                break;
            }
        }
    }
    return Report;
}

void UMetaWeaverValidationSubsystem::NotifyDefinitionSetsChanged() const
{
    DefinitionSetsChangedEvent.Broadcast();
}
