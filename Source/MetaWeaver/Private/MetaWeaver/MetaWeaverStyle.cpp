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
#include "MetaWeaverStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "MetaWeaver/Validation/MetaWeaverValidationTypes.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FMetaWeaverStyle::StyleInstance;

void FMetaWeaverStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FMetaWeaverStyle::Shutdown()
{
    if (StyleInstance.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
        ensure(StyleInstance.IsUnique());
        StyleInstance.Reset();
    }
}

void FMetaWeaverStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

const ISlateStyle& FMetaWeaverStyle::Get()
{
    return *StyleInstance.Get();
}

FName FMetaWeaverStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("MetaWeaverStyle"));
    return StyleSetName;
}

FSlateIcon FMetaWeaverStyle::GetMenuGroupIcon()
{
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Edit");
}

FSlateIcon FMetaWeaverStyle::GetNomadTabIcon()
{
    return GetMenuGroupIcon();
}

const FSlateBrush* FMetaWeaverStyle::GetCheckBrush()
{
    return FAppStyle::Get().GetBrush(TEXT("Icons.Check"));
}

const FSlateBrush* FMetaWeaverStyle::GetSearchBrush()
{
    return FAppStyle::Get().GetBrush(TEXT("Icons.Search"));
}

const FSlateBrush* FMetaWeaverStyle::GetEditBrush()
{
    return FAppStyle::Get().GetBrush(TEXT("Icons.Edit"));
}

const FSlateBrush* FMetaWeaverStyle::GetBrushForAsset(const UObject* Asset)
{
    const FSlateIcon Icon = FSlateIconFinder::FindIconForClass(Asset->GetClass());
    if (const auto Found = Icon.GetIcon())
    {
        return Found;
    }
    else
    {
        return GetDocumentBrush();
    }
}

const FButtonStyle& FMetaWeaverStyle::GetButtonStyle()
{
    return FAppStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("SimpleButton"));
}

const FSlateBrush* FMetaWeaverStyle::GetDeleteBrush()
{
    return FAppStyle::Get().GetBrush("Icons.Delete");
}

const FSlateBrush* FMetaWeaverStyle::GetBorderImageBrush()
{
    return FAppStyle::Get().GetBrush(TEXT("DetailsView.CategoryTop"));
}

const FSlateBrush* FMetaWeaverStyle::GetDocumentBrush()
{
    return FAppStyle::Get().GetBrush(TEXT("Icons.Documentation"));
}

const FSlateBrush* FMetaWeaverStyle::GetResetToDefaultBrush()
{
    return FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault");
}

const FSlateBrush* FMetaWeaverStyle::GetLockBrush(bool bLocked)
{
    return FAppStyle::Get().GetBrush(bLocked ? TEXT("Icons.Lock") : TEXT("Icons.Unlock"));
}

const FSlateBrush* FMetaWeaverStyle::GetBrushForIssueSeverity(const EMetaWeaverIssueSeverity IssueSeverity)
{
    const auto& SlateStyle = FAppStyle::Get();
    if (EMetaWeaverIssueSeverity::Error == IssueSeverity)
    {
        return SlateStyle.GetBrush("Icons.Error");
    }
    else if (EMetaWeaverIssueSeverity::Warning == IssueSeverity)
    {
        return SlateStyle.GetBrush("Icons.WarningWithColor");
    }
    else
    {
        return SlateStyle.GetBrush("Icons.Info");
    }
}

FLinearColor FMetaWeaverStyle::GetErrorColor()
{
    return FLinearColor(0.85f, 0.2f, 0.2f);
}

FLinearColor FMetaWeaverStyle::GetWarningColor()
{
    return FLinearColor(0.9f, 0.7f, 0.1f);
}

FLinearColor FMetaWeaverStyle::GetInfoColor()
{
    return FLinearColor(0.3f, 0.3f, 0.9f);
}

FLinearColor FMetaWeaverStyle::GetColorForIssueSeverity(EMetaWeaverIssueSeverity IssueSeverity)
{
    return EMetaWeaverIssueSeverity::Error == IssueSeverity  ? GetErrorColor()
        : EMetaWeaverIssueSeverity::Warning == IssueSeverity ? GetWarningColor()
                                                             : GetInfoColor();
}

TSharedRef<FSlateStyleSet> FMetaWeaverStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>(GetStyleSetName());

    // Set content root to the plugin's Resources folder if present
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("MetaWeaver"));
    if (Plugin.IsValid())
    {
        Style->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
    }

    return Style;
}
