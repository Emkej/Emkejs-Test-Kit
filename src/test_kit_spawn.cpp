#include "test_kit_spawn.h"
#include "test_kit_spawn_runtime.h"

#include <core/Functions.h>
#include <kenshi/Faction.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/SensoryData.h>
#include <mygui/MyGUI_InputManager.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace test_kit
{
const int kSpawnTemplateQuantityMax = 50;

namespace
{
const float kSpawnTemplateBaseRadius = 180.0f;
const float kSpawnTemplateRingSpacing = 120.0f;
const int kSpawnTemplateFirstRingSlots = 6;
const int kSpawnTemplateMaxPlacementAttemptsPerUnit = 4;
const float kSpawnTemplateMaxResolvedDrift = 500.0f;
const float kSpawnTemplateCloseRadiusMultiplier = 0.55f;
const float kSpawnTemplateWideRadiusMultiplier = 1.8f;
const float kPi = 3.14159265358979323846f;

std::vector<SpawnTemplateOption> g_spawnTemplateOptions;
std::vector<size_t> g_filteredSpawnTemplateOptionIndexes;
bool g_spawnSelectionSyncInProgress = false;
bool g_spawnTemplateOptionsLoaded = false;
}

bool TryResolveSelectedSpawnTemplate(GameData** templateDataOut, std::string* displayNameOut, std::string* summaryLabelOut);
bool TryGetSpawnTemplateQuantity(int* outQuantity);
bool IsSpawnTemplateModeAllowed(SpawnTemplateSquadMode squadMode, SpawnTemplateAllegiance allegiance);
const char* GetSpawnTemplateModeRestrictionMessage(SpawnTemplateSquadMode squadMode, SpawnTemplateAllegiance allegiance);
const char* SpawnTemplateAllegianceToLabel(SpawnTemplateAllegiance allegiance);
const char* SpawnTemplateRadiusPresetToLabel(SpawnTemplateRadiusPreset radiusPreset);
const char* SpawnTemplateSquadModeToLabel(SpawnTemplateSquadMode squadMode);
const char* SpawnCreatureAgePresetToLabel(SpawnCreatureAgePreset agePreset);
bool IsCreatureSpawnTemplateData(GameData* templateData);
SpawnTemplateAllegiance GetSelectedSpawnTemplateAllegiance();
SpawnTemplateRadiusPreset GetSelectedSpawnTemplateRadiusPreset();
SpawnTemplateSquadMode GetSelectedSpawnTemplateSquadMode();
SpawnCreatureAgePreset GetSelectedSpawnCreatureAgePreset();
bool TryResolveAndValidateSpawnTemplateFactionSelection(
    Character* target,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    SpawnTemplateFactionSelection* outSelection,
    std::string* outMessage);

const char* SpawnTemplateCategoryToTypeLabel(SpawnTemplateCategory category)
{
    switch (category)
    {
    case SpawnTemplateCategory_Characters:
        return "Character";
    case SpawnTemplateCategory_Creatures:
        return "Creature";
    case SpawnTemplateCategory_All:
    default:
        return "Template";
    }
}

SpawnTemplateCategory GetSelectedSpawnTemplateCategory()
{
    if (!g_spawnCategoryDropdown)
    {
        return SpawnTemplateCategory_All;
    }

    switch (g_spawnCategoryDropdown->getIndexSelected())
    {
    case 1:
        return SpawnTemplateCategory_Characters;
    case 2:
        return SpawnTemplateCategory_Creatures;
    default:
        return SpawnTemplateCategory_All;
    }
}

const char* SpawnTemplateRadiusPresetToLabel(SpawnTemplateRadiusPreset radiusPreset)
{
    switch (radiusPreset)
    {
    case SpawnTemplateRadius_Close:
        return "Close";
    case SpawnTemplateRadius_Wide:
        return "Wide";
    case SpawnTemplateRadius_Normal:
    default:
        return "Normal";
    }
}

SpawnTemplateRadiusPreset GetSelectedSpawnTemplateRadiusPreset()
{
    if (!g_spawnRadiusDropdown)
    {
        return SpawnTemplateRadius_Normal;
    }

    switch (g_spawnRadiusDropdown->getIndexSelected())
    {
    case 0:
        return SpawnTemplateRadius_Close;
    case 2:
        return SpawnTemplateRadius_Wide;
    case 1:
    default:
        return SpawnTemplateRadius_Normal;
    }
}

const char* SpawnTemplateAllegianceToLabel(SpawnTemplateAllegiance allegiance)
{
    switch (allegiance)
    {
    case SpawnTemplateAllegiance_FriendlyPlayer:
        return "Friendly (player)";
    case SpawnTemplateAllegiance_Neutral:
        return "Neutral";
    case SpawnTemplateAllegiance_Hostile:
        return "Hostile";
    case SpawnTemplateAllegiance_SameAsTarget:
    default:
        return "Same as target";
    }
}

SpawnTemplateAllegiance GetSelectedSpawnTemplateAllegiance()
{
    if (!g_spawnAllegianceDropdown)
    {
        return SpawnTemplateAllegiance_SameAsTarget;
    }

    switch (g_spawnAllegianceDropdown->getIndexSelected())
    {
    case 1:
        return SpawnTemplateAllegiance_FriendlyPlayer;
    case 2:
        return SpawnTemplateAllegiance_Neutral;
    case 3:
        return SpawnTemplateAllegiance_Hostile;
    case 0:
    default:
        return SpawnTemplateAllegiance_SameAsTarget;
    }
}

const char* SpawnTemplateSquadModeToLabel(SpawnTemplateSquadMode squadMode)
{
    switch (squadMode)
    {
    case SpawnTemplateSquadMode_AddToTargetSquad:
        return "Add to target squad";
    case SpawnTemplateSquadMode_SeparateSquad:
    default:
        return "Independent";
    }
}

const char* SpawnCreatureAgePresetToLabel(SpawnCreatureAgePreset agePreset)
{
    switch (agePreset)
    {
    case SpawnCreatureAge_Pup:
        return "Pup";
    case SpawnCreatureAge_Young:
        return "Young";
    case SpawnCreatureAge_Adult:
    default:
        return "Adult";
    }
}

SpawnCreatureAgePreset GetSelectedSpawnCreatureAgePreset()
{
    if (!g_spawnCreatureAgeDropdown)
    {
        return SpawnCreatureAge_Adult;
    }

    switch (g_spawnCreatureAgeDropdown->getIndexSelected())
    {
    case 0:
        return SpawnCreatureAge_Pup;
    case 1:
        return SpawnCreatureAge_Young;
    case 2:
    default:
        return SpawnCreatureAge_Adult;
    }
}

float ResolveSpawnCreatureAge0To1(SpawnCreatureAgePreset agePreset)
{
    switch (agePreset)
    {
    case SpawnCreatureAge_Pup:
        return 0.0f;
    case SpawnCreatureAge_Young:
        return 0.5f;
    case SpawnCreatureAge_Adult:
    default:
        return 1.0f;
    }
}

SpawnTemplateSquadMode GetSelectedSpawnTemplateSquadMode()
{
    if (!g_spawnModeDropdown)
    {
        return SpawnTemplateSquadMode_SeparateSquad;
    }

    switch (g_spawnModeDropdown->getIndexSelected())
    {
    case 1:
        return SpawnTemplateSquadMode_AddToTargetSquad;
    case 0:
    default:
        return SpawnTemplateSquadMode_SeparateSquad;
    }
}

bool TryResolveAndValidateSpawnTemplateFactionSelection(
    Character* target,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    SpawnTemplateFactionSelection* outSelection,
    std::string* outMessage)
{
    if (!TryResolveSelectedSpawnTemplateFactionSelection(outSelection, outMessage))
    {
        return false;
    }

    if (!target)
    {
        return true;
    }

    return TryGetSpawnTemplateFactionRestrictionMessage(
        target,
        squadMode,
        allegiance,
        outSelection ? *outSelection : SpawnTemplateFactionSelection(),
        outMessage);
}

bool IsCreatureSpawnTemplateData(GameData* templateData)
{
    return templateData != 0 && templateData->type == ANIMAL_CHARACTER;
}

bool ShouldEnableSpawnCreatureAgeControls()
{
    if (GetSelectedSpawnTemplateCategory() == SpawnTemplateCategory_Creatures)
    {
        return true;
    }

    GameData* templateData = 0;
    return TryResolveSelectedSpawnTemplate(&templateData, 0, 0) && IsCreatureSpawnTemplateData(templateData);
}

void RefreshSpawnCreatureAgeControlState()
{
    const bool enabled = ShouldEnableSpawnCreatureAgeControls();
    if (g_spawnCreatureAgeLabelText)
    {
        g_spawnCreatureAgeLabelText->setEnabled(enabled);
    }
    if (g_spawnCreatureAgeDropdown)
    {
        g_spawnCreatureAgeDropdown->setEnabled(enabled);
    }
}

size_t GetSpawnTemplateAllegianceIndex(SpawnTemplateAllegiance allegiance)
{
    switch (allegiance)
    {
    case SpawnTemplateAllegiance_FriendlyPlayer:
        return 1u;
    case SpawnTemplateAllegiance_Neutral:
        return 2u;
    case SpawnTemplateAllegiance_Hostile:
        return 3u;
    case SpawnTemplateAllegiance_SameAsTarget:
    default:
        return 0u;
    }
}

size_t GetSpawnTemplateSquadModeIndex(SpawnTemplateSquadMode squadMode)
{
    return squadMode == SpawnTemplateSquadMode_AddToTargetSquad ? 1u : 0u;
}

void SetSelectedSpawnTemplateAllegiance(SpawnTemplateAllegiance allegiance)
{
    if (!g_spawnAllegianceDropdown)
    {
        return;
    }

    const size_t index = GetSpawnTemplateAllegianceIndex(allegiance);
    if (g_spawnAllegianceDropdown->getIndexSelected() != index)
    {
        g_spawnAllegianceDropdown->setIndexSelected(index);
    }
}

void SetSelectedSpawnTemplateSquadMode(SpawnTemplateSquadMode squadMode)
{
    if (!g_spawnModeDropdown)
    {
        return;
    }

    const size_t index = GetSpawnTemplateSquadModeIndex(squadMode);
    if (g_spawnModeDropdown->getIndexSelected() != index)
    {
        g_spawnModeDropdown->setIndexSelected(index);
    }
}

bool TryAutoAdjustSpawnSelectionForAllegianceChange(std::string* outMessage)
{
    if (outMessage)
    {
        outMessage->clear();
    }

    const SpawnTemplateAllegiance allegiance = GetSelectedSpawnTemplateAllegiance();
    const SpawnTemplateSquadMode squadMode = GetSelectedSpawnTemplateSquadMode();

    if (allegiance == SpawnTemplateAllegiance_SameAsTarget || allegiance == SpawnTemplateAllegiance_FriendlyPlayer)
    {
        if (squadMode != SpawnTemplateSquadMode_AddToTargetSquad)
        {
            SetSelectedSpawnTemplateSquadMode(SpawnTemplateSquadMode_AddToTargetSquad);
            if (outMessage)
            {
                *outMessage =
                    std::string("Mode adjusted to Add to target squad for ")
                    + SpawnTemplateAllegianceToLabel(allegiance);
            }
            return true;
        }

        return false;
    }

    if (squadMode != SpawnTemplateSquadMode_SeparateSquad)
    {
        SetSelectedSpawnTemplateSquadMode(SpawnTemplateSquadMode_SeparateSquad);
        if (outMessage)
        {
            *outMessage =
                std::string("Mode adjusted to Independent for ")
                + SpawnTemplateAllegianceToLabel(allegiance);
        }
        return true;
    }

    return false;
}

bool TryAutoAdjustSpawnSelectionForModeChange(std::string* outMessage)
{
    if (outMessage)
    {
        outMessage->clear();
    }

    const SpawnTemplateAllegiance allegiance = GetSelectedSpawnTemplateAllegiance();
    const SpawnTemplateSquadMode squadMode = GetSelectedSpawnTemplateSquadMode();

    if (squadMode == SpawnTemplateSquadMode_SeparateSquad)
    {
        if (allegiance == SpawnTemplateAllegiance_SameAsTarget
            || allegiance == SpawnTemplateAllegiance_FriendlyPlayer)
        {
            SetSelectedSpawnTemplateAllegiance(SpawnTemplateAllegiance_Neutral);
            if (outMessage)
            {
                *outMessage = "Allegiance adjusted to Neutral for Independent mode";
            }
            return true;
        }

        return false;
    }

    if (allegiance == SpawnTemplateAllegiance_Neutral || allegiance == SpawnTemplateAllegiance_Hostile)
    {
        SetSelectedSpawnTemplateAllegiance(SpawnTemplateAllegiance_SameAsTarget);
        if (outMessage)
        {
            *outMessage = "Allegiance adjusted to Same as target for Add to target squad";
        }
        return true;
    }

    return false;
}

std::string BuildSpawnTemplateDisplayName(GameData* templateData)
{
    if (!templateData)
    {
        return "";
    }

    const std::string name = TrimAscii(templateData->name);
    if (!name.empty())
    {
        return name;
    }

    const std::string stringId = TrimAscii(templateData->stringID);
    if (!stringId.empty())
    {
        return stringId;
    }

    std::stringstream fallback;
    fallback << SpawnTemplateCategoryToTypeLabel(
        templateData->type == ANIMAL_CHARACTER ? SpawnTemplateCategory_Creatures : SpawnTemplateCategory_Characters)
             << " " << templateData->id;
    return fallback.str();
}

std::string BuildSpawnTemplateSearchText(
    GameData* templateData,
    const std::string& displayName,
    const char* typeLabel)
{
    std::string searchTextUpper = ToUpperAscii(displayName);
    if (typeLabel && *typeLabel)
    {
        searchTextUpper += " ";
        searchTextUpper += ToUpperAscii(typeLabel);
    }

    if (!templateData)
    {
        return searchTextUpper;
    }

    const std::string stringId = TrimAscii(templateData->stringID);
    if (!stringId.empty())
    {
        searchTextUpper += " ";
        searchTextUpper += ToUpperAscii(stringId);
    }

    return searchTextUpper;
}

bool DoesSpawnTemplateMatchCategory(const SpawnTemplateOption& option, SpawnTemplateCategory category)
{
    return category == SpawnTemplateCategory_All || option.category == category;
}

bool DoesSpawnTemplateMatchSearch(const SpawnTemplateOption& option, const std::string& searchUpper)
{
    return searchUpper.empty() || option.searchTextUpper.find(searchUpper) != std::string::npos;
}

bool HasSpawnTemplateOptionForData(GameData* templateData)
{
    if (!templateData)
    {
        return false;
    }

    for (size_t index = 0; index < g_spawnTemplateOptions.size(); ++index)
    {
        if (g_spawnTemplateOptions[index].templateData == templateData)
        {
            return true;
        }
    }

    return false;
}

void AddSpawnTemplateOptionsForType(itemType dataType, SpawnTemplateCategory category)
{
    if (!ou || !ou->initialized)
    {
        return;
    }

    lektor<GameData*> templateDatas;
    ou->gamedata.getDataOfType(templateDatas, dataType);

    for (lektor<GameData*>::const_iterator it = templateDatas.begin(); it != templateDatas.end(); ++it)
    {
        GameData* templateData = *it;
        if (!templateData || !templateData->isValid() || HasSpawnTemplateOptionForData(templateData))
        {
            continue;
        }

        SpawnTemplateOption option;
        option.displayName = BuildSpawnTemplateDisplayName(templateData);
        option.category = category;
        option.templateData = templateData;
        option.listLabel = option.displayName + " (" + SpawnTemplateCategoryToTypeLabel(category) + ")";
        option.summaryLabel = option.displayName + " · " + SpawnTemplateCategoryToTypeLabel(category);
        option.searchTextUpper = BuildSpawnTemplateSearchText(
            templateData,
            option.displayName,
            SpawnTemplateCategoryToTypeLabel(category));
        g_spawnTemplateOptions.push_back(option);
    }
}

bool TryResolveSelectedSpawnTemplate(GameData** templateDataOut, std::string* displayNameOut, std::string* summaryLabelOut)
{
    if (templateDataOut)
    {
        *templateDataOut = 0;
    }
    if (displayNameOut)
    {
        displayNameOut->clear();
    }
    if (summaryLabelOut)
    {
        summaryLabelOut->clear();
    }

    if (!g_spawnResultsList || g_filteredSpawnTemplateOptionIndexes.empty())
    {
        return false;
    }

    const size_t selectedIndex = g_spawnResultsList->getIndexSelected();
    if (selectedIndex >= g_filteredSpawnTemplateOptionIndexes.size())
    {
        return false;
    }

    const SpawnTemplateOption& option =
        g_spawnTemplateOptions[g_filteredSpawnTemplateOptionIndexes[selectedIndex]];
    if (!option.templateData)
    {
        return false;
    }

    if (templateDataOut)
    {
        *templateDataOut = option.templateData;
    }
    if (displayNameOut)
    {
        *displayNameOut = option.displayName;
    }
    if (summaryLabelOut)
    {
        *summaryLabelOut = option.summaryLabel;
    }

    return true;
}

bool TryGetSpawnTemplateQuantity(int* outQuantity)
{
    if (!outQuantity || !g_spawnQuantityEdit)
    {
        return false;
    }

    int quantity = 0;
    if (!TryParsePositiveInt(TrimAscii(g_spawnQuantityEdit->getOnlyText().asUTF8()), &quantity))
    {
        return false;
    }

    if (quantity < 1 || quantity > kSpawnTemplateQuantityMax)
    {
        return false;
    }

    *outQuantity = quantity;
    return true;
}

void RefreshSpawnTemplateList()
{
    if (!g_spawnResultsList)
    {
        g_filteredSpawnTemplateOptionIndexes.clear();
        if (g_spawnResultCountText)
        {
            g_spawnResultCountText->setCaption("0 results");
        }
        return;
    }

    GameData* previouslySelectedTemplate = 0;
    const size_t previousSelectedIndex = g_spawnResultsList->getIndexSelected();
    if (previousSelectedIndex < g_filteredSpawnTemplateOptionIndexes.size())
    {
        previouslySelectedTemplate =
            g_spawnTemplateOptions[g_filteredSpawnTemplateOptionIndexes[previousSelectedIndex]].templateData;
    }

    g_spawnResultsList->removeAllItems();
    g_filteredSpawnTemplateOptionIndexes.clear();

    std::string searchUpper;
    if (g_spawnSearchEdit)
    {
        searchUpper = ToUpperAscii(TrimAscii(g_spawnSearchEdit->getOnlyText().asUTF8()));
    }
    const SpawnTemplateCategory category = GetSelectedSpawnTemplateCategory();

    for (size_t index = 0; index < g_spawnTemplateOptions.size(); ++index)
    {
        const SpawnTemplateOption& option = g_spawnTemplateOptions[index];
        if (!DoesSpawnTemplateMatchCategory(option, category)
            || !DoesSpawnTemplateMatchSearch(option, searchUpper))
        {
            continue;
        }

        g_filteredSpawnTemplateOptionIndexes.push_back(index);
        g_spawnResultsList->addItem(option.listLabel);
    }

    if (g_spawnResultCountText)
    {
        if (!g_spawnTemplateOptionsLoaded)
        {
            g_spawnResultCountText->setCaption("Loading...");
        }
        else
        {
            std::stringstream caption;
            caption << g_filteredSpawnTemplateOptionIndexes.size() << " results";
            g_spawnResultCountText->setCaption(caption.str());
        }
    }

    if (g_filteredSpawnTemplateOptionIndexes.empty())
    {
        if (!g_spawnTemplateOptionsLoaded)
        {
            g_spawnResultsList->addItem("Loading spawn templates...");
        }
        else if (g_spawnTemplateOptions.empty())
        {
            g_spawnResultsList->addItem("No spawn templates available");
        }
        else
        {
            g_spawnResultsList->addItem("No matching spawn templates");
        }

        g_spawnResultsList->clearIndexSelected();
        g_spawnResultsList->beginToItemFirst();
        RefreshSpawnCreatureAgeControlState();
        RefreshSpawnButtonState();
        RefreshSpawnPreviewText();
        return;
    }

    size_t nextSelectedIndex = MyGUI::ITEM_NONE;
    if (previouslySelectedTemplate)
    {
        for (size_t filteredIndex = 0; filteredIndex < g_filteredSpawnTemplateOptionIndexes.size(); ++filteredIndex)
        {
            if (g_spawnTemplateOptions[g_filteredSpawnTemplateOptionIndexes[filteredIndex]].templateData
                == previouslySelectedTemplate)
            {
                nextSelectedIndex = filteredIndex;
                break;
            }
        }
    }

    if (nextSelectedIndex == MyGUI::ITEM_NONE && g_filteredSpawnTemplateOptionIndexes.size() == 1u)
    {
        nextSelectedIndex = 0u;
    }

    if (nextSelectedIndex != MyGUI::ITEM_NONE)
    {
        g_spawnResultsList->setIndexSelected(nextSelectedIndex);
        g_spawnResultsList->beginToItemSelected();
    }
    else
    {
        g_spawnResultsList->clearIndexSelected();
        g_spawnResultsList->beginToItemFirst();
    }

    RefreshSpawnCreatureAgeControlState();
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void EnsureSpawnTemplateOptionsLoaded()
{
    if (g_spawnTemplateOptionsLoaded || !ou || !ou->initialized)
    {
        return;
    }

    g_spawnTemplateOptions.clear();
    AddSpawnTemplateOptionsForType(CHARACTER, SpawnTemplateCategory_Characters);
    AddSpawnTemplateOptionsForType(HUMAN_CHARACTER, SpawnTemplateCategory_Characters);
    AddSpawnTemplateOptionsForType(ANIMAL_CHARACTER, SpawnTemplateCategory_Creatures);

    std::sort(
        g_spawnTemplateOptions.begin(),
        g_spawnTemplateOptions.end(),
        [](const SpawnTemplateOption& left, const SpawnTemplateOption& right) -> bool
        {
            if (left.displayName != right.displayName)
            {
                return left.displayName < right.displayName;
            }

            return left.listLabel < right.listLabel;
        });

    g_spawnTemplateOptionsLoaded = true;
    RefreshSpawnTemplateList();
}

bool IsSpawnTemplateModeAllowed(SpawnTemplateSquadMode squadMode, SpawnTemplateAllegiance allegiance)
{
    if (squadMode == SpawnTemplateSquadMode_AddToTargetSquad)
    {
        return allegiance != SpawnTemplateAllegiance_Hostile;
    }

    return allegiance != SpawnTemplateAllegiance_SameAsTarget
        && allegiance != SpawnTemplateAllegiance_FriendlyPlayer;
}

const char* GetSpawnTemplateModeRestrictionMessage(SpawnTemplateSquadMode squadMode, SpawnTemplateAllegiance allegiance)
{
    if (squadMode == SpawnTemplateSquadMode_AddToTargetSquad && allegiance == SpawnTemplateAllegiance_Hostile)
    {
        return "Hostile can't add to target squad";
    }

    if (squadMode == SpawnTemplateSquadMode_SeparateSquad)
    {
        switch (allegiance)
        {
        case SpawnTemplateAllegiance_SameAsTarget:
        case SpawnTemplateAllegiance_FriendlyPlayer:
            return "This allegiance requires Add to target squad";
        default:
            break;
        }
    }

    return "Selected mode is unavailable";
}

void RefreshSpawnPreviewText()
{
    if (!g_spawnPreviewText || !g_spawnSelectedSummaryText)
    {
        return;
    }

    GameData* templateData = 0;
    std::string displayName;
    std::string summaryLabel;
    const bool hasSelectedTemplate = TryResolveSelectedSpawnTemplate(&templateData, &displayName, &summaryLabel);
    if (hasSelectedTemplate)
    {
        g_spawnSelectedSummaryText->setCaption("Selected: " + summaryLabel);
    }
    else
    {
        g_spawnSelectedSummaryText->setCaption("Selected: None");
    }

    const bool hasTarget =
        g_hasLastTargetSnapshot
        && g_lastTargetSnapshot.hasTarget
        && g_lastTargetSnapshot.target != 0;
    int quantity = 0;
    const bool hasValidQuantity = TryGetSpawnTemplateQuantity(&quantity);

    if (!hasSelectedTemplate)
    {
        g_spawnPreviewText->setCaption("Preview: Select a spawn template");
        return;
    }

    if (!hasValidQuantity)
    {
        std::stringstream preview;
        preview << "Preview: Enter a quantity from 1 to " << kSpawnTemplateQuantityMax;
        g_spawnPreviewText->setCaption(preview.str());
        return;
    }

    const SpawnTemplateAllegiance allegiance = GetSelectedSpawnTemplateAllegiance();
    const SpawnTemplateSquadMode squadMode = GetSelectedSpawnTemplateSquadMode();
    SpawnTemplateFactionSelection factionSelection;
    std::string factionMessage;
    if (!TryResolveSelectedSpawnTemplateFactionSelection(&factionSelection, &factionMessage))
    {
        g_spawnPreviewText->setCaption(std::string("Preview: ") + factionMessage);
        return;
    }

    if (!IsSpawnTemplateModeAllowed(squadMode, allegiance))
    {
        g_spawnPreviewText->setCaption(
            std::string("Preview: ") + GetSpawnTemplateModeRestrictionMessage(squadMode, allegiance));
        return;
    }

    if (!hasTarget)
    {
        g_spawnPreviewText->setCaption("Preview: Select a target to place " + displayName);
        return;
    }

    if (!TryGetSpawnTemplateFactionRestrictionMessage(
            g_lastTargetSnapshot.target,
            squadMode,
            allegiance,
            factionSelection,
            &factionMessage))
    {
        g_spawnPreviewText->setCaption(std::string("Preview: ") + factionMessage);
        return;
    }

    const SpawnTemplateRadiusPreset radiusPreset = GetSelectedSpawnTemplateRadiusPreset();
    const bool isCreatureTemplate = IsCreatureSpawnTemplateData(templateData);
    const SpawnCreatureAgePreset creatureAgePreset = GetSelectedSpawnCreatureAgePreset();
    std::stringstream preview;
    preview << "Preview: Spawn " << quantity << "x " << displayName
            << " near " << g_lastTargetSnapshot.name;
    if (isCreatureTemplate)
    {
        preview << ", age: " << SpawnCreatureAgePresetToLabel(creatureAgePreset);
    }
    preview
            << ", faction: " << DescribeSpawnTemplateFactionSelection(factionSelection)
            << ", allegiance: " << SpawnTemplateAllegianceToLabel(allegiance)
            << ", radius: " << SpawnTemplateRadiusPresetToLabel(radiusPreset)
            << ", mode: " << SpawnTemplateSquadModeToLabel(squadMode);
    g_spawnPreviewText->setCaption(preview.str());
}

void RefreshSpawnButtonState()
{
    if (!g_spawnCharactersButton)
    {
        return;
    }

    const bool hasTarget =
        g_hasLastTargetSnapshot
        && g_lastTargetSnapshot.hasTarget
        && g_lastTargetSnapshot.target != 0;
    GameData* templateData = 0;
    int quantity = 0;
    const SpawnTemplateAllegiance allegiance = GetSelectedSpawnTemplateAllegiance();
    const SpawnTemplateSquadMode squadMode = GetSelectedSpawnTemplateSquadMode();
    SpawnTemplateFactionSelection factionSelection;
    std::string factionMessage;
    const bool hasSelectedTemplate = TryResolveSelectedSpawnTemplate(&templateData, 0, 0) && templateData != 0;
    const bool hasValidQuantity = TryGetSpawnTemplateQuantity(&quantity);
    const bool hasSupportedMode = IsSpawnTemplateModeAllowed(squadMode, allegiance);
    const bool hasValidFactionSelection =
        TryResolveSelectedSpawnTemplateFactionSelection(&factionSelection, &factionMessage);
    const bool hasCompatibleFaction =
        hasTarget
        && hasValidFactionSelection
        && TryGetSpawnTemplateFactionRestrictionMessage(
            g_lastTargetSnapshot.target,
            squadMode,
            allegiance,
            factionSelection,
            &factionMessage);
    g_spawnCharactersButton->setEnabled(
        hasTarget
        && hasSelectedTemplate
        && hasValidQuantity
        && hasSupportedMode
        && hasValidFactionSelection
        && hasCompatibleFaction);
}

void OnSpawnSearchTextChanged(MyGUI::EditBox*)
{
    EnsureSpawnTemplateOptionsLoaded();
    RefreshSpawnTemplateList();
}

void OnSpawnCategoryChanged(MyGUI::ComboBox*, size_t)
{
    EnsureSpawnTemplateOptionsLoaded();
    RefreshSpawnTemplateList();
}

void OnSpawnQuantityTextChanged(MyGUI::EditBox*)
{
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void OnSpawnAllegianceChanged(MyGUI::ComboBox*, size_t)
{
    if (g_spawnSelectionSyncInProgress)
    {
        return;
    }

    std::string adjustmentMessage;
    g_spawnSelectionSyncInProgress = true;
    const bool adjusted = TryAutoAdjustSpawnSelectionForAllegianceChange(&adjustmentMessage);
    g_spawnSelectionSyncInProgress = false;

    if (adjusted && !adjustmentMessage.empty())
    {
        SetStatusMessage(adjustmentMessage);
    }

    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void OnSpawnRadiusChanged(MyGUI::ComboBox*, size_t)
{
    RefreshSpawnPreviewText();
}

void OnSpawnCreatureAgeChanged(MyGUI::ComboBox*, size_t)
{
    RefreshSpawnPreviewText();
}

void OnSpawnModeChanged(MyGUI::ComboBox*, size_t)
{
    if (g_spawnSelectionSyncInProgress)
    {
        return;
    }

    std::string adjustmentMessage;
    g_spawnSelectionSyncInProgress = true;
    const bool adjusted = TryAutoAdjustSpawnSelectionForModeChange(&adjustmentMessage);
    g_spawnSelectionSyncInProgress = false;

    if (adjusted && !adjustmentMessage.empty())
    {
        SetStatusMessage(adjustmentMessage);
    }

    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void OnSpawnResultsSelectionChanged(MyGUI::ListBox*, size_t)
{
    RefreshSpawnCreatureAgeControlState();
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void OnSpawnCharactersButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "spawn_templates";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"spawn_templates\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    Character* requestedTarget = g_lastTargetSnapshot.target;
    const std::string requestedTargetName = g_lastTargetSnapshot.name;
    const TargetSource requestedTargetSource = g_lastTargetSnapshot.source;

    GameData* templateData = 0;
    std::string displayName;
    if (!TryResolveSelectedSpawnTemplate(&templateData, &displayName, 0) || !templateData)
    {
        LogInfoLine("event=testkit_action_result action=\"spawn_templates\" success=false reason=\"no_template_selected\"");
        SetStatusMessage("Spawn failed - select a spawn template");
        return;
    }

    int quantity = 0;
    if (!TryGetSpawnTemplateQuantity(&quantity))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_templates\" success=false reason=\"invalid_quantity\""
               << " template_name=\"" << SanitizeLogValue(displayName) << "\"";
        LogInfoLine(result.str());
        std::stringstream status;
        status << "Spawn failed - enter a quantity from 1 to " << kSpawnTemplateQuantityMax;
        SetStatusMessage(status.str());
        return;
    }

    const SpawnTemplateAllegiance allegiance = GetSelectedSpawnTemplateAllegiance();
    const SpawnTemplateRadiusPreset radiusPreset = GetSelectedSpawnTemplateRadiusPreset();
    const SpawnTemplateSquadMode squadMode = GetSelectedSpawnTemplateSquadMode();
    SpawnTemplateFactionSelection factionSelection;
    std::string factionMessage;
    if (!TryResolveAndValidateSpawnTemplateFactionSelection(
            requestedTarget,
            squadMode,
            allegiance,
            &factionSelection,
            &factionMessage))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_templates\" success=false reason=\"invalid_faction_selection\""
               << " template_name=\"" << SanitizeLogValue(displayName) << "\""
               << " quantity=" << quantity
               << " faction_mode=\"" << SanitizeLogValue(SpawnTemplateFactionModeToLabel(GetSelectedSpawnTemplateFactionMode())) << "\""
               << " faction_query=\"" << SanitizeLogValue(g_spawnCustomFactionSearchEdit ? TrimAscii(g_spawnCustomFactionSearchEdit->getOnlyText().asUTF8()) : "") << "\""
               << " target_name=\"" << SanitizeLogValue(requestedTargetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Spawn failed - " + factionMessage);
        return;
    }

    const bool isCreatureTemplate = IsCreatureSpawnTemplateData(templateData);
    const SpawnCreatureAgePreset creatureAgePreset = GetSelectedSpawnCreatureAgePreset();
    SpawnTemplateApplyResult applyResult;
    if (!TrySpawnTemplateNearTarget(
            templateData,
            displayName,
            requestedTarget,
            radiusPreset,
            squadMode,
            allegiance,
            factionSelection,
            creatureAgePreset,
            quantity,
            &applyResult))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_templates\" success=false reason=\"apply_failed\""
               << " template_name=\"" << SanitizeLogValue(displayName) << "\""
               << " quantity=" << quantity
               << " faction_mode=\"" << SanitizeLogValue(SpawnTemplateFactionModeToLabel(factionSelection.mode)) << "\""
               << " faction=\"" << SanitizeLogValue(DescribeSpawnTemplateFactionSelection(factionSelection)) << "\""
               << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
               << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
               << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
               << " creature_template=" << (isCreatureTemplate ? "true" : "false")
               << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
               << " target_name=\"" << SanitizeLogValue(requestedTargetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Spawn failed - " + applyResult.message);
        return;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"spawn_templates\" success="
           << (applyResult.success ? "true" : "false")
           << " template_name=\"" << SanitizeLogValue(displayName) << "\""
           << " quantity=" << quantity
           << " faction_mode=\"" << SanitizeLogValue(SpawnTemplateFactionModeToLabel(factionSelection.mode)) << "\""
           << " faction=\"" << SanitizeLogValue(DescribeSpawnTemplateFactionSelection(factionSelection)) << "\""
           << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
           << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
           << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
           << " creature_template=" << (isCreatureTemplate ? "true" : "false")
           << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
           << " target_name=\"" << SanitizeLogValue(requestedTargetName) << "\""
           << " spawned_count=" << applyResult.spawnedCount
           << " valid_spawn_count=" << applyResult.validSpawnCount
           << " creation_failure_count=" << applyResult.creationFailureCount
           << " hostility_failure_count=" << applyResult.hostilityFailureCount
           << " used_target_platoon_fallback=" << (applyResult.usedTargetPlatoonFallback ? "true" : "false");
    if (!applyResult.success)
    {
        result << " reason=\"" << SanitizeLogValue(applyResult.message) << "\"";
    }
    LogInfoLine(result.str());

    bool restoredRequestedTarget = false;
    if (applyResult.spawnedCount > 0
        && requestedTargetSource == TargetSource_Selected
        && g_lastPlayerInterface)
    {
        restoredRequestedTarget =
            TryRestoreRequestedSelectedSpawnTarget(g_lastPlayerInterface, requestedTarget);
    }

    if (g_developerMode && applyResult.spawnedCount > 0)
    {
        std::stringstream line;
        line << "[investigate][spawn][selection_restore] requested_target_name=\""
             << SanitizeLogValue(requestedTargetName) << "\""
             << " source=\"" << TargetSourceToLogLabel(requestedTargetSource) << "\""
             << " attempted="
             << ((requestedTargetSource == TargetSource_Selected && g_lastPlayerInterface) ? "true" : "false")
             << " restored=" << (restoredRequestedTarget ? "true" : "false");
        if (g_hasLastTargetSnapshot && g_lastTargetSnapshot.hasTarget)
        {
            line << " current_target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        }
        LogInfoLine(line.str());
    }

    if (applyResult.spawnedCount <= 0)
    {
        SetStatusMessage("Spawn failed - " + applyResult.message);
        return;
    }

    if (!applyResult.success)
    {
        std::stringstream status;
        status << "Spawned " << applyResult.spawnedCount;
        if (applyResult.spawnedCount != quantity)
        {
            status << " of " << quantity;
        }
        status << "x " << displayName;
        if (isCreatureTemplate)
        {
            status << " (" << SpawnCreatureAgePresetToLabel(creatureAgePreset) << ")";
        }
        status
               << " near " << requestedTargetName
               << " (" << DescribeSpawnTemplateFactionSelection(factionSelection)
               << ", " << SpawnTemplateAllegianceToLabel(allegiance)
               << ", " << SpawnTemplateRadiusPresetToLabel(radiusPreset)
               << ", " << SpawnTemplateSquadModeToLabel(squadMode) << ")"
               << " - " << applyResult.message;
        SetStatusMessage(status.str());
        return;
    }

    std::stringstream status;
    if (applyResult.spawnedCount == quantity)
    {
        status << "Spawned " << applyResult.spawnedCount << "x " << displayName;
        if (isCreatureTemplate)
        {
            status << " (" << SpawnCreatureAgePresetToLabel(creatureAgePreset) << ")";
        }
        status
               << " near " << requestedTargetName
               << " (" << DescribeSpawnTemplateFactionSelection(factionSelection)
               << ", " << SpawnTemplateAllegianceToLabel(allegiance)
               << ", " << SpawnTemplateRadiusPresetToLabel(radiusPreset)
               << ", " << SpawnTemplateSquadModeToLabel(squadMode) << ")";
    }
    else
    {
        status << "Spawned " << applyResult.spawnedCount << " of " << quantity << "x " << displayName;
        if (isCreatureTemplate)
        {
            status << " (" << SpawnCreatureAgePresetToLabel(creatureAgePreset) << ")";
        }
        status
               << " near " << requestedTargetName
               << " (" << DescribeSpawnTemplateFactionSelection(factionSelection)
               << ", " << SpawnTemplateAllegianceToLabel(allegiance)
               << ", " << SpawnTemplateRadiusPresetToLabel(radiusPreset)
               << ", " << SpawnTemplateSquadModeToLabel(squadMode) << ")";
    }
    SetStatusMessage(status.str());
}

void OnSpawnCharactersButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSpawnCharactersButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}
}
