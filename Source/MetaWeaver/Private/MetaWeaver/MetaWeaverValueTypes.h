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
#include "MetaWeaverValueTypes.generated.h"

/**
 * Public enum for MetaWeaver value types (Editor-visible).
 */
UENUM(BlueprintType)
enum class EMetaWeaverValueType : uint8
{
    Integer UMETA(DisplayName = "Integer"),
    Float UMETA(DisplayName = "Float"),
    String UMETA(DisplayName = "String"),
    Bool UMETA(DisplayName = "Bool"),
    Enum UMETA(DisplayName = "Enum"),
    AssetReference UMETA(DisplayName = "Asset Reference")
};
