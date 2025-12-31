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
#include "MetaWeaverTypes.h"
#include "String/LexFromString.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MetaWeaverValueTypes)

static bool ParseInt64(const FString& In, int64& Out)
{
    return LexTryParseString(Out, *In);
}

static bool ParseDouble(const FString& In, double& Out)
{
    return LexTryParseString(Out, *In);
}

FString FMetaWeaverValue::ToString() const
{
    switch (Type)
    {
        case EMetaWeaverValueType::Integer:
            return LexToString(IntValue);
        case EMetaWeaverValueType::Float:
            return LexToString(FloatValue);
        case EMetaWeaverValueType::String:
            return StringValue;
        case EMetaWeaverValueType::Bool:
            return bBoolValue ? TEXT("True") : TEXT("False");
        case EMetaWeaverValueType::Enum:
            return EnumValue.ToString();
        case EMetaWeaverValueType::AssetReference:
            return AssetPath.ToString();
        default:
            return FString();
    }
}

bool FMetaWeaverValue::TryParse(const EMetaWeaverValueType TargetType, const FString& In, FMetaWeaverValue& Out)
{
    switch (TargetType)
    {
        case EMetaWeaverValueType::Integer:
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            int64 V{ 0 };
            if (!ParseInt64(In, V))
            {
                return false;
            }
            else
            {
                Out = FromInt(V);
                return true;
            }
        }
        case EMetaWeaverValueType::Float:
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            double V{ 0.f };
            if (!ParseDouble(In, V))
            {
                return false;
            }
            else
            {
                Out = FromFloat(V);
                return true;
            }
        }
        case EMetaWeaverValueType::String:
            Out = FromString(In);
            return true;
        case EMetaWeaverValueType::Bool:
        {
            const auto Lower = In.ToLower();
            if (TEXT("true") == Lower || TEXT("1") == Lower)
            {
                Out = FromBool(true);
                return true;
            }
            else if (TEXT("false") == Lower || TEXT("0") == Lower)
            {
                Out = FromBool(false);
                return true;
            }
            else
            {
                return false;
            }
        }
        case EMetaWeaverValueType::Enum:
            Out = FromEnum(FName(*In));
            return true;
        case EMetaWeaverValueType::AssetReference:
            Out = FromAsset(FSoftObjectPath(In));
            return true;
        default:
            return false;
    }
}

bool FMetaWeaverValue::Canonicalize(const EMetaWeaverValueType TargetType,
                                    const FString& In,
                                    FString& OutCanonicalValue)
{
    // ReSharper disable once CppTooWideScopeInitStatement
    FMetaWeaverValue Tmp;
    if (!TryParse(TargetType, In, Tmp))
    {
        return false;
    }
    else
    {
        OutCanonicalValue = Tmp.ToString();
        return true;
    }
}
