#include "test_kit_spawn.h"

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
bool TrySpawnTemplateNearTarget(
    GameData* templateData,
    const std::string& templateName,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    SpawnCreatureAgePreset creatureAgePreset,
    int quantity,
    SpawnTemplateApplyResult* outResult);
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

bool TryGetSpawnAnchorPosition(Character* target, Ogre::Vector3* outPosition)
{
    if (!target || !outPosition)
    {
        return false;
    }

    if (TryGetCharacterTeleportReferencePosition(target, true, outPosition, 0))
    {
        return true;
    }

    __try
    {
        *outPosition = target->getPosition();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

float ResolveSpawnTemplateRadiusMultiplier(SpawnTemplateRadiusPreset radiusPreset)
{
    switch (radiusPreset)
    {
    case SpawnTemplateRadius_Close:
        return kSpawnTemplateCloseRadiusMultiplier;
    case SpawnTemplateRadius_Wide:
        return kSpawnTemplateWideRadiusMultiplier;
    case SpawnTemplateRadius_Normal:
    default:
        return 1.0f;
    }
}

bool TryBuildSpawnPlacementOffset(
    int placementIndex,
    SpawnTemplateRadiusPreset radiusPreset,
    Ogre::Vector3* outOffset)
{
    if (!outOffset || placementIndex < 0)
    {
        return false;
    }

    int ring = 0;
    int slot = placementIndex;
    int slotsInRing = kSpawnTemplateFirstRingSlots;
    while (slot >= slotsInRing)
    {
        slot -= slotsInRing;
        ++ring;
        slotsInRing = kSpawnTemplateFirstRingSlots * (ring + 1);
    }

    const float radiusMultiplier = ResolveSpawnTemplateRadiusMultiplier(radiusPreset);
    const float radius =
        (kSpawnTemplateBaseRadius + (static_cast<float>(ring) * kSpawnTemplateRingSpacing)) * radiusMultiplier;
    const float angle = ((2.0f * kPi) * static_cast<float>(slot)) / static_cast<float>(slotsInRing);
    *outOffset = Ogre::Vector3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    return true;
}

bool TryResolveSpawnPlacementPosition(
    Character* target,
    int placementIndex,
    SpawnTemplateRadiusPreset radiusPreset,
    Ogre::Vector3* requestedPositionOut,
    Ogre::Vector3* resolvedPositionOut)
{
    if (requestedPositionOut)
    {
        *requestedPositionOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }
    if (resolvedPositionOut)
    {
        *resolvedPositionOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }

    if (!target || !requestedPositionOut || !resolvedPositionOut || !ou)
    {
        return false;
    }

    Ogre::Vector3 anchorPosition(0.0f, 0.0f, 0.0f);
    Ogre::Vector3 placementOffset(0.0f, 0.0f, 0.0f);
    if (!TryGetSpawnAnchorPosition(target, &anchorPosition)
        || !TryBuildSpawnPlacementOffset(placementIndex, radiusPreset, &placementOffset))
    {
        return false;
    }

    Ogre::Vector3 requestedPosition = anchorPosition + placementOffset;
    ou->fixNaNPosition(requestedPosition);
    Ogre::Vector3 resolvedPosition = requestedPosition;
    Ogre::Vector3 validatedPosition = requestedPosition;
    if (!ou->findValidSpawnPos(validatedPosition, requestedPosition))
    {
        return false;
    }

    if (ComputeHorizontalDistance(validatedPosition, requestedPosition) <= kSpawnTemplateMaxResolvedDrift)
    {
        resolvedPosition = validatedPosition;
    }

    *requestedPositionOut = requestedPosition;
    *resolvedPositionOut = resolvedPosition;
    return true;
}

bool TryResolveTargetSpawnFaction(Character* target, Faction** outFaction)
{
    return TryResolveCharacterFaction(target, outFaction);
}

bool TryResolvePlayerSpawnFaction(Faction** outFaction)
{
    if (!outFaction)
    {
        return false;
    }

    *outFaction = 0;
    if (!ou || !ou->player)
    {
        return false;
    }

    __try
    {
        *outFaction = ou->player->getFaction();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outFaction = 0;
    }

    return *outFaction != 0;
}

bool TryResolveEmptySpawnFaction(Faction** outFaction)
{
    if (!outFaction)
    {
        return false;
    }

    *outFaction = 0;
    if (!ou || !ou->factionMgr)
    {
        return false;
    }

    __try
    {
        *outFaction = ou->factionMgr->getEmptyFaction();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outFaction = 0;
    }

    return *outFaction != 0;
}

SensoryData* TryGetCharacterSensoryData(Character* character)
{
    if (!character)
    {
        return 0;
    }

    __try
    {
        return character->getSensoryData();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

bool TryNoticeCharacter(SensoryData* sensoryData, Character* character, bool alarmed)
{
    if (!sensoryData || !character)
    {
        return false;
    }

    __try
    {
        sensoryData->noticeThisPerson(character, alarmed);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TryResolvePlatoonName(ActivePlatoon* platoon, std::string* outName)
{
    if (!outName)
    {
        return false;
    }

    *outName = "Unknown";
    if (!platoon || !IsProbablyReadableEnginePointer(platoon))
    {
        return false;
    }

    __try
    {
        const std::string& name = platoon->getName();
        if (!name.empty())
        {
            *outName = name;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outName = "Unknown";
        return false;
    }
}

std::string SafePlatoonName(ActivePlatoon* platoon)
{
    std::string name = "Unknown";
    TryResolvePlatoonName(platoon, &name);
    return name;
}

Character* TryGetPlatoonLeader(ActivePlatoon* platoon)
{
    if (!platoon || !IsProbablyReadableEnginePointer(platoon))
    {
        return 0;
    }

    __try
    {
        return platoon->getSquadLeader();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

bool TryResolvePlatoonSize(ActivePlatoon* platoon, int* outSize)
{
    if (!outSize)
    {
        return false;
    }

    *outSize = -1;
    if (!platoon || !IsProbablyReadableEnginePointer(platoon))
    {
        return false;
    }

    __try
    {
        *outSize = platoon->getSquadSize();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outSize = -1;
        return false;
    }
}

bool TryResolveCharacterWithPlayer(Character* character, bool* outIsWithPlayer)
{
    if (!outIsWithPlayer)
    {
        return false;
    }

    *outIsWithPlayer = false;
    if (!character)
    {
        return false;
    }

    __try
    {
        *outIsWithPlayer = character->isWithThePlayer();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outIsWithPlayer = false;
        return false;
    }
}

bool TryResolveCharacterPermajobCount(Character* character, int* outCount)
{
    if (!outCount)
    {
        return false;
    }

    *outCount = -1;
    if (!character)
    {
        return false;
    }

    __try
    {
        *outCount = character->getPermajobCount();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outCount = -1;
        return false;
    }
}

bool TryResolveCharacterAge0To1(Character* character, float* outAge0To1)
{
    if (!outAge0To1)
    {
        return false;
    }

    *outAge0To1 = -1.0f;
    if (!character)
    {
        return false;
    }

    __try
    {
        *outAge0To1 = character->getAge0to1();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outAge0To1 = -1.0f;
        return false;
    }
}

bool TryApplySpawnedCharacterAge(Character* character, float age0To1)
{
    if (!character)
    {
        return false;
    }

    __try
    {
        if (!character->isAnimal())
        {
            return false;
        }

        character->setAge(age0To1);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TryResolveCharacterEnemyState(Character* character, Character* other, bool* outIsEnemy)
{
    if (!outIsEnemy)
    {
        return false;
    }

    *outIsEnemy = false;
    if (!character || !other)
    {
        return false;
    }

    __try
    {
        *outIsEnemy = character->isEnemy(other, false);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outIsEnemy = false;
        return false;
    }
}

bool TryResolvePlayerEnemyState(Character* character, bool* outIsEnemy)
{
    if (!outIsEnemy)
    {
        return false;
    }

    *outIsEnemy = false;
    if (!character || !ou || !ou->player)
    {
        return false;
    }

    __try
    {
        *outIsEnemy = ou->player->isEnemy(character);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outIsEnemy = false;
        return false;
    }
}

bool TryResolveSensoryAwareness(Character* observer, Character* other, bool* outAware)
{
    if (!outAware)
    {
        return false;
    }

    *outAware = false;
    if (!observer || !other)
    {
        return false;
    }

    SensoryData* sensoryData = TryGetCharacterSensoryData(observer);
    if (!sensoryData)
    {
        return false;
    }

    __try
    {
        *outAware = sensoryData->amIAwareOfThisGuy(other, false);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outAware = false;
        return false;
    }
}

bool TryResolveSensoryCanSee(Character* observer, Character* other, bool* outCanSee)
{
    if (!outCanSee)
    {
        return false;
    }

    *outCanSee = false;
    if (!observer || !other)
    {
        return false;
    }

    SensoryData* sensoryData = TryGetCharacterSensoryData(observer);
    if (!sensoryData)
    {
        return false;
    }

    __try
    {
        *outCanSee = sensoryData->canISeeThisGuy(other);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outCanSee = false;
        return false;
    }
}

bool TryResolveCharacterSensorySummary(
    Character* character,
    int* outNumEnemies,
    int* outNumNeutrals,
    float* outNearestEnemyDistanceSq,
    float* outLastThreat)
{
    if (outNumEnemies)
    {
        *outNumEnemies = -1;
    }
    if (outNumNeutrals)
    {
        *outNumNeutrals = -1;
    }
    if (outNearestEnemyDistanceSq)
    {
        *outNearestEnemyDistanceSq = -1.0f;
    }
    if (outLastThreat)
    {
        *outLastThreat = -1.0f;
    }

    if (!character)
    {
        return false;
    }

    SensoryData* sensoryData = TryGetCharacterSensoryData(character);
    if (!sensoryData)
    {
        return false;
    }

    __try
    {
        if (outNumEnemies)
        {
            *outNumEnemies = sensoryData->numEnemies;
        }
        if (outNumNeutrals)
        {
            *outNumNeutrals = sensoryData->numNeutrals;
        }
        if (outNearestEnemyDistanceSq)
        {
            *outNearestEnemyDistanceSq = sensoryData->getNearestEnemyDistanceSq();
        }
        if (outLastThreat)
        {
            *outLastThreat = sensoryData->lastThreat;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (outNumEnemies)
        {
            *outNumEnemies = -1;
        }
        if (outNumNeutrals)
        {
            *outNumNeutrals = -1;
        }
        if (outNearestEnemyDistanceSq)
        {
            *outNearestEnemyDistanceSq = -1.0f;
        }
        if (outLastThreat)
        {
            *outLastThreat = -1.0f;
        }
        return false;
    }
}

const char* FormatOptionalBoolLogValue(bool isKnown, bool value)
{
    if (!isKnown)
    {
        return "unknown";
    }

    return value ? "true" : "false";
}

void AppendSensorySummaryLogFields(
    std::stringstream& line,
    const char* prefix,
    bool isKnown,
    int numEnemies,
    int numNeutrals,
    float nearestEnemyDistanceSq,
    float lastThreat)
{
    const std::string fieldPrefix = prefix ? prefix : "sensory";
    line << " " << fieldPrefix << "_known=" << (isKnown ? "true" : "false")
         << " " << fieldPrefix << "_num_enemies=" << (isKnown ? numEnemies : -1)
         << " " << fieldPrefix << "_num_neutrals=" << (isKnown ? numNeutrals : -1)
         << " " << fieldPrefix << "_nearest_enemy_distance_sq="
         << (isKnown ? nearestEnemyDistanceSq : -1.0f)
         << " " << fieldPrefix << "_last_threat=" << (isKnown ? lastThreat : -1.0f);
}

void AppendFactionLogFields(std::stringstream& line, const char* prefix, Faction* faction)
{
    const std::string fieldPrefix = prefix ? prefix : "faction";
    std::string factionName = "Unknown";
    if (faction && IsProbablyReadableEnginePointer(faction))
    {
        factionName = SafeFactionName(faction);
    }
    else if (faction)
    {
        factionName = "Invalid";
    }
    line << " " << fieldPrefix << "_ptr=" << FormatPointerValue(faction)
         << " " << fieldPrefix << "_name=\"" << SanitizeLogValue(factionName) << "\"";
}

void AppendPlatoonLogFields(std::stringstream& line, const char* prefix, ActivePlatoon* platoon)
{
    const std::string fieldPrefix = prefix ? prefix : "platoon";
    if (platoon && !IsProbablyReadableEnginePointer(platoon))
    {
        line << " " << fieldPrefix << "_ptr=" << FormatPointerValue(platoon)
             << " " << fieldPrefix << "_name=\"Invalid\""
             << " " << fieldPrefix << "_size=-1"
             << " " << fieldPrefix << "_leader_ptr=" << FormatPointerValue(0)
             << " " << fieldPrefix << "_leader_name=\"Unknown\"";
        return;
    }

    const int squadSize = [&]() -> int
    {
        int value = -1;
        TryResolvePlatoonSize(platoon, &value);
        return value;
    }();
    Character* squadLeader = TryGetPlatoonLeader(platoon);

    line << " " << fieldPrefix << "_ptr=" << FormatPointerValue(platoon)
         << " " << fieldPrefix << "_name=\"" << SanitizeLogValue(SafePlatoonName(platoon)) << "\""
         << " " << fieldPrefix << "_size=" << squadSize
         << " " << fieldPrefix << "_leader_ptr=" << FormatPointerValue(squadLeader)
         << " " << fieldPrefix << "_leader_name=\"" << SanitizeLogValue(SafeCharacterName(squadLeader)) << "\"";
}

ActivePlatoon* TryGetCharacterActivePlatoon(Character* character)
{
    if (!character)
    {
        return 0;
    }

    __try
    {
        return character->getPlatoon();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

void AppendCharacterProbeLogFields(std::stringstream& line, const char* prefix, Character* character)
{
    const std::string fieldPrefix = prefix ? prefix : "character";
    if (character && !IsProbablyReadableEnginePointer(character))
    {
        line << " " << fieldPrefix << "_ptr=" << FormatPointerValue(character)
             << " " << fieldPrefix << "_name=\"Invalid\""
             << " " << fieldPrefix << "_with_player=unknown"
             << " " << fieldPrefix << "_permajob_count=-1"
             << " " << fieldPrefix << "_age_0to1=-1"
             << " " << fieldPrefix << "_snapshot_known=false";

        const std::string factionPrefix = fieldPrefix + "_faction";
        AppendFactionLogFields(line, factionPrefix.c_str(), 0);

        const std::string platoonPrefix = fieldPrefix + "_platoon";
        AppendPlatoonLogFields(line, platoonPrefix.c_str(), 0);

        const std::string sensoryPrefix = fieldPrefix + "_sensory";
        AppendSensorySummaryLogFields(line, sensoryPrefix.c_str(), false, -1, -1, -1.0f, -1.0f);
        return;
    }

    Faction* faction = 0;
    TryResolveCharacterFaction(character, &faction);

    ActivePlatoon* platoon = TryGetCharacterActivePlatoon(character);

    bool isWithPlayer = false;
    const bool hasWithPlayer = TryResolveCharacterWithPlayer(character, &isWithPlayer);
    int permajobCount = -1;
    const bool hasPermajobCount = TryResolveCharacterPermajobCount(character, &permajobCount);
    float age0To1 = -1.0f;
    const bool hasAge0To1 = TryResolveCharacterAge0To1(character, &age0To1);
    int numEnemies = -1;
    int numNeutrals = -1;
    float nearestEnemyDistanceSq = -1.0f;
    float lastThreat = -1.0f;
    const bool hasSensory =
        TryResolveCharacterSensorySummary(character, &numEnemies, &numNeutrals, &nearestEnemyDistanceSq, &lastThreat);
    CharacterPositionSnapshot snapshot;
    const bool hasSnapshot = TryGetCharacterPositionSnapshot(character, &snapshot);

    line << " " << fieldPrefix << "_ptr=" << FormatPointerValue(character)
         << " " << fieldPrefix << "_name=\"" << SanitizeLogValue(SafeCharacterName(character)) << "\""
         << " " << fieldPrefix << "_with_player=" << FormatOptionalBoolLogValue(hasWithPlayer, isWithPlayer)
         << " " << fieldPrefix << "_permajob_count=" << (hasPermajobCount ? permajobCount : -1)
         << " " << fieldPrefix << "_age_0to1=" << (hasAge0To1 ? age0To1 : -1.0f)
         << " " << fieldPrefix << "_snapshot_known=" << (hasSnapshot ? "true" : "false");

    const std::string factionPrefix = fieldPrefix + "_faction";
    AppendFactionLogFields(line, factionPrefix.c_str(), faction);

    const std::string platoonPrefix = fieldPrefix + "_platoon";
    AppendPlatoonLogFields(line, platoonPrefix.c_str(), platoon);

    const std::string sensoryPrefix = fieldPrefix + "_sensory";
    AppendSensorySummaryLogFields(
        line,
        sensoryPrefix.c_str(),
        hasSensory,
        numEnemies,
        numNeutrals,
        nearestEnemyDistanceSq,
        lastThreat);

    if (hasSnapshot)
    {
        const std::string snapshotPrefix = fieldPrefix + "_snapshot";
        AppendCharacterSnapshotLogFields(line, snapshotPrefix.c_str(), snapshot);
    }
}

void AppendCharacterRelationProbeLogFields(
    std::stringstream& line,
    const char* prefix,
    Character* observer,
    Character* other)
{
    const std::string fieldPrefix = prefix ? prefix : "relation";
    bool isEnemy = false;
    const bool hasEnemy = TryResolveCharacterEnemyState(observer, other, &isEnemy);
    bool isAware = false;
    const bool hasAwareness = TryResolveSensoryAwareness(observer, other, &isAware);
    bool canSee = false;
    const bool hasCanSee = TryResolveSensoryCanSee(observer, other, &canSee);

    line << " " << fieldPrefix << "_enemy=" << FormatOptionalBoolLogValue(hasEnemy, isEnemy)
         << " " << fieldPrefix << "_aware=" << FormatOptionalBoolLogValue(hasAwareness, isAware)
         << " " << fieldPrefix << "_can_see=" << FormatOptionalBoolLogValue(hasCanSee, canSee);
}

void LogSpawnInvestigationReject(
    const char* reason,
    const std::string& templateName,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    bool isCreatureTemplate,
    SpawnCreatureAgePreset creatureAgePreset,
    float requestedAge0To1,
    int quantity,
    Faction* targetFaction,
    Faction* desiredFaction,
    Faction* emptyFaction,
    Faction* createFaction,
    ActivePlatoon* targetPlatoon)
{
    if (!g_developerMode)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][spawn][reject] reason=\"" << SanitizeLogValue(reason ? reason : "unknown") << "\""
         << " template_name=\"" << SanitizeLogValue(templateName) << "\""
         << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\""
         << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
         << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
         << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
         << " creature_template=" << (isCreatureTemplate ? "true" : "false")
         << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
         << " requested_age_0to1=" << requestedAge0To1
         << " quantity=" << quantity;
    AppendCharacterProbeLogFields(line, "target", target);
    AppendFactionLogFields(line, "resolved_target_faction", targetFaction);
    AppendFactionLogFields(line, "desired_faction", desiredFaction);
    AppendFactionLogFields(line, "empty_faction", emptyFaction);
    AppendFactionLogFields(line, "create_faction", createFaction);
    AppendPlatoonLogFields(line, "resolved_target_platoon", targetPlatoon);
    LogInfoLine(line.str());
}

void LogSpawnInvestigationBegin(
    const std::string& templateName,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    bool isCreatureTemplate,
    SpawnCreatureAgePreset creatureAgePreset,
    float requestedAge0To1,
    int quantity,
    Faction* targetFaction,
    Faction* desiredFaction,
    Faction* emptyFaction,
    Faction* createFaction,
    ActivePlatoon* targetPlatoon)
{
    if (!g_developerMode)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][spawn][begin] template_name=\"" << SanitizeLogValue(templateName) << "\""
         << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\""
         << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
         << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
         << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
         << " creature_template=" << (isCreatureTemplate ? "true" : "false")
         << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
         << " requested_age_0to1=" << requestedAge0To1
         << " quantity=" << quantity
         << " add_to_target_squad="
         << (squadMode == SpawnTemplateSquadMode_AddToTargetSquad ? "true" : "false");
    AppendCharacterProbeLogFields(line, "target", target);
    AppendFactionLogFields(line, "resolved_target_faction", targetFaction);
    AppendFactionLogFields(line, "desired_faction", desiredFaction);
    AppendFactionLogFields(line, "empty_faction", emptyFaction);
    AppendFactionLogFields(line, "create_faction", createFaction);
    AppendPlatoonLogFields(line, "resolved_target_platoon", targetPlatoon);
    LogInfoLine(line.str());
}

void LogSpawnInvestigationHostility(
    const std::string& templateName,
    Character* spawnedCharacter,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    bool hasTargetPosition,
    bool noticedSpawnedToTarget,
    bool noticedTargetToSpawned,
    bool targetMarkedUnderAttack,
    bool focusedAttackGoalIssued,
    bool focusedAttackJobIssued,
    bool attackOrderIssued,
    bool hasSpawnedWithPlayerBefore,
    bool spawnedWithPlayerBefore,
    bool hasSpawnedWithPlayerAfter,
    bool spawnedWithPlayerAfter,
    bool hasPlayerEnemyOfSpawnBefore,
    bool playerEnemyOfSpawnBefore,
    bool hasPlayerEnemyOfSpawnAfter,
    bool playerEnemyOfSpawnAfter,
    bool hasSpawnedPermajobCountBefore,
    int spawnedPermajobCountBefore,
    bool hasSpawnedPermajobCountAfter,
    int spawnedPermajobCountAfter,
    bool hasSpawnedEnemyBefore,
    bool spawnedEnemyBefore,
    bool hasSpawnedEnemyAfter,
    bool spawnedEnemyAfter,
    bool hasTargetEnemyBefore,
    bool targetEnemyBefore,
    bool hasTargetEnemyAfter,
    bool targetEnemyAfter,
    bool hasSpawnedAwareBefore,
    bool spawnedAwareBefore,
    bool hasSpawnedAwareAfter,
    bool spawnedAwareAfter,
    bool hasTargetAwareBefore,
    bool targetAwareBefore,
    bool hasTargetAwareAfter,
    bool targetAwareAfter,
    bool hasSpawnedCanSeeBefore,
    bool spawnedCanSeeBefore,
    bool hasSpawnedCanSeeAfter,
    bool spawnedCanSeeAfter,
    bool hasTargetCanSeeBefore,
    bool targetCanSeeBefore,
    bool hasTargetCanSeeAfter,
    bool targetCanSeeAfter,
    bool hasSpawnedSensoryBefore,
    int spawnedNumEnemiesBefore,
    int spawnedNumNeutralsBefore,
    float spawnedNearestEnemyDistanceSqBefore,
    float spawnedLastThreatBefore,
    bool hasSpawnedSensoryAfter,
    int spawnedNumEnemiesAfter,
    int spawnedNumNeutralsAfter,
    float spawnedNearestEnemyDistanceSqAfter,
    float spawnedLastThreatAfter,
    bool hasTargetSensoryBefore,
    int targetNumEnemiesBefore,
    int targetNumNeutralsBefore,
    float targetNearestEnemyDistanceSqBefore,
    float targetLastThreatBefore,
    bool hasTargetSensoryAfter,
    int targetNumEnemiesAfter,
    int targetNumNeutralsAfter,
    float targetNearestEnemyDistanceSqAfter,
    float targetLastThreatAfter)
{
    if (!g_developerMode)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][spawn][hostility] template_name=\"" << SanitizeLogValue(templateName) << "\""
         << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\""
         << " spawned_name=\"" << SanitizeLogValue(SafeCharacterName(spawnedCharacter)) << "\""
         << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
         << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
         << " target_position_known=" << (hasTargetPosition ? "true" : "false")
         << " notice_spawned_to_target=" << (noticedSpawnedToTarget ? "true" : "false")
         << " notice_target_to_spawned=" << (noticedTargetToSpawned ? "true" : "false")
         << " target_marked_under_attack=" << (targetMarkedUnderAttack ? "true" : "false")
         << " focused_attack_goal_issued=" << (focusedAttackGoalIssued ? "true" : "false")
         << " focused_attack_job_issued=" << (focusedAttackJobIssued ? "true" : "false")
         << " attack_order_issued=" << (attackOrderIssued ? "true" : "false")
         << " spawned_with_player_before="
         << FormatOptionalBoolLogValue(hasSpawnedWithPlayerBefore, spawnedWithPlayerBefore)
         << " spawned_with_player_after="
         << FormatOptionalBoolLogValue(hasSpawnedWithPlayerAfter, spawnedWithPlayerAfter)
         << " player_enemy_of_spawn_before="
         << FormatOptionalBoolLogValue(hasPlayerEnemyOfSpawnBefore, playerEnemyOfSpawnBefore)
         << " player_enemy_of_spawn_after="
         << FormatOptionalBoolLogValue(hasPlayerEnemyOfSpawnAfter, playerEnemyOfSpawnAfter)
         << " spawned_permajob_count_before="
         << (hasSpawnedPermajobCountBefore ? spawnedPermajobCountBefore : -1)
         << " spawned_permajob_count_after="
         << (hasSpawnedPermajobCountAfter ? spawnedPermajobCountAfter : -1)
         << " spawned_enemy_of_target_before="
         << FormatOptionalBoolLogValue(hasSpawnedEnemyBefore, spawnedEnemyBefore)
         << " spawned_enemy_of_target_after="
         << FormatOptionalBoolLogValue(hasSpawnedEnemyAfter, spawnedEnemyAfter)
         << " target_enemy_of_spawn_before="
         << FormatOptionalBoolLogValue(hasTargetEnemyBefore, targetEnemyBefore)
         << " target_enemy_of_spawn_after="
         << FormatOptionalBoolLogValue(hasTargetEnemyAfter, targetEnemyAfter)
         << " spawned_aware_of_target_before="
         << FormatOptionalBoolLogValue(hasSpawnedAwareBefore, spawnedAwareBefore)
         << " spawned_aware_of_target_after="
         << FormatOptionalBoolLogValue(hasSpawnedAwareAfter, spawnedAwareAfter)
         << " target_aware_of_spawn_before="
         << FormatOptionalBoolLogValue(hasTargetAwareBefore, targetAwareBefore)
         << " target_aware_of_spawn_after="
         << FormatOptionalBoolLogValue(hasTargetAwareAfter, targetAwareAfter)
         << " spawned_can_see_target_before="
         << FormatOptionalBoolLogValue(hasSpawnedCanSeeBefore, spawnedCanSeeBefore)
         << " spawned_can_see_target_after="
         << FormatOptionalBoolLogValue(hasSpawnedCanSeeAfter, spawnedCanSeeAfter)
         << " target_can_see_spawn_before="
         << FormatOptionalBoolLogValue(hasTargetCanSeeBefore, targetCanSeeBefore)
         << " target_can_see_spawn_after="
         << FormatOptionalBoolLogValue(hasTargetCanSeeAfter, targetCanSeeAfter);
    AppendSensorySummaryLogFields(
        line,
        "spawned_sensory_before",
        hasSpawnedSensoryBefore,
        spawnedNumEnemiesBefore,
        spawnedNumNeutralsBefore,
        spawnedNearestEnemyDistanceSqBefore,
        spawnedLastThreatBefore);
    AppendSensorySummaryLogFields(
        line,
        "spawned_sensory_after",
        hasSpawnedSensoryAfter,
        spawnedNumEnemiesAfter,
        spawnedNumNeutralsAfter,
        spawnedNearestEnemyDistanceSqAfter,
        spawnedLastThreatAfter);
    AppendSensorySummaryLogFields(
        line,
        "target_sensory_before",
        hasTargetSensoryBefore,
        targetNumEnemiesBefore,
        targetNumNeutralsBefore,
        targetNearestEnemyDistanceSqBefore,
        targetLastThreatBefore);
    AppendSensorySummaryLogFields(
        line,
        "target_sensory_after",
        hasTargetSensoryAfter,
        targetNumEnemiesAfter,
        targetNumNeutralsAfter,
        targetNearestEnemyDistanceSqAfter,
        targetLastThreatAfter);
    AppendCharacterProbeLogFields(line, "spawned", spawnedCharacter);
    AppendCharacterProbeLogFields(line, "target", target);
    AppendCharacterRelationProbeLogFields(line, "spawned_to_target", spawnedCharacter, target);
    AppendCharacterRelationProbeLogFields(line, "target_to_spawned", target, spawnedCharacter);
    LogInfoLine(line.str());
}

bool TryPrimeLocalSpawnHostility(
    const std::string& templateName,
    Character* spawnedCharacter,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode)
{
    if (!spawnedCharacter || !target || spawnedCharacter == target)
    {
        return false;
    }

    bool spawnedEnemyBefore = false;
    const bool hasSpawnedEnemyBefore = TryResolveCharacterEnemyState(spawnedCharacter, target, &spawnedEnemyBefore);
    bool targetEnemyBefore = false;
    const bool hasTargetEnemyBefore = TryResolveCharacterEnemyState(target, spawnedCharacter, &targetEnemyBefore);
    bool spawnedAwareBefore = false;
    const bool hasSpawnedAwareBefore = TryResolveSensoryAwareness(spawnedCharacter, target, &spawnedAwareBefore);
    bool targetAwareBefore = false;
    const bool hasTargetAwareBefore = TryResolveSensoryAwareness(target, spawnedCharacter, &targetAwareBefore);
    bool spawnedCanSeeBefore = false;
    const bool hasSpawnedCanSeeBefore = TryResolveSensoryCanSee(spawnedCharacter, target, &spawnedCanSeeBefore);
    bool targetCanSeeBefore = false;
    const bool hasTargetCanSeeBefore = TryResolveSensoryCanSee(target, spawnedCharacter, &targetCanSeeBefore);
    bool spawnedWithPlayerBefore = false;
    const bool hasSpawnedWithPlayerBefore = TryResolveCharacterWithPlayer(spawnedCharacter, &spawnedWithPlayerBefore);
    bool playerEnemyOfSpawnBefore = false;
    const bool hasPlayerEnemyOfSpawnBefore = TryResolvePlayerEnemyState(spawnedCharacter, &playerEnemyOfSpawnBefore);
    int spawnedPermajobCountBefore = -1;
    const bool hasSpawnedPermajobCountBefore =
        TryResolveCharacterPermajobCount(spawnedCharacter, &spawnedPermajobCountBefore);
    int spawnedNumEnemiesBefore = -1;
    int spawnedNumNeutralsBefore = -1;
    float spawnedNearestEnemyDistanceSqBefore = -1.0f;
    float spawnedLastThreatBefore = -1.0f;
    const bool hasSpawnedSensoryBefore = TryResolveCharacterSensorySummary(
        spawnedCharacter,
        &spawnedNumEnemiesBefore,
        &spawnedNumNeutralsBefore,
        &spawnedNearestEnemyDistanceSqBefore,
        &spawnedLastThreatBefore);
    int targetNumEnemiesBefore = -1;
    int targetNumNeutralsBefore = -1;
    float targetNearestEnemyDistanceSqBefore = -1.0f;
    float targetLastThreatBefore = -1.0f;
    const bool hasTargetSensoryBefore = TryResolveCharacterSensorySummary(
        target,
        &targetNumEnemiesBefore,
        &targetNumNeutralsBefore,
        &targetNearestEnemyDistanceSqBefore,
        &targetLastThreatBefore);

    const bool noticedSpawnedToTarget = TryNoticeCharacter(TryGetCharacterSensoryData(spawnedCharacter), target, true);
    const bool noticedTargetToSpawned = TryNoticeCharacter(TryGetCharacterSensoryData(target), spawnedCharacter, true);

    CharacterPositionSnapshot targetSnapshot;
    const bool hasTargetPosition = TryGetCharacterPositionSnapshot(target, &targetSnapshot) && targetSnapshot.hasPosition;

    bool targetMarkedUnderAttack = false;
    __try
    {
        target->attackingYou(spawnedCharacter, true, false);
        targetMarkedUnderAttack = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    bool focusedAttackGoalIssued = false;
    __try
    {
        spawnedCharacter->addGoal(FOCUSED_MELEE_ATTACK, target);
        focusedAttackGoalIssued = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    bool focusedAttackJobIssued = false;
    if (hasTargetPosition)
    {
        __try
        {
            spawnedCharacter->addJob(FOCUSED_MELEE_ATTACK, target, false, false, targetSnapshot.position);
            focusedAttackJobIssued = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    bool attackOrderIssued = false;
    __try
    {
        spawnedCharacter->attackTarget(target);
        attackOrderIssued = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    bool spawnedEnemyAfter = false;
    const bool hasSpawnedEnemyAfter = TryResolveCharacterEnemyState(spawnedCharacter, target, &spawnedEnemyAfter);
    bool targetEnemyAfter = false;
    const bool hasTargetEnemyAfter = TryResolveCharacterEnemyState(target, spawnedCharacter, &targetEnemyAfter);
    bool spawnedAwareAfter = false;
    const bool hasSpawnedAwareAfter = TryResolveSensoryAwareness(spawnedCharacter, target, &spawnedAwareAfter);
    bool targetAwareAfter = false;
    const bool hasTargetAwareAfter = TryResolveSensoryAwareness(target, spawnedCharacter, &targetAwareAfter);
    bool spawnedCanSeeAfter = false;
    const bool hasSpawnedCanSeeAfter = TryResolveSensoryCanSee(spawnedCharacter, target, &spawnedCanSeeAfter);
    bool targetCanSeeAfter = false;
    const bool hasTargetCanSeeAfter = TryResolveSensoryCanSee(target, spawnedCharacter, &targetCanSeeAfter);
    bool spawnedWithPlayerAfter = false;
    const bool hasSpawnedWithPlayerAfter = TryResolveCharacterWithPlayer(spawnedCharacter, &spawnedWithPlayerAfter);
    bool playerEnemyOfSpawnAfter = false;
    const bool hasPlayerEnemyOfSpawnAfter = TryResolvePlayerEnemyState(spawnedCharacter, &playerEnemyOfSpawnAfter);
    int spawnedPermajobCountAfter = -1;
    const bool hasSpawnedPermajobCountAfter =
        TryResolveCharacterPermajobCount(spawnedCharacter, &spawnedPermajobCountAfter);
    int spawnedNumEnemiesAfter = -1;
    int spawnedNumNeutralsAfter = -1;
    float spawnedNearestEnemyDistanceSqAfter = -1.0f;
    float spawnedLastThreatAfter = -1.0f;
    const bool hasSpawnedSensoryAfter = TryResolveCharacterSensorySummary(
        spawnedCharacter,
        &spawnedNumEnemiesAfter,
        &spawnedNumNeutralsAfter,
        &spawnedNearestEnemyDistanceSqAfter,
        &spawnedLastThreatAfter);
    int targetNumEnemiesAfter = -1;
    int targetNumNeutralsAfter = -1;
    float targetNearestEnemyDistanceSqAfter = -1.0f;
    float targetLastThreatAfter = -1.0f;
    const bool hasTargetSensoryAfter = TryResolveCharacterSensorySummary(
        target,
        &targetNumEnemiesAfter,
        &targetNumNeutralsAfter,
        &targetNearestEnemyDistanceSqAfter,
        &targetLastThreatAfter);

    LogSpawnInvestigationHostility(
        templateName,
        spawnedCharacter,
        target,
        radiusPreset,
        squadMode,
        hasTargetPosition,
        noticedSpawnedToTarget,
        noticedTargetToSpawned,
        targetMarkedUnderAttack,
        focusedAttackGoalIssued,
        focusedAttackJobIssued,
        attackOrderIssued,
        hasSpawnedWithPlayerBefore,
        spawnedWithPlayerBefore,
        hasSpawnedWithPlayerAfter,
        spawnedWithPlayerAfter,
        hasPlayerEnemyOfSpawnBefore,
        playerEnemyOfSpawnBefore,
        hasPlayerEnemyOfSpawnAfter,
        playerEnemyOfSpawnAfter,
        hasSpawnedPermajobCountBefore,
        spawnedPermajobCountBefore,
        hasSpawnedPermajobCountAfter,
        spawnedPermajobCountAfter,
        hasSpawnedEnemyBefore,
        spawnedEnemyBefore,
        hasSpawnedEnemyAfter,
        spawnedEnemyAfter,
        hasTargetEnemyBefore,
        targetEnemyBefore,
        hasTargetEnemyAfter,
        targetEnemyAfter,
        hasSpawnedAwareBefore,
        spawnedAwareBefore,
        hasSpawnedAwareAfter,
        spawnedAwareAfter,
        hasTargetAwareBefore,
        targetAwareBefore,
        hasTargetAwareAfter,
        targetAwareAfter,
        hasSpawnedCanSeeBefore,
        spawnedCanSeeBefore,
        hasSpawnedCanSeeAfter,
        spawnedCanSeeAfter,
        hasTargetCanSeeBefore,
        targetCanSeeBefore,
        hasTargetCanSeeAfter,
        targetCanSeeAfter,
        hasSpawnedSensoryBefore,
        spawnedNumEnemiesBefore,
        spawnedNumNeutralsBefore,
        spawnedNearestEnemyDistanceSqBefore,
        spawnedLastThreatBefore,
        hasSpawnedSensoryAfter,
        spawnedNumEnemiesAfter,
        spawnedNumNeutralsAfter,
        spawnedNearestEnemyDistanceSqAfter,
        spawnedLastThreatAfter,
        hasTargetSensoryBefore,
        targetNumEnemiesBefore,
        targetNumNeutralsBefore,
        targetNearestEnemyDistanceSqBefore,
        targetLastThreatBefore,
        hasTargetSensoryAfter,
        targetNumEnemiesAfter,
        targetNumNeutralsAfter,
        targetNearestEnemyDistanceSqAfter,
        targetLastThreatAfter);

    return attackOrderIssued || targetMarkedUnderAttack || focusedAttackGoalIssued || focusedAttackJobIssued;
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

bool TryResolveSpawnAllegianceFaction(
    Character* target,
    SpawnTemplateAllegiance allegiance,
    Faction** outFaction)
{
    if (!outFaction)
    {
        return false;
    }

    *outFaction = 0;

    Faction* targetFaction = 0;
    const bool hasTargetFaction = TryResolveTargetSpawnFaction(target, &targetFaction) && targetFaction != 0;
    Faction* playerFaction = 0;
    const bool hasPlayerFaction = TryResolvePlayerSpawnFaction(&playerFaction) && playerFaction != 0;

    switch (allegiance)
    {
    case SpawnTemplateAllegiance_FriendlyPlayer:
        if (hasPlayerFaction)
        {
            *outFaction = playerFaction;
            return true;
        }
        break;
    case SpawnTemplateAllegiance_Neutral:
        if (TryResolveEmptySpawnFaction(outFaction))
        {
            return true;
        }
        break;
    case SpawnTemplateAllegiance_Hostile:
        return TryResolveEmptySpawnFaction(outFaction);
    case SpawnTemplateAllegiance_SameAsTarget:
    default:
        break;
    }

    if (hasTargetFaction)
    {
        *outFaction = targetFaction;
        return true;
    }

    if (hasPlayerFaction)
    {
        *outFaction = playerFaction;
        return true;
    }

    return false;
}

Character* TryCreateSpawnedCharacter(
    GameData* templateData,
    Faction* faction,
    RootObjectContainer* ownerContainer,
    const Ogre::Vector3& resolvedPosition,
    float age0To1)
{
    if (!templateData || !ou || !ou->theFactory)
    {
        return 0;
    }

    RootObject* createdRoot = 0;
    __try
    {
        createdRoot = ou->theFactory->createRandomCharacter(
            faction,
            resolvedPosition,
            ownerContainer,
            templateData,
            0,
            age0To1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    if (!createdRoot)
    {
        return 0;
    }

    __try
    {
        return createdRoot->getHandle().getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

void TryAssignSpawnedCharacterFaction(Character* spawnedCharacter, Faction* faction, ActivePlatoon* spawnGroupContainer)
{
    if (!spawnedCharacter || !faction)
    {
        return;
    }

    __try
    {
        spawnedCharacter->setFaction(faction, spawnGroupContainer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void TryFinalizeSpawnedCharacterPosition(Character* spawnedCharacter)
{
    if (!spawnedCharacter)
    {
        return;
    }

    __try
    {
        spawnedCharacter->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool TrySpawnTemplateNearTarget(
    GameData* templateData,
    const std::string& templateName,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    SpawnCreatureAgePreset creatureAgePreset,
    int quantity,
    SpawnTemplateApplyResult* outResult)
{
    if (!outResult)
    {
        return false;
    }

    *outResult = SpawnTemplateApplyResult();
    outResult->requestedCount = quantity;

    Faction* targetFaction = 0;
    Faction* desiredFaction = 0;
    Faction* emptyFaction = 0;
    Faction* createFaction = 0;
    ActivePlatoon* targetPlatoon = 0;
    const bool addToTargetSquad = (squadMode == SpawnTemplateSquadMode_AddToTargetSquad);
    const bool isCreatureTemplate = IsCreatureSpawnTemplateData(templateData);
    const float requestedAge0To1 =
        isCreatureTemplate ? ResolveSpawnCreatureAge0To1(creatureAgePreset) : 1.0f;

    if (!templateData || !target || quantity <= 0 || !ou || !ou->theFactory)
    {
        outResult->message = "spawn prerequisites unavailable";
        LogSpawnInvestigationReject(
            "spawn_prerequisites_unavailable",
            templateName,
            target,
            radiusPreset,
            squadMode,
            allegiance,
            isCreatureTemplate,
            creatureAgePreset,
            requestedAge0To1,
            quantity,
            targetFaction,
            desiredFaction,
            emptyFaction,
            createFaction,
            targetPlatoon);
        return false;
    }

    if (!TryResolveTargetSpawnFaction(target, &targetFaction))
    {
        outResult->message = "target faction unavailable";
        LogSpawnInvestigationReject(
            "target_faction_unavailable",
            templateName,
            target,
            radiusPreset,
            squadMode,
            allegiance,
            isCreatureTemplate,
            creatureAgePreset,
            requestedAge0To1,
            quantity,
            targetFaction,
            desiredFaction,
            emptyFaction,
            createFaction,
            targetPlatoon);
        return false;
    }

    if (!TryResolveSpawnAllegianceFaction(target, allegiance, &desiredFaction) || !desiredFaction)
    {
        outResult->message = "spawn allegiance unavailable";
        LogSpawnInvestigationReject(
            "spawn_allegiance_unavailable",
            templateName,
            target,
            radiusPreset,
            squadMode,
            allegiance,
            isCreatureTemplate,
            creatureAgePreset,
            requestedAge0To1,
            quantity,
            targetFaction,
            desiredFaction,
            emptyFaction,
            createFaction,
            targetPlatoon);
        return false;
    }

    targetPlatoon = TryGetCharacterActivePlatoon(target);
    if (!IsSpawnTemplateModeAllowed(squadMode, allegiance))
    {
        outResult->message = GetSpawnTemplateModeRestrictionMessage(squadMode, allegiance);
        LogSpawnInvestigationReject(
            "mode_unavailable",
            templateName,
            target,
            radiusPreset,
            squadMode,
            allegiance,
            isCreatureTemplate,
            creatureAgePreset,
            requestedAge0To1,
            quantity,
            targetFaction,
            desiredFaction,
            emptyFaction,
            createFaction,
            targetPlatoon);
        return false;
    }

    if (addToTargetSquad)
    {
        if (!targetPlatoon)
        {
            outResult->message = "target squad unavailable";
            LogSpawnInvestigationReject(
                "target_squad_unavailable",
                templateName,
                target,
                radiusPreset,
                squadMode,
                allegiance,
                isCreatureTemplate,
                creatureAgePreset,
                requestedAge0To1,
                quantity,
                targetFaction,
                desiredFaction,
                emptyFaction,
                createFaction,
                targetPlatoon);
            return false;
        }

        if (!targetFaction || desiredFaction != targetFaction)
        {
            outResult->message = "selected allegiance can't join target squad";
            LogSpawnInvestigationReject(
                "selected_allegiance_cant_join_target_squad",
                templateName,
                target,
                radiusPreset,
                squadMode,
                allegiance,
                isCreatureTemplate,
                creatureAgePreset,
                requestedAge0To1,
                quantity,
                targetFaction,
                desiredFaction,
                emptyFaction,
                createFaction,
                targetPlatoon);
            return false;
        }
    }

    if (!addToTargetSquad)
    {
        TryResolveEmptySpawnFaction(&emptyFaction);
    }

    createFaction = addToTargetSquad ? desiredFaction : (emptyFaction ? emptyFaction : desiredFaction);
    if (!createFaction)
    {
        outResult->message = "spawn faction unavailable";
        LogSpawnInvestigationReject(
            "spawn_faction_unavailable",
            templateName,
            target,
            radiusPreset,
            squadMode,
            allegiance,
            isCreatureTemplate,
            creatureAgePreset,
            requestedAge0To1,
            quantity,
            targetFaction,
            desiredFaction,
            emptyFaction,
            createFaction,
            targetPlatoon);
        return false;
    }

    LogSpawnInvestigationBegin(
        templateName,
        target,
        radiusPreset,
        squadMode,
        allegiance,
        isCreatureTemplate,
        creatureAgePreset,
        requestedAge0To1,
        quantity,
        targetFaction,
        desiredFaction,
        emptyFaction,
        createFaction,
        targetPlatoon);

    ActivePlatoon* spawnGroupContainer = addToTargetSquad ? targetPlatoon : 0;

    for (int spawnIndex = 0; spawnIndex < quantity; ++spawnIndex)
    {
        bool placementFound = false;
        int placementAttemptsUsed = 0;
        Ogre::Vector3 requestedPosition(0.0f, 0.0f, 0.0f);
        Ogre::Vector3 resolvedPosition(0.0f, 0.0f, 0.0f);
        for (int attempt = 0; attempt < kSpawnTemplateMaxPlacementAttemptsPerUnit; ++attempt)
        {
            if (TryResolveSpawnPlacementPosition(
                    target,
                    spawnIndex + attempt,
                    radiusPreset,
                    &requestedPosition,
                    &resolvedPosition))
            {
                placementFound = true;
                placementAttemptsUsed = attempt + 1;
                break;
            }
        }

        if (!placementFound)
        {
            if (g_developerMode)
            {
                std::stringstream line;
                line << "[investigate][spawn][placement_failed] template_name=\"" << SanitizeLogValue(templateName) << "\""
                     << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\""
                     << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
                     << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
                     << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
                     << " creature_template=" << (isCreatureTemplate ? "true" : "false")
                     << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
                     << " requested_age_0to1=" << requestedAge0To1
                     << " spawn_index=" << spawnIndex
                     << " attempts=" << kSpawnTemplateMaxPlacementAttemptsPerUnit;
                AppendCharacterProbeLogFields(line, "target", target);
                AppendFactionLogFields(line, "desired_faction", desiredFaction);
                AppendFactionLogFields(line, "create_faction", createFaction);
                LogInfoLine(line.str());
            }
            continue;
        }

        ++outResult->validSpawnCount;

        ActivePlatoon* spawnGroupContainerBeforePrepare = spawnGroupContainer;
        RootObjectContainer* createOwnerContainer = addToTargetSquad ? targetPlatoon : 0;
        Character* spawnedCharacter = TryCreateSpawnedCharacter(
            templateData,
            createFaction,
            createOwnerContainer,
            resolvedPosition,
            requestedAge0To1);
        bool usedTargetPlatoonFallback = false;
        if (!spawnedCharacter)
        {
            ++outResult->creationFailureCount;
            if (g_developerMode)
            {
                std::stringstream line;
                line << "[investigate][spawn][create_failed] template_name=\"" << SanitizeLogValue(templateName) << "\""
                     << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\""
                     << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
                     << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
                     << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
                     << " creature_template=" << (isCreatureTemplate ? "true" : "false")
                     << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
                     << " requested_age_0to1=" << requestedAge0To1
                     << " spawn_index=" << spawnIndex
                     << " placement_attempts_used=" << placementAttemptsUsed
                     << " requested_x=" << requestedPosition.x
                     << " requested_y=" << requestedPosition.y
                     << " requested_z=" << requestedPosition.z
                     << " resolved_x=" << resolvedPosition.x
                     << " resolved_y=" << resolvedPosition.y
                     << " resolved_z=" << resolvedPosition.z
                     << " create_owner_container_ptr=" << FormatPointerValue(createOwnerContainer);
                AppendCharacterProbeLogFields(line, "target", target);
                AppendFactionLogFields(line, "desired_faction", desiredFaction);
                AppendFactionLogFields(line, "create_faction", createFaction);
                AppendPlatoonLogFields(line, "resolved_target_platoon", targetPlatoon);
                LogInfoLine(line.str());
            }
            continue;
        }

        Faction* spawnedFactionBeforeAssign = 0;
        TryResolveCharacterFaction(spawnedCharacter, &spawnedFactionBeforeAssign);
        ActivePlatoon* spawnedPlatoonBeforePrepare = TryGetCharacterActivePlatoon(spawnedCharacter);
        bool spawnedWithPlayerBeforeAssign = false;
        const bool hasSpawnedWithPlayerBeforeAssign =
            TryResolveCharacterWithPlayer(spawnedCharacter, &spawnedWithPlayerBeforeAssign);
        bool appliedCreatureAge = false;
        if (isCreatureTemplate)
        {
            appliedCreatureAge = TryApplySpawnedCharacterAge(spawnedCharacter, requestedAge0To1);
        }

        TryAssignSpawnedCharacterFaction(
            spawnedCharacter,
            desiredFaction,
            addToTargetSquad ? targetPlatoon : spawnGroupContainer);
        TryFinalizeSpawnedCharacterPosition(spawnedCharacter);

        Faction* spawnedFactionAfterAssign = 0;
        TryResolveCharacterFaction(spawnedCharacter, &spawnedFactionAfterAssign);
        ActivePlatoon* spawnedPlatoonAfterAssign = TryGetCharacterActivePlatoon(spawnedCharacter);
        bool spawnedWithPlayerAfterAssign = false;
        const bool hasSpawnedWithPlayerAfterAssign =
            TryResolveCharacterWithPlayer(spawnedCharacter, &spawnedWithPlayerAfterAssign);
        bool playerEnemyOfSpawnAfterAssign = false;
        const bool hasPlayerEnemyOfSpawnAfterAssign =
            TryResolvePlayerEnemyState(spawnedCharacter, &playerEnemyOfSpawnAfterAssign);
        float spawnedAge0To1AfterAssign = -1.0f;
        const bool hasSpawnedAge0To1AfterAssign =
            TryResolveCharacterAge0To1(spawnedCharacter, &spawnedAge0To1AfterAssign);
        bool localHostilityApplied = false;
        if (allegiance == SpawnTemplateAllegiance_Hostile)
        {
            localHostilityApplied = TryPrimeLocalSpawnHostility(
                templateName,
                spawnedCharacter,
                target,
                radiusPreset,
                squadMode);
            if (!localHostilityApplied)
            {
                ++outResult->hostilityFailureCount;
            }
        }

        if (usedTargetPlatoonFallback)
        {
            outResult->usedTargetPlatoonFallback = true;
        }

        if (g_developerMode)
        {
            std::stringstream line;
            line << "[investigate][spawn] template_name=\"" << SanitizeLogValue(templateName) << "\""
                 << " target_name=\"" << SanitizeLogValue(SafeCharacterName(target)) << "\""
                 << " allegiance=\"" << SanitizeLogValue(SpawnTemplateAllegianceToLabel(allegiance)) << "\""
                 << " radius=\"" << SanitizeLogValue(SpawnTemplateRadiusPresetToLabel(radiusPreset)) << "\""
                 << " mode=\"" << SanitizeLogValue(SpawnTemplateSquadModeToLabel(squadMode)) << "\""
                 << " creature_template=" << (isCreatureTemplate ? "true" : "false")
                 << " creature_age_preset=\"" << SanitizeLogValue(SpawnCreatureAgePresetToLabel(creatureAgePreset)) << "\""
                 << " requested_age_0to1=" << requestedAge0To1
                 << " spawn_index=" << spawnIndex
                 << " placement_attempts_used=" << placementAttemptsUsed
                 << " requested_x=" << requestedPosition.x
                 << " requested_y=" << requestedPosition.y
                 << " requested_z=" << requestedPosition.z
                 << " resolved_x=" << resolvedPosition.x
                 << " resolved_y=" << resolvedPosition.y
                 << " resolved_z=" << resolvedPosition.z
                 << " used_target_platoon_fallback=" << (usedTargetPlatoonFallback ? "true" : "false")
                 << " target_platoon_ptr=" << FormatPointerValue(targetPlatoon)
                 << " create_owner_container_ptr=" << FormatPointerValue(createOwnerContainer)
                 << " spawn_group_container_before_ptr=" << FormatPointerValue(spawnGroupContainerBeforePrepare)
                 << " spawn_group_container_after_ptr=" << FormatPointerValue(spawnGroupContainer)
                 << " spawned_platoon_before_ptr=" << FormatPointerValue(spawnedPlatoonBeforePrepare)
                 << " spawned_platoon_after_ptr=" << FormatPointerValue(spawnedPlatoonAfterAssign)
                 << " local_hostility_applied=" << (localHostilityApplied ? "true" : "false")
                 << " created_in_target_platoon="
                 << ((spawnedPlatoonBeforePrepare && spawnedPlatoonBeforePrepare == targetPlatoon) ? "true" : "false")
                 << " final_in_target_platoon="
                 << ((spawnedPlatoonAfterAssign && spawnedPlatoonAfterAssign == targetPlatoon) ? "true" : "false")
                 << " final_in_spawn_group="
                 << ((spawnedPlatoonAfterAssign && spawnedPlatoonAfterAssign == spawnGroupContainer) ? "true" : "false")
                 << " applied_creature_age=" << (appliedCreatureAge ? "true" : "false")
                 << " spawned_age_0to1_after_assign="
                 << (hasSpawnedAge0To1AfterAssign ? spawnedAge0To1AfterAssign : -1.0f)
                 << " spawned_with_player_before_assign="
                 << FormatOptionalBoolLogValue(hasSpawnedWithPlayerBeforeAssign, spawnedWithPlayerBeforeAssign)
                 << " spawned_with_player_after_assign="
                 << FormatOptionalBoolLogValue(hasSpawnedWithPlayerAfterAssign, spawnedWithPlayerAfterAssign)
                 << " player_enemy_of_spawn_after_assign="
                 << FormatOptionalBoolLogValue(hasPlayerEnemyOfSpawnAfterAssign, playerEnemyOfSpawnAfterAssign);
            AppendCharacterProbeLogFields(line, "target", target);
            AppendCharacterProbeLogFields(line, "spawned", spawnedCharacter);
            AppendFactionLogFields(line, "resolved_target_faction", targetFaction);
            AppendFactionLogFields(line, "desired_faction", desiredFaction);
            AppendFactionLogFields(line, "create_faction", createFaction);
            AppendFactionLogFields(line, "spawned_faction_before_assign", spawnedFactionBeforeAssign);
            AppendFactionLogFields(line, "spawned_faction_after_assign", spawnedFactionAfterAssign);
            AppendPlatoonLogFields(line, "resolved_target_platoon", targetPlatoon);
            AppendPlatoonLogFields(line, "spawn_group_container_before_details", spawnGroupContainerBeforePrepare);
            AppendPlatoonLogFields(line, "spawn_group_container_after_details", spawnGroupContainer);
            AppendPlatoonLogFields(line, "spawned_platoon_before_assign_details", spawnedPlatoonBeforePrepare);
            AppendPlatoonLogFields(line, "spawned_platoon_after_assign_details", spawnedPlatoonAfterAssign);
            LogInfoLine(line.str());
        }

        ++outResult->spawnedCount;
    }

    outResult->success = outResult->spawnedCount > 0 && outResult->hostilityFailureCount <= 0;
    if (outResult->spawnedCount <= 0)
    {
        if (outResult->validSpawnCount <= 0)
        {
            outResult->message = "no valid spawn positions found";
        }
        else
        {
            outResult->message = "character creation failed";
        }
        return true;
    }

    if (outResult->hostilityFailureCount > 0)
    {
        std::stringstream message;
        message << "local hostility failed for " << outResult->hostilityFailureCount << " spawned character";
        if (outResult->hostilityFailureCount != 1)
        {
            message << "s";
        }
        outResult->message = message.str();
        return true;
    }

    if (outResult->spawnedCount == quantity)
    {
        outResult->message = "spawned all requested characters";
    }
    else
    {
        std::stringstream message;
        message << "spawned " << outResult->spawnedCount << " of " << quantity << " requested characters";
        outResult->message = message.str();
    }

    return true;
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
    const bool hasSelectedTemplate = TryResolveSelectedSpawnTemplate(&templateData, 0, 0) && templateData != 0;
    const bool hasValidQuantity = TryGetSpawnTemplateQuantity(&quantity);
    const bool hasSupportedMode = IsSpawnTemplateModeAllowed(squadMode, allegiance);
    g_spawnCharactersButton->setEnabled(hasTarget && hasSelectedTemplate && hasValidQuantity && hasSupportedMode);
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
            creatureAgePreset,
            quantity,
            &applyResult))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_templates\" success=false reason=\"apply_failed\""
               << " template_name=\"" << SanitizeLogValue(displayName) << "\""
               << " quantity=" << quantity
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
               << " (" << SpawnTemplateAllegianceToLabel(allegiance)
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
               << " (" << SpawnTemplateAllegianceToLabel(allegiance)
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
               << " (" << SpawnTemplateAllegianceToLabel(allegiance)
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
