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
#include "MetaWeaver/MetaWeaverMetadataDefinitionSet.h"
#include "MetaWeaver/MetaWeaverTypes.h"
#include "MetaWeaver/Validation/MetaWeaverValidationSubsystem.h"
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MetaWeaverMetadataDefinitionSet)

#if WITH_EDITOR

EDataValidationResult FMetadataParameterSpec::IsDataValid(FDataValidationContext& Context,
                                                          const FString& ContextPath) const
{
    auto Result = EDataValidationResult::Valid;

    if (Key.IsNone())
    {
        const auto String = FString::Printf(TEXT("%s has not specified the Key property"), *ContextPath);
        Context.AddError(FText::FromString(String));
        Result = EDataValidationResult::Invalid;
    }

    // If DefaultValue is set, ensure it is correctly formatted for the declared Type
    if (!DefaultValue.IsEmpty())
    {
        FString Canonical;
        if (!FMetaWeaverValue::Canonicalize(Type, DefaultValue, Canonical))
        {
            const auto Enum = StaticEnum<EMetaWeaverValueType>();
            const auto TypeText = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Type)) : TEXT("Unknown");
            const auto Message =
                FString::Printf(TEXT("%s.DefaultValue is not valid for Type '%s'."), *ContextPath, *TypeText);
            Context.AddError(FText::FromString(Message));
            Result = EDataValidationResult::Invalid;
        }
        else if (EMetaWeaverValueType::AssetReference == Type)
        {
            // For asset references, if resolvable, ensure it matches AllowedClass
            const FSoftObjectPath Path(DefaultValue);
            if (const auto Resolved = Path.TryLoad())
            {
                if (nullptr != AllowedClass.Get() && !Resolved->IsA(AllowedClass))
                {
                    const auto Message =
                        FString::Printf(TEXT("%s.DefaultValue references an asset that is not a '%s'."),
                                        *ContextPath,
                                        *AllowedClass->GetName());
                    Context.AddError(FText::FromString(Message));
                    Result = EDataValidationResult::Invalid;
                }
            }
            else
            {
                // If asset cannot be resolved in editor, warn but do not hard-fail formatting
                const auto Message =
                    FString::Printf(TEXT("%s.DefaultValue asset reference could not be resolved in editor."),
                                    *ContextPath);
                Context.AddWarning(FText::FromString(Message));
            }
        }
        else if (EMetaWeaverValueType::Enum == Type)
        {
            bool bFound = false;
            for (const auto& V : EnumValues)
            {
                if (DefaultValue.Equals(V, ESearchCase::CaseSensitive))
                {
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                const auto Msg = FString::Printf(TEXT("%s.DefaultValue is not in EnumValues."), *ContextPath);
                Context.AddError(FText::FromString(Msg));
                Result = EDataValidationResult::Invalid;
            }
        }
    }

    return Result;
}

EDataValidationResult FMetaWeaverObjectParameterSet::IsDataValid(FDataValidationContext& Context,
                                                                 const FString& ContextPath) const
{
    auto Result = EDataValidationResult::Valid;

    // Validate each parameter entry
    for (auto Index = 0; Index < Parameters.Num(); ++Index)
    {
        const auto& Parameter = Parameters[Index];
        const auto Path = FString::Printf(TEXT("%s.Parameters[%d]"), *ContextPath, Index);
        Result = CombineDataValidationResults(Parameter.IsDataValid(Context, Path), Result);
    }

    // Ensure that Parameters have unique keys
    {
        TSet<FName> Seen;
        for (auto Index = 0; Index < Parameters.Num(); ++Index)
        {
            if (const auto Key = Parameters[Index].Key; !Key.IsNone())
            {
                if (Seen.Contains(Key))
                {
                    const auto Msg = FString::Printf(TEXT("%s.Parameters[%d] has a duplicate Key '%s'"),
                                                     *ContextPath,
                                                     Index,
                                                     *Key.ToString());
                    Context.AddError(FText::FromString(Msg));
                    Result = EDataValidationResult::Invalid;
                }
                else
                {
                    Seen.Add(Key);
                }
            }
        }
    }

    return Result;
}

EDataValidationResult UMetaWeaverMetadataDefinitionSet::IsDataValid(FDataValidationContext& Context) const
{
    auto Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

    auto Index{ 0 };
    for (const auto& ParameterSet : ParameterSets)
    {
        const auto ContextPath = FString::Printf(TEXT("ParameterSets[%d]"), Index++);
        Result = CombineDataValidationResults(ParameterSet.IsDataValid(Context, ContextPath), Result);
    }

    Index = 0;
    for (const auto& MetadataDefinitionSet : MetadataDefinitionSets)
    {
        if (MetadataDefinitionSet.IsNull())
        {
            const auto String = FString::Printf(TEXT("MetadataDefinitionSet[%d] has not been specified"), Index);
            Context.AddError(FText::FromString(String));
            Result = EDataValidationResult::Invalid;
        }
        Index++;
    }

    return Result;
}

#endif

void FMetadataParameterSpec::PreSave()
{
    DefaultValue = DefaultValue.TrimStartAndEnd();
    Description = Description.TrimStartAndEnd();
    if (EMetaWeaverValueType::AssetReference != Type)
    {
        AllowedClass = nullptr;
    }
    if (EMetaWeaverValueType::Enum != Type)
    {
        EnumValues.Reset();
    }
    else
    {
        // Clean EnumValues: trim, drop empty, unique, sort
        TArray<FString> Clean;
        Clean.Reserve(EnumValues.Num());
        TSet<FString> Seen;
        for (const auto& V : EnumValues)
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            const auto T = V.TrimStartAndEnd();
            if (!T.IsEmpty() && !Seen.Contains(T))
            {
                Seen.Add(T);
                Clean.Add(T);
            }
        }
        Clean.Sort();
        EnumValues = MoveTemp(Clean);
    }
}

void FMetaWeaverObjectParameterSet::PreSave()
{
    for (auto& Parameter : Parameters)
    {
        Parameter.PreSave();
    }
    Parameters.RemoveAll([](const auto& Value) { return Value.Key.IsNone(); });
    Parameters.Sort([](const auto& A, const auto& B) { return A.Key.ToString() < B.Key.ToString(); });
}

void UMetaWeaverMetadataDefinitionSet::PreSave(const FObjectPreSaveContext SaveContext)
{
    // Remove empty definition set entries
    MetadataDefinitionSets.RemoveAll([](const auto& Value) { return Value.IsNull(); });
    for (auto& ParameterSet : ParameterSets)
    {
        ParameterSet.PreSave();
    }
    Super::PreSave(SaveContext);

#if WITH_EDITOR
    if (GEditor)
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
        {
            Subsystem->NotifyDefinitionSetsChanged();
        }
    }
#endif
}

#if WITH_EDITOR
void UMetaWeaverMetadataDefinitionSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (GEditor)
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
        {
            Subsystem->NotifyDefinitionSetsChanged();
        }
    }
}
#endif
