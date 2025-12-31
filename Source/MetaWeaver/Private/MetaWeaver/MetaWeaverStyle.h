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
#include "Styling/SlateStyle.h"

enum class EMetaWeaverIssueSeverity : uint8;

/**
 * MetaWeaver style set. Icons are optional at this stage; this provides a
 * stable style name for commands and UI elements.
 */
class FMetaWeaverStyle final
{
public:
    static void Initialize();
    static void Shutdown();

    static void ReloadTextures();

    static const ISlateStyle& Get();
    static FName GetStyleSetName();

    static FSlateIcon GetMenuGroupIcon();
    static FSlateIcon GetNomadTabIcon();

    static const FSlateBrush* GetCheckBrush();
    static const FSlateBrush* GetSearchBrush();
    static const FSlateBrush* GetEditBrush();
    static const FSlateBrush* GetDeleteBrush();
    static const FSlateBrush* GetBorderImageBrush();
    static const FSlateBrush* GetDocumentBrush();
    static const FSlateBrush* GetResetToDefaultBrush();
    static const FSlateBrush* GetLockBrush(bool bLocked);
    static const FSlateBrush* GetBrushForIssueSeverity(EMetaWeaverIssueSeverity IssueSeverity);
    static const FSlateBrush* GetBrushForAsset(const UObject* Asset);

    static const FButtonStyle& GetButtonStyle();

    static FLinearColor GetErrorColor();
    static FLinearColor GetWarningColor();
    static FLinearColor GetInfoColor();
    static FLinearColor GetColorForIssueSeverity(EMetaWeaverIssueSeverity IssueSeverity);

private:
    static TSharedRef<FSlateStyleSet> Create();

    static TSharedPtr<FSlateStyleSet> StyleInstance;
};
