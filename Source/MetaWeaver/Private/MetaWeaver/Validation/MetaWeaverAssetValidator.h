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
#include "EditorValidatorBase.h"
#include "MetaWeaverAssetValidator.generated.h"

/**
 * Validates asset metadata using active MetaWeaver definition sets.
 */
UCLASS()
class UMetaWeaverAssetValidator final : public UEditorValidatorBase
{
    GENERATED_BODY()

public:
    UMetaWeaverAssetValidator();

    virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData,
                                                 UObject* InObject,
                                                 FDataValidationContext& InContext) const override;
    virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData,
                                                                     UObject* InAsset,
                                                                     FDataValidationContext& Context) override;
};
