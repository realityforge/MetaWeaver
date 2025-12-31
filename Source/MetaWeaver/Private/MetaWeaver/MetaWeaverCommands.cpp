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
#include "MetaWeaverCommands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "MetaWeaverStyle.h"

#define LOCTEXT_NAMESPACE "FMetaWeaverCommands"

FMetaWeaverCommands::FMetaWeaverCommands()
    : TCommands(TEXT("MetaWeaver"),
                NSLOCTEXT("MetaWeaver", "MetaWeaverCommands", "MetaWeaver"),
                NAME_None,
                FMetaWeaverStyle::GetStyleSetName())
{
}

void FMetaWeaverCommands::RegisterCommands()
{
    UI_COMMAND(OpenMetaWeaver,
               "Open MetaWeaver",
               "Open the MetaWeaver editor",
               EUserInterfaceActionType::Button,
               FInputChord());
}

#undef LOCTEXT_NAMESPACE
