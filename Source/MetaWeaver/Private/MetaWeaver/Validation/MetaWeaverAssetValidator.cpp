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
#include "MetaWeaver/Validation/MetaWeaverAssetValidator.h"
#include "Editor.h"
#include "MetaWeaver/MetaWeaverLogging.h"
#include "MetaWeaver/Validation/MetaWeaverValidationSubsystem.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MetaWeaverAssetValidator)

#define LOCTEXT_NAMESPACE "MetaWeaverAssetValidator"

UMetaWeaverAssetValidator::UMetaWeaverAssetValidator()
{
    bIsEnabled = true;
}

bool UMetaWeaverAssetValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData,
                                                                UObject* InObject,
                                                                FDataValidationContext& InContext) const
{
    return nullptr != InObject;
}

EDataValidationResult UMetaWeaverAssetValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData,
                                                                                    UObject* InAsset,
                                                                                    FDataValidationContext& Context)
{
    if (!InAsset || !GEditor)
    {
        return EDataValidationResult::NotValidated;
    }
    else
    {
        if (const auto Subsystem = GEditor->GetEditorSubsystem<UMetaWeaverValidationSubsystem>())
        {
            const auto Report = Subsystem->ValidateAsset(InAsset);
            auto bHasErrors = Report.bHasErrors;

            for (const auto& Issue : Report.Issues)
            {
                const auto Message = Issue.Key.IsNone()
                    ? Issue.Message
                    : FText::Format(LOCTEXT("MetaWeaverIssueWithKey", "Metadata '{0}': {1}"),
                                    FText::FromName(Issue.Key),
                                    Issue.Message);
                if (EMetaWeaverIssueSeverity::Error == Issue.Severity)
                {
                    Context.AddError(Message);
                    bHasErrors = true;
                }
                else if (EMetaWeaverIssueSeverity::Info == Issue.Severity)
                {
                    Context.AddWarning(Message);
                }
                else
                {
                    check(EMetaWeaverIssueSeverity::Warning == Issue.Severity);
                    Context.AddWarning(Message);
                }
            }

            return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
        }
        else
        {
            UE_LOG(LogMetaWeaver, Warning, TEXT("MetaWeaver validation subsystem is unavailable."));
            return EDataValidationResult::Valid;
        }
    }
}

#undef LOCTEXT_NAMESPACE
