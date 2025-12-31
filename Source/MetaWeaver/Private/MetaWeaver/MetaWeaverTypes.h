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
#include "MetaWeaver/MetaWeaverValueTypes.h"
#include "UObject/SoftObjectPath.h"

/**
 * A tagged value container with canonical string conversion helpers.
 */
struct FMetaWeaverValue
{
    EMetaWeaverValueType Type = EMetaWeaverValueType::String;

    int64 IntValue{ 0 };
    double FloatValue{ 0.0 };
    FString StringValue;
    bool bBoolValue{ false };
    FName EnumValue{ NAME_None };
    FSoftObjectPath AssetPath;

    static FMetaWeaverValue FromInt(const int64 V)
    {
        FMetaWeaverValue Out;
        Out.Type = EMetaWeaverValueType::Integer;
        Out.IntValue = V;
        return Out;
    }

    static FMetaWeaverValue FromFloat(const double V)
    {
        FMetaWeaverValue Out;
        Out.Type = EMetaWeaverValueType::Float;
        Out.FloatValue = V;
        return Out;
    }

    static FMetaWeaverValue FromString(const FString& V)
    {
        FMetaWeaverValue Out;
        Out.Type = EMetaWeaverValueType::String;
        Out.StringValue = V;
        return Out;
    }

    static FMetaWeaverValue FromBool(const bool V)
    {
        FMetaWeaverValue Out;
        Out.Type = EMetaWeaverValueType::Bool;
        Out.bBoolValue = V;
        return Out;
    }

    static FMetaWeaverValue FromEnum(const FName V)
    {
        FMetaWeaverValue Out;
        Out.Type = EMetaWeaverValueType::Enum;
        Out.EnumValue = V;
        return Out;
    }

    static FMetaWeaverValue FromAsset(const FSoftObjectPath& V)
    {
        FMetaWeaverValue Out;
        Out.Type = EMetaWeaverValueType::AssetReference;
        Out.AssetPath = V;
        return Out;
    }

    /** Canonical string form for persistence. */
    FString ToString() const;

    /** Try to parse a string to a typed value of the requested type. */
    static bool TryParse(EMetaWeaverValueType TargetType, const FString& In, FMetaWeaverValue& Out);

    /** Canonicalize a string by parsing as TargetType and re-serializing. */
    static bool Canonicalize(EMetaWeaverValueType TargetType, const FString& In, FString& OutCanonicalValue);
};
