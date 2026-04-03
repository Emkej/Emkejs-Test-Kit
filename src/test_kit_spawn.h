#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
enum SpawnTemplateCategory
{
    SpawnTemplateCategory_All = 0,
    SpawnTemplateCategory_Characters = 1,
    SpawnTemplateCategory_Creatures = 2
};

enum SpawnTemplateRadiusPreset
{
    SpawnTemplateRadius_Close = 0,
    SpawnTemplateRadius_Normal = 1,
    SpawnTemplateRadius_Wide = 2
};

enum SpawnTemplateAllegiance
{
    SpawnTemplateAllegiance_SameAsTarget = 0,
    SpawnTemplateAllegiance_FriendlyPlayer = 1,
    SpawnTemplateAllegiance_Neutral = 2,
    SpawnTemplateAllegiance_Hostile = 3
};

enum SpawnTemplateFactionMode
{
    SpawnTemplateFactionMode_None = 0,
    SpawnTemplateFactionMode_Custom = 1
};

enum SpawnTemplateSquadMode
{
    SpawnTemplateSquadMode_SeparateSquad = 0,
    SpawnTemplateSquadMode_AddToTargetSquad = 1
};

enum SpawnCreatureAgePreset
{
    SpawnCreatureAge_Pup = 0,
    SpawnCreatureAge_Young = 1,
    SpawnCreatureAge_Adult = 2
};

struct SpawnTemplateOption
{
    std::string displayName;
    std::string listLabel;
    std::string summaryLabel;
    std::string searchTextUpper;
    SpawnTemplateCategory category;
    GameData* templateData;
};

struct SpawnTemplateApplyResult
{
    SpawnTemplateApplyResult()
        : success(false)
        , requestedCount(0)
        , spawnedCount(0)
        , validSpawnCount(0)
        , creationFailureCount(0)
        , hostilityFailureCount(0)
        , usedTargetPlatoonFallback(false)
    {
    }

    bool success;
    int requestedCount;
    int spawnedCount;
    int validSpawnCount;
    int creationFailureCount;
    int hostilityFailureCount;
    bool usedTargetPlatoonFallback;
    std::string message;
};

struct SpawnTemplateFactionSelection
{
    SpawnTemplateFactionSelection()
        : mode(SpawnTemplateFactionMode_None)
        , customFaction(0)
    {
    }

    SpawnTemplateFactionMode mode;
    Faction* customFaction;
    std::string customFactionQuery;
    std::string customFactionLabel;
};

extern const int kSpawnTemplateQuantityMax;

void EnsureSpawnTemplateOptionsLoaded();
const char* SpawnTemplateFactionModeToLabel(SpawnTemplateFactionMode factionMode);
SpawnTemplateFactionMode GetSelectedSpawnTemplateFactionMode();
bool TryResolveSelectedSpawnTemplateFactionSelection(
    SpawnTemplateFactionSelection* outSelection,
    std::string* outErrorMessage);
std::string DescribeSpawnTemplateFactionSelection(const SpawnTemplateFactionSelection& selection);
void RefreshSpawnTemplateList();
void RefreshSpawnCreatureAgeControlState();
void RefreshSpawnFactionControlState();
void RefreshSpawnButtonState();
void RefreshSpawnPreviewText();
void ResetSpawnTargetAnchor();
bool ShouldShowSpawnCustomFactionControls();
void OnSpawnSearchTextChanged(MyGUI::EditBox*);
void OnSpawnCategoryChanged(MyGUI::ComboBox*, size_t);
void OnSpawnQuantityTextChanged(MyGUI::EditBox*);
void OnSpawnAllegianceChanged(MyGUI::ComboBox*, size_t);
void OnSpawnFactionModeChanged(MyGUI::ComboBox*, size_t);
void OnSpawnRadiusChanged(MyGUI::ComboBox*, size_t);
void OnSpawnCustomFactionTextChanged(MyGUI::EditBox*);
void OnSpawnCustomFactionResultsSelectionChanged(MyGUI::ListBox*, size_t);
void OnSpawnCreatureAgeChanged(MyGUI::ComboBox*, size_t);
void OnSpawnModeChanged(MyGUI::ComboBox*, size_t);
void OnSpawnResultsSelectionChanged(MyGUI::ListBox*, size_t);
void OnSpawnCharactersButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
}
