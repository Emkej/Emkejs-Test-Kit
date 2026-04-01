#include <Debug.h>

#include <emc/mod_hub_client.h>

#include "src/test_kit_config.h"
#include "src/test_kit_health.h"
#include "src/test_kit_inventory.h"
#include "src/test_kit_panel.h"
#include "src/test_kit_spawn.h"
#include "src/test_kit_stats.h"
#include "src/test_kit_teleport.h"

#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/Damages.h>
#include <kenshi/Dialogue.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/InputHandler.h>
#include <kenshi/Item.h>
#include <kenshi/Kenshi.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Platoon.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/SaveManager.h>
#include <kenshi/SensoryData.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_ComboBox.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>
#include <ois/OISKeyboard.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cctype>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace test_kit
{
const char* kPluginName = "Emkejs-Test-Kit";
const char* kConfigFileName = "mod-config.json";
const char* kDeveloperModeConfigKey = "developer_mode";
const char* kSavedLocationsConfigKey = "saved_locations";
const char* kDefaultTogglePanelKey = "D";
extern const int kPanelLeft = 18;
extern const int kPanelTop = 140;
int kPanelWidth = 360;
extern const int kPanelWidthDefault = 360;
extern const int kPanelWidthLowerBound = 360;
extern const int kPanelWidthUpperBound = 560;
extern const int kPanelExpandedHeight = 708;
extern const int kPanelMinExpandedHeightDefault = 320;
extern const int kPanelExpandedHeightLowerBound = 260;
extern const int kPanelExpandedHeightUpperBound = 920;
extern const int kPanelCollapsedHeight = 42;
extern const int kPanelViewportPadding = 16;
extern const int kPanelDragThreshold = 3;
extern const int kPanelHeaderHeight = 38;
extern const int kPanelBodyOverlapDefault = 6;
extern const int kPanelBodyOverlapLowerBound = 0;
extern const int kPanelBodyOverlapUpperBound = 8;
extern const int kPanelBodyScrollPadding = 20;
extern const int kPanelBodyBottomPadding = 18;
extern const int kPanelStatusGap = 20;
extern const int kPanelEdgeSnapDistance = 12;
extern const int kPanelMinimumVisibleWidth = 48;
extern const int kPanelMinimumVisibleHeight = 42;
extern const int kPanelHeaderTitleFontHeightDefault = 24;
extern const int kPanelHeaderTitleFontHeightLowerBound = 14;
extern const int kPanelHeaderTitleFontHeightUpperBound = 30;
extern const int kPanelCollapseButtonSizeDefault = 28;
extern const int kPanelCloseButtonSizeDefault = 28;
extern const int kPanelHeaderButtonSizeLowerBound = 18;
extern const int kPanelHeaderButtonSizeUpperBound = 32;
extern const int kPanelHeaderButtonGap = 6;
extern const int kPanelHeaderButtonRightPadding = 10;
bool g_runtimePanelPositionSet = false;
int g_runtimePanelLeft = kPanelLeft;
int g_runtimePanelTop = kPanelTop;
bool g_panelDragging = false;
bool g_panelDragMoved = false;
int g_panelDragLastMouseX = 0;
int g_panelDragLastMouseY = 0;
int g_panelDragMovedDistance = 0;
bool g_forceDyingArmed = false;
DWORD g_forceDyingArmedAtMs = 0;
std::string g_lastStatusMessage = "Ready";
TargetSnapshot g_lastTargetSnapshot;
bool g_hasLastTargetSnapshot = false;
PanelTab g_activePanelTab = PanelTab_Health;

PlayerInterface* g_lastPlayerInterface = 0;
bool g_loggedPanelCreateFailure = false;

MyGUI::Widget* g_panel = 0;
MyGUI::Button* g_headerBackground = 0;
MyGUI::Widget* g_headerFrame = 0;
MyGUI::TextBox* g_headerTitleText = 0;
MyGUI::Button* g_collapseButton = 0;
MyGUI::Button* g_closeButton = 0;
MyGUI::Button* g_bodyFrame = 0;
MyGUI::ScrollView* g_bodyScrollView = 0;
MyGUI::TextBox* g_targetSectionText = 0;
MyGUI::TextBox* g_targetNameText = 0;
MyGUI::TextBox* g_targetFactionText = 0;
MyGUI::TextBox* g_targetAlignmentText = 0;
MyGUI::TextBox* g_targetMembershipText = 0;
MyGUI::TextBox* g_targetStateText = 0;
MyGUI::TextBox* g_noTargetText = 0;
MyGUI::Button* g_healthTabButton = 0;
MyGUI::Button* g_statsTabButton = 0;
MyGUI::Button* g_teleportTabButton = 0;
MyGUI::Button* g_inventoryTabButton = 0;
MyGUI::Button* g_spawnTabButton = 0;
MyGUI::TextBox* g_statesSectionText = 0;
MyGUI::Button* g_fullRestoreButton = 0;
MyGUI::Button* g_forceUnconsciousButton = 0;
MyGUI::Button* g_forcePlayingDeadButton = 0;
MyGUI::TextBox* g_limbDamageSectionText = 0;
MyGUI::Button* g_damageLeftArmButton = 0;
MyGUI::Button* g_damageRightArmButton = 0;
MyGUI::Button* g_damageLeftLegButton = 0;
MyGUI::Button* g_damageRightLegButton = 0;
MyGUI::TextBox* g_statsSectionText = 0;
MyGUI::TextBox* g_statsScopeText = 0;
MyGUI::Button* g_statsApplyToAllButton = 0;
MyGUI::TextBox* g_statsClipboardText = 0;
MyGUI::Button* g_statsCopyButton = 0;
MyGUI::Button* g_statsPasteButton = 0;
MyGUI::TextBox* g_statsSectionFilterText = 0;
MyGUI::Button* g_statsAllSectionButton = 0;
MyGUI::Button* g_statsCommonSectionButton = 0;
MyGUI::Button* g_statsCoreSectionButton = 0;
MyGUI::Button* g_statsUtilitySectionButton = 0;
MyGUI::Button* g_statsCombatSectionButton = 0;
MyGUI::Button* g_statsWeaponsSectionButton = 0;
MyGUI::Button* g_statsLaborSectionButton = 0;
MyGUI::TextBox* g_statsSearchLabelText = 0;
MyGUI::EditBox* g_statsSearchEdit = 0;
MyGUI::TextBox* g_statsResultCountText = 0;
MyGUI::ListBox* g_statsResultsList = 0;
MyGUI::TextBox* g_statsSelectedSummaryText = 0;
MyGUI::TextBox* g_statsCurrentValueText = 0;
MyGUI::TextBox* g_statsInputLabelText = 0;
MyGUI::EditBox* g_statsInputEdit = 0;
MyGUI::Button* g_statsSetButton = 0;
MyGUI::Button* g_statsAddButton = 0;
MyGUI::Button* g_statsSubtractButton = 0;
MyGUI::TextBox* g_statsPreviewText = 0;
MyGUI::TextBox* g_teleportSectionText = 0;
MyGUI::TextBox* g_saveLocationNameLabelText = 0;
MyGUI::EditBox* g_saveLocationNameEdit = 0;
MyGUI::Button* g_saveSelectedLocationButton = 0;
MyGUI::TextBox* g_savedLocationsSectionText = 0;
MyGUI::Button* g_savedLocationsCollapseButton = 0;
MyGUI::Widget* g_savedLocationsRowsRoot = 0;
MyGUI::TextBox* g_savedLocationSearchLabelText = 0;
MyGUI::EditBox* g_savedLocationSearchEdit = 0;
MyGUI::ListBox* g_savedLocationsListBox = 0;
MyGUI::TextBox* g_savedLocationsEmptyText = 0;
MyGUI::Button* g_savedLocationTeleportButton = 0;
MyGUI::Button* g_savedLocationPinButton = 0;
MyGUI::Button* g_savedLocationRenameButton = 0;
MyGUI::Button* g_savedLocationDeleteButton = 0;
MyGUI::TextBox* g_inventorySectionText = 0;
MyGUI::TextBox* g_moneyAmountLabelText = 0;
MyGUI::EditBox* g_moneyAmountEdit = 0;
MyGUI::Button* g_addMoneyButton = 0;
MyGUI::TextBox* g_spawnFoodSectionText = 0;
MyGUI::TextBox* g_itemCategoryLabelText = 0;
MyGUI::ComboBox* g_itemCategoryDropdown = 0;
MyGUI::TextBox* g_itemSearchLabelText = 0;
MyGUI::EditBox* g_itemSearchEdit = 0;
MyGUI::ListBox* g_itemSearchResultsList = 0;
MyGUI::TextBox* g_itemQuantityLabelText = 0;
MyGUI::EditBox* g_itemQuantityEdit = 0;
MyGUI::Button* g_spawnItemButton = 0;
MyGUI::TextBox* g_spawnSectionText = 0;
MyGUI::TextBox* g_spawnCategoryLabelText = 0;
MyGUI::ComboBox* g_spawnCategoryDropdown = 0;
MyGUI::TextBox* g_spawnSearchLabelText = 0;
MyGUI::EditBox* g_spawnSearchEdit = 0;
MyGUI::TextBox* g_spawnResultCountText = 0;
MyGUI::ListBox* g_spawnResultsList = 0;
MyGUI::TextBox* g_spawnSelectedSummaryText = 0;
MyGUI::TextBox* g_spawnQuantityLabelText = 0;
MyGUI::EditBox* g_spawnQuantityEdit = 0;
MyGUI::TextBox* g_spawnAllegianceLabelText = 0;
MyGUI::ComboBox* g_spawnAllegianceDropdown = 0;
MyGUI::TextBox* g_spawnRadiusLabelText = 0;
MyGUI::ComboBox* g_spawnRadiusDropdown = 0;
MyGUI::TextBox* g_spawnCreatureAgeLabelText = 0;
MyGUI::ComboBox* g_spawnCreatureAgeDropdown = 0;
MyGUI::TextBox* g_spawnModeLabelText = 0;
MyGUI::ComboBox* g_spawnModeDropdown = 0;
MyGUI::TextBox* g_spawnPreviewText = 0;
MyGUI::Button* g_spawnCharactersButton = 0;
MyGUI::TextBox* g_dangerousSectionText = 0;
MyGUI::Button* g_forceDyingButton = 0;
MyGUI::TextBox* g_statusText = 0;

std::vector<int> g_filteredStatsRegistryIndexes;
std::vector<StatsClipboardEntry> g_statsClipboardEntries;
StatsEnumerated g_selectedStatsStat = STAT_NONE;
bool g_statsApplyToAllSelected = false;
StatsSectionFilter g_activeStatsSectionFilter = StatsSectionFilter_All;
std::string g_statsClipboardSourceName;

void (*PlayerInterface_updateUT_orig)(PlayerInterface*) = 0;
void (*SaveManager_loadByInfo_orig)(SaveManager*, const SaveInfo&, bool) = 0;
void (*SaveManager_loadByName_orig)(SaveManager*, const std::string&) = 0;

const char* ResolvePanelHeaderTitleFontName(int fontHeight);
void ApplyPanelHeaderTitleFont();
void ApplyPanelLayout();
int GetSelectedCharacterCount(PlayerInterface* player);
bool HasPrimarySelectedCharacter(PlayerInterface* player);
Character* TryGetPrimarySelectedCharacter(PlayerInterface* player);
int ClampIntValue(int value, int minValue, int maxValue);
int ClampPanelWidthValue(int value);
void ConfigureTextWidget(MyGUI::TextBox* widget);
void SetActivePanelTab(PanelTab tab);
void CreatePanelWidgets();
void DestroyPanel();
void RefreshStatsUi(PlayerInterface* player);
void OnStatsTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsAllSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCommonSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCoreSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsUtilitySectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCombatSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsWeaponsSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsLaborSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
bool TryResolveSelectedStatsEntry(const StatsRegistryEntry** entryOut);
void OnStatsSearchTextChanged(MyGUI::EditBox*);
void OnStatsResultsSelectionChanged(MyGUI::ListBox*, size_t);
void OnStatsInputTextChanged(MyGUI::EditBox*);
void OnStatsApplyToAllButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCopyButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsPasteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsSetButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsAddButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsSubtractButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
bool TryApplyStatsEditToCharacter(
    Character* character,
    const StatsRegistryEntry& entry,
    StatsEditOperation operation,
    int inputValue,
    int* beforeValueOut,
    int* afterValueOut,
    bool* clampedOut);
float ComputeHorizontalDistance(const Ogre::Vector3& a, const Ogre::Vector3& b);
bool TryGetCharacterPositionSnapshot(Character* character, CharacterPositionSnapshot* outSnapshot);

bool IsSupportedVersion(KenshiLib::BinaryVersion& versionInfo)
{
    const unsigned int platform = versionInfo.GetPlatform();
    const std::string version = versionInfo.GetVersion();

    return platform != KenshiLib::BinaryVersion::UNKNOWN
        && (version == "1.0.65" || version == "1.0.68");
}

void LogInfoLine(const std::string& message)
{
    std::stringstream line;
    line << kPluginName << " INFO: " << message;
    DebugLog(line.str().c_str());
}

void LogWarnLine(const std::string& message)
{
    std::stringstream line;
    line << kPluginName << " WARN: " << message;
    ErrorLog(line.str().c_str());
}

void LogErrorLine(const std::string& message)
{
    std::stringstream line;
    line << kPluginName << " ERROR: " << message;
    ErrorLog(line.str().c_str());
}

bool ShouldLogDebug()
{
    return g_loggingLevel == LoggingLevel_Debug;
}

void LogDebugLine(const std::string& message)
{
    if (ShouldLogDebug())
    {
        LogInfoLine(message);
    }
}

std::string TrimAscii(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
    {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(start, end - start);
}

std::string ToUpperAscii(const std::string& value)
{
    std::string upper;
    upper.reserve(value.size());

    for (size_t index = 0; index < value.size(); ++index)
    {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(value[index]))));
    }

    return upper;
}

bool TryParsePositiveInt(const std::string& value, int* outValue)
{
    if (!outValue)
    {
        return false;
    }

    const std::string trimmed = TrimAscii(value);
    if (trimmed.empty())
    {
        return false;
    }

    for (size_t index = 0; index < trimmed.size(); ++index)
    {
        if (std::isdigit(static_cast<unsigned char>(trimmed[index])) == 0)
        {
            return false;
        }
    }

    long long parsed = 0;
    std::stringstream stream(trimmed);
    stream >> parsed;
    if (!stream || !stream.eof() || parsed <= 0 || parsed > std::numeric_limits<int>::max())
    {
        return false;
    }

    *outValue = static_cast<int>(parsed);
    return true;
}

bool TryParseNonNegativeInt(const std::string& value, int* outValue)
{
    if (!outValue)
    {
        return false;
    }

    const std::string trimmed = TrimAscii(value);
    if (trimmed.empty())
    {
        return false;
    }

    for (size_t index = 0; index < trimmed.size(); ++index)
    {
        if (std::isdigit(static_cast<unsigned char>(trimmed[index])) == 0)
        {
            return false;
        }
    }

    long long parsed = 0;
    std::stringstream stream(trimmed);
    stream >> parsed;
    if (!stream || !stream.eof() || parsed < 0 || parsed > std::numeric_limits<int>::max())
    {
        return false;
    }

    *outValue = static_cast<int>(parsed);
    return true;
}

std::string SanitizeLogValue(const std::string& value)
{
    std::string sanitized;
    sanitized.reserve(value.size());

    for (size_t index = 0; index < value.size(); ++index)
    {
        const char current = value[index];
        if (current == '"')
        {
            sanitized.push_back('\'');
            continue;
        }

        sanitized.push_back(current);
    }

    return sanitized;
}

const char* TargetSourceToUiLabel(TargetSource source)
{
    switch (source)
    {
    case TargetSource_Selected:
        return "Selected";
    case TargetSource_Hovered:
        return "Hovered";
    case TargetSource_Conversation:
        return "Conversation";
    default:
        return "None";
    }
}

const char* TargetSourceToLogLabel(TargetSource source)
{
    switch (source)
    {
    case TargetSource_Selected:
        return "selected";
    case TargetSource_Hovered:
        return "hovered";
    case TargetSource_Conversation:
        return "conversation";
    default:
        return "none";
    }
}

void ResetTargetSnapshot(TargetSnapshot* snapshot)
{
    if (!snapshot)
    {
        return;
    }

    snapshot->hasTarget = false;
    snapshot->source = TargetSource_None;
    snapshot->target = 0;
    snapshot->name.clear();
    snapshot->factionName.clear();
    snapshot->alignment.clear();
    snapshot->membership.clear();
    snapshot->stateLabel = "Unknown";
    snapshot->unconscious = false;
    snapshot->playingDead = false;
    snapshot->dying = false;
    snapshot->dead = false;
}

bool AreTargetSnapshotsEqual(const TargetSnapshot& left, const TargetSnapshot& right)
{
    return left.hasTarget == right.hasTarget
        && left.source == right.source
        && left.target == right.target
        && left.name == right.name
        && left.factionName == right.factionName
        && left.alignment == right.alignment
        && left.membership == right.membership
        && left.stateLabel == right.stateLabel
        && left.unconscious == right.unconscious
        && left.playingDead == right.playingDead
        && left.dying == right.dying
        && left.dead == right.dead;
}

void RefreshStatusWidget()
{
    if (!g_statusText)
    {
        return;
    }

    std::stringstream caption;
    caption << "Status: " << g_lastStatusMessage;
    g_statusText->setCaption(caption.str());
}

void SetStatusMessage(const std::string& message)
{
    g_lastStatusMessage = message;
    RefreshStatusWidget();
}

bool HasPrimarySelectedCharacter(PlayerInterface* player)
{
    if (!player)
    {
        return false;
    }

    __try
    {
        return player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter() != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

Character* TryGetPrimarySelectedCharacter(PlayerInterface* player)
{
    if (!player)
    {
        return 0;
    }

    __try
    {
        if (player->selectedCharacter.isValid())
        {
            return player->selectedCharacter.getCharacter();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    return 0;
}

void AppendCharacterSnapshotLogFields(
    std::stringstream& line,
    const char* prefix,
    const CharacterPositionSnapshot& snapshot)
{
    const std::string fieldPrefix = prefix ? prefix : "snapshot";
    if (snapshot.hasPosition)
    {
        line << " " << fieldPrefix << "_position_x=" << snapshot.position.x
             << " " << fieldPrefix << "_position_y=" << snapshot.position.y
             << " " << fieldPrefix << "_position_z=" << snapshot.position.z;
    }
    if (snapshot.hasRawEntityPosition)
    {
        line << " " << fieldPrefix << "_raw_entity_x=" << snapshot.rawEntityPosition.x
             << " " << fieldPrefix << "_raw_entity_y=" << snapshot.rawEntityPosition.y
             << " " << fieldPrefix << "_raw_entity_z=" << snapshot.rawEntityPosition.z;
    }
    if (snapshot.hasRawPosition)
    {
        line << " " << fieldPrefix << "_raw_x=" << snapshot.rawPosition.x
             << " " << fieldPrefix << "_raw_y=" << snapshot.rawPosition.y
             << " " << fieldPrefix << "_raw_z=" << snapshot.rawPosition.z;
    }
    if (snapshot.hasTerrainHeight)
    {
        line << " " << fieldPrefix << "_terrain_height=" << snapshot.terrainHeight;
    }
}

bool TryGetCharacterPosition(Character* character, Ogre::Vector3* outPosition)
{
    if (!character || !outPosition)
    {
        return false;
    }

    CharacterPositionSnapshot snapshot;

    __try
    {
        snapshot.position = character->getPosition();
        snapshot.hasPosition = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        snapshot.rawPosition = character->_getRawPosition();
        snapshot.hasRawPosition = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        snapshot.rawEntityPosition = character->getRawEntityPosition();
        snapshot.hasRawEntityPosition = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        snapshot.terrainHeight = character->getTerrainHeightPosition();
        snapshot.hasTerrainHeight = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (snapshot.hasRawEntityPosition)
    {
        *outPosition = snapshot.rawEntityPosition;
        return true;
    }

    if (snapshot.hasPosition)
    {
        *outPosition = snapshot.position;
        return true;
    }

    if (snapshot.hasRawPosition)
    {
        *outPosition = snapshot.rawPosition;
        return true;
    }

    return false;
}

bool TryGetCharacterPositionSnapshot(Character* character, CharacterPositionSnapshot* outSnapshot)
{
    if (!character || !outSnapshot)
    {
        return false;
    }

    CharacterPositionSnapshot snapshot;

    __try
    {
        snapshot.position = character->getPosition();
        snapshot.hasPosition = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        snapshot.rawPosition = character->_getRawPosition();
        snapshot.hasRawPosition = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        snapshot.rawEntityPosition = character->getRawEntityPosition();
        snapshot.hasRawEntityPosition = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        snapshot.terrainHeight = character->getTerrainHeightPosition();
        snapshot.hasTerrainHeight = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    *outSnapshot = snapshot;
    return snapshot.hasPosition
        || snapshot.hasRawPosition
        || snapshot.hasRawEntityPosition;
}

bool TryGetCharacterTeleportReferencePosition(
    Character* character,
    bool useSpawnValidation,
    Ogre::Vector3* outPosition,
    const char** outSourceLabel)
{
    if (outSourceLabel)
    {
        *outSourceLabel = "unavailable";
    }

    if (!character || !outPosition)
    {
        return false;
    }

    CharacterPositionSnapshot snapshot;
    if (!TryGetCharacterPositionSnapshot(character, &snapshot))
    {
        return false;
    }

    if (!useSpawnValidation)
    {
        if (snapshot.hasRawEntityPosition)
        {
            *outPosition = snapshot.rawEntityPosition;
            if (outSourceLabel)
            {
                *outSourceLabel = "raw_entity";
            }
            return true;
        }
    }

    if (snapshot.hasPosition)
    {
        *outPosition = snapshot.position;
        if (outSourceLabel)
        {
            *outSourceLabel = "position";
        }
        return true;
    }

    if (snapshot.hasRawEntityPosition)
    {
        *outPosition = snapshot.rawEntityPosition;
        if (outSourceLabel)
        {
            *outSourceLabel = "raw_entity";
        }
        return true;
    }

    if (snapshot.hasRawPosition)
    {
        *outPosition = snapshot.rawPosition;
        if (outSourceLabel)
        {
            *outSourceLabel = "raw";
        }
        return true;
    }

    return false;
}

int ClampIntValue(int value, int minValue, int maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

int ClampPanelWidthValue(int value)
{
    return ClampIntValue(value, kPanelWidthLowerBound, kPanelWidthUpperBound);
}

bool TryGetSelectedTarget(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    Character* target = 0;
    __try
    {
        if (player->selectedObject && player->selectedObject.isValid())
        {
            target = player->selectedObject.getCharacter();
        }

        if (!target)
        {
            target = player->selectedCharacter.getCharacter();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetSelectedPlayerCharacter(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    Character* target = 0;
    __try
    {
        target = player->selectedCharacter.getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetHoveredTarget(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    bool hasMouseRightTarget = false;
    RootObject* root = 0;
    __try
    {
        hasMouseRightTarget = player->mouseRightTargetSet;
        root = player->mouseRightTarget;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!hasMouseRightTarget || !root)
    {
        return false;
    }

    Character* target = 0;
    __try
    {
        target = root->getHandle().getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetConversationTargetFromSpeaker(Character* speaker, Character** outTarget)
{
    if (!speaker || !outTarget)
    {
        return false;
    }

    Dialogue* dialogue = 0;
    bool hasEnded = true;
    Character* target = 0;
    __try
    {
        dialogue = speaker->dialogue;
        if (!dialogue)
        {
            return false;
        }

        hasEnded = dialogue->conversationHasEndedPrettyMuch();
        if (hasEnded)
        {
            return false;
        }

        target = dialogue->getConversationTarget().getCharacter();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!target)
    {
        return false;
    }

    *outTarget = target;
    return true;
}

bool TryGetConversationTarget(PlayerInterface* player, Character** outTarget)
{
    if (!player || !outTarget)
    {
        return false;
    }

    Character* selected = 0;
    if (TryGetSelectedPlayerCharacter(player, &selected) && TryGetConversationTargetFromSpeaker(selected, outTarget))
    {
        return true;
    }

    __try
    {
        const lektor<Character*>& playerCharacters = player->getAllPlayerCharacters();
        if (!playerCharacters.valid())
        {
            return false;
        }

        for (lektor<Character*>::const_iterator it = playerCharacters.begin(); it != playerCharacters.end(); ++it)
        {
            Character* speaker = *it;
            if (!speaker || speaker == selected)
            {
                continue;
            }

            if (TryGetConversationTargetFromSpeaker(speaker, outTarget))
            {
                return true;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return false;
}

bool ResolveInspectionTarget(PlayerInterface* player, Character** outTarget, TargetSource* outSource)
{
    if (!outTarget || !outSource)
    {
        return false;
    }

    *outTarget = 0;
    *outSource = TargetSource_None;

    Character* target = 0;
    if (TryGetSelectedTarget(player, &target))
    {
        *outTarget = target;
        *outSource = TargetSource_Selected;
        return true;
    }

    if (TryGetHoveredTarget(player, &target))
    {
        *outTarget = target;
        *outSource = TargetSource_Hovered;
        return true;
    }

    if (TryGetConversationTarget(player, &target))
    {
        *outTarget = target;
        *outSource = TargetSource_Conversation;
        return true;
    }

    return false;
}

std::string SafeCharacterName(Character* target)
{
    if (!target)
    {
        return "Unknown";
    }

    const std::string name = target->getName();
    if (name.empty())
    {
        return "Unknown";
    }

    return name;
}

bool TryResolveCharacterFaction(Character* character, Faction** outFaction)
{
    if (!outFaction)
    {
        return false;
    }

    *outFaction = 0;
    if (!character || !IsProbablyReadableEnginePointer(character))
    {
        return false;
    }

    if (character->owner)
    {
        *outFaction = character->owner;
        return true;
    }

    __try
    {
        *outFaction = character->getFaction();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outFaction = 0;
    }

    return *outFaction != 0;
}

bool TryResolveFactionName(Faction* faction, std::string* outName)
{
    if (!outName)
    {
        return false;
    }

    *outName = "Unknown";
    if (!faction || !IsProbablyReadableEnginePointer(faction))
    {
        return false;
    }

    __try
    {
        const std::string& name = faction->getName();
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

std::string SafeFactionName(Faction* faction)
{
    std::string name = "Unknown";
    TryResolveFactionName(faction, &name);
    return name;
}

std::string SafeFactionName(Character* target)
{
    Faction* faction = 0;
    if (!TryResolveCharacterFaction(target, &faction) || !faction)
    {
        return "Unknown";
    }

    return SafeFactionName(faction);
}

std::string FormatPointerValue(const void* pointer)
{
    std::stringstream stream;
    stream << pointer;
    return stream.str();
}

bool IsProbablyReadableEnginePointer(const void* pointer)
{
    return reinterpret_cast<std::uintptr_t>(pointer) >= 0x10000u;
}

float ComputeHorizontalDistance(const Ogre::Vector3& a, const Ogre::Vector3& b)
{
    const float deltaX = a.x - b.x;
    const float deltaZ = a.z - b.z;
    return std::sqrt((deltaX * deltaX) + (deltaZ * deltaZ));
}

bool TryResolveMembershipLabel(Character* target, std::string* outLabel)
{
    if (!target || !outLabel)
    {
        return false;
    }

    __try
    {
        *outLabel = target->isWithThePlayer() ? "Squad" : "Non-squad";
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TryResolveAlignmentLabel(PlayerInterface* player, Character* target, std::string* outLabel)
{
    if (!player || !target || !outLabel)
    {
        return false;
    }

    __try
    {
        if (target->isWithThePlayer())
        {
            *outLabel = "Ally";
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    __try
    {
        if (player->isEnemy(target))
        {
            *outLabel = "Enemy";
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    bool sawPlayerCharacter = false;
    bool sawAlly = false;
    __try
    {
        const lektor<Character*>& playerCharacters = player->getAllPlayerCharacters();
        if (playerCharacters.valid())
        {
            for (lektor<Character*>::const_iterator it = playerCharacters.begin(); it != playerCharacters.end(); ++it)
            {
                Character* playerCharacter = *it;
                if (!playerCharacter || playerCharacter == target)
                {
                    continue;
                }

                sawPlayerCharacter = true;
                if (target->isAlly(playerCharacter, true))
                {
                    sawAlly = true;
                    break;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (sawAlly)
    {
        *outLabel = "Ally";
        return true;
    }

    *outLabel = sawPlayerCharacter ? "Neutral" : "Unknown";
    return true;
}

void SetActionButtonsEnabled(bool enabled)
{
    if (g_fullRestoreButton)
    {
        g_fullRestoreButton->setEnabled(enabled);
    }
    if (g_forceUnconsciousButton)
    {
        g_forceUnconsciousButton->setEnabled(enabled);
    }
    if (g_forcePlayingDeadButton)
    {
        g_forcePlayingDeadButton->setEnabled(enabled);
    }
    if (g_damageLeftArmButton)
    {
        g_damageLeftArmButton->setEnabled(enabled);
    }
    if (g_damageRightArmButton)
    {
        g_damageRightArmButton->setEnabled(enabled);
    }
    if (g_damageLeftLegButton)
    {
        g_damageLeftLegButton->setEnabled(enabled);
    }
    if (g_damageRightLegButton)
    {
        g_damageRightLegButton->setEnabled(enabled);
    }
    if (g_forceDyingButton)
    {
        g_forceDyingButton->setEnabled(enabled);
    }
    if (g_addMoneyButton)
    {
        g_addMoneyButton->setEnabled(enabled);
    }
    if (g_spawnItemButton)
    {
        g_spawnItemButton->setEnabled(enabled);
    }
    if (g_spawnCharactersButton)
    {
        g_spawnCharactersButton->setEnabled(enabled);
    }
}

void SetSelectionActionButtonsEnabled(bool enabled)
{
    if (g_saveSelectedLocationButton)
    {
        g_saveSelectedLocationButton->setEnabled(enabled || !g_savedLocationRenameId.empty());
    }
}

int GetSelectedCharacterCount(PlayerInterface* player)
{
    if (!player)
    {
        return 0;
    }

    int selectedCharacterCount = 0;

    __try
    {
        const ogre_unordered_set<hand>::type& selectedCharacters = player->selectedCharacters;
        ogre_unordered_set<hand>::type::const_iterator it = selectedCharacters.begin();
        for (; it != selectedCharacters.end(); ++it)
        {
            if (it->getCharacter())
            {
                ++selectedCharacterCount;
            }
        }

        if (selectedCharacterCount <= 0 && player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter())
        {
            selectedCharacterCount = 1;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    return selectedCharacterCount;
}

void UpdateSelectionActionButtons(PlayerInterface* player)
{
    const bool hasSelectedCharacters = GetSelectedCharacterCount(player) > 0;
    SetSelectionActionButtonsEnabled(hasSelectedCharacters);

    if (g_saveSelectedLocationButton)
    {
        g_saveSelectedLocationButton->setEnabled(!g_savedLocationRenameId.empty() || HasPrimarySelectedCharacter(player));
    }

    RefreshSavedLocationActionButtons(player);
    if (g_activePanelTab == PanelTab_Stats || g_statsApplyToAllSelected)
    {
        RefreshStatsUi(player);
    }
    RefreshInventorySpawnButtonState();
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void ApplyTargetSnapshotToUi(const TargetSnapshot& snapshot)
{
    if (!g_targetNameText || !g_targetFactionText || !g_targetAlignmentText
        || !g_targetMembershipText || !g_targetStateText || !g_noTargetText)
    {
        return;
    }

    if (!snapshot.hasTarget)
    {
        g_targetNameText->setCaption("Name: No target");
        g_targetFactionText->setCaption("Faction: Unknown");
        g_targetAlignmentText->setCaption("Alignment: Unknown");
        g_targetMembershipText->setCaption("Membership: Unknown");
        g_targetStateText->setCaption("State: Unknown");
        g_noTargetText->setCaption("Source: None");
        SetActionButtonsEnabled(false);
        RefreshSpawnPreviewText();
        return;
    }

    g_targetNameText->setCaption("Name: " + snapshot.name);
    g_targetFactionText->setCaption("Faction: " + snapshot.factionName);
    g_targetAlignmentText->setCaption("Alignment: " + snapshot.alignment);
    g_targetMembershipText->setCaption("Membership: " + snapshot.membership);
    g_targetStateText->setCaption("State: " + snapshot.stateLabel);
    g_noTargetText->setCaption(std::string("Source: ") + TargetSourceToUiLabel(snapshot.source));
    SetActionButtonsEnabled(true);
    RefreshSpawnPreviewText();
}

void LogTargetSnapshotIfChanged(const TargetSnapshot& snapshot)
{
    if (g_hasLastTargetSnapshot && AreTargetSnapshotsEqual(g_lastTargetSnapshot, snapshot))
    {
        return;
    }

    std::stringstream line;
    line << "event=testkit_target_snapshot target_present=" << (snapshot.hasTarget ? "true" : "false");
    if (snapshot.hasTarget)
    {
        line << " source=\"" << TargetSourceToLogLabel(snapshot.source) << "\""
             << " name=\"" << SanitizeLogValue(snapshot.name) << "\""
             << " faction=\"" << SanitizeLogValue(snapshot.factionName) << "\""
             << " alignment=\"" << snapshot.alignment << "\""
             << " membership=\"" << snapshot.membership << "\""
             << " state=\"" << snapshot.stateLabel << "\""
             << " unconscious=" << (snapshot.unconscious ? "true" : "false")
             << " playing_dead=" << (snapshot.playingDead ? "true" : "false")
             << " dying=" << (snapshot.dying ? "true" : "false")
             << " dead=" << (snapshot.dead ? "true" : "false");
    }
    LogDebugLine(line.str());
}

void BuildTargetSnapshot(PlayerInterface* player, Character* target, TargetSource source, TargetSnapshot* snapshotOut)
{
    if (!snapshotOut)
    {
        return;
    }

    ResetTargetSnapshot(snapshotOut);
    if (!target)
    {
        return;
    }

    snapshotOut->hasTarget = true;
    snapshotOut->source = source;
    snapshotOut->target = target;

    snapshotOut->name = SafeCharacterName(target);
    snapshotOut->factionName = SafeFactionName(target);

    if (!TryResolveAlignmentLabel(player, target, &snapshotOut->alignment))
    {
        snapshotOut->alignment = "Unknown";
    }
    if (!TryResolveMembershipLabel(target, &snapshotOut->membership))
    {
        snapshotOut->membership = "Unknown";
    }
    if (!TryResolveStateSummary(
            target,
            &snapshotOut->stateLabel,
            &snapshotOut->unconscious,
            &snapshotOut->playingDead,
            &snapshotOut->dying,
            &snapshotOut->dead))
    {
        snapshotOut->stateLabel = "Unknown";
        snapshotOut->unconscious = false;
        snapshotOut->playingDead = false;
        snapshotOut->dying = false;
        snapshotOut->dead = false;
    }
}

void UpdateTargetInspection(PlayerInterface* player)
{
    EnsureInventoryFoodItemOptionsLoaded();

    TargetSnapshot snapshot;
    ResetTargetSnapshot(&snapshot);

    Character* target = 0;
    TargetSource source = TargetSource_None;
    if (ResolveInspectionTarget(player, &target, &source))
    {
        BuildTargetSnapshot(player, target, source, &snapshot);
    }

    if (g_forceDyingArmed
        && (!snapshot.hasTarget
            || (g_hasLastTargetSnapshot
                && (snapshot.target != g_lastTargetSnapshot.target || snapshot.source != g_lastTargetSnapshot.source))))
    {
        ClearForceDyingArm(snapshot.hasTarget ? "target_changed" : "target_lost", true);
    }

    ApplyTargetSnapshotToUi(snapshot);
    UpdateSelectionActionButtons(player);
    LogTargetSnapshotIfChanged(snapshot);
    g_lastTargetSnapshot = snapshot;
    g_hasLastTargetSnapshot = true;
}

bool TryRestoreRequestedSelectedSpawnTarget(PlayerInterface* player, Character* requestedTarget)
{
    if (!player || !requestedTarget)
    {
        return false;
    }

    auto TrySelectRequestedTarget = [](PlayerInterface* localPlayer, Character* localTarget) -> bool
    {
        if (!localPlayer || !localTarget)
        {
            return false;
        }

        __try
        {
            localPlayer->selectObject(localTarget, false);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    };

    if (!TrySelectRequestedTarget(player, requestedTarget))
    {
        return false;
    }

    Character* selectedTarget = 0;
    if (!TryGetSelectedTarget(player, &selectedTarget) || selectedTarget != requestedTarget)
    {
        return false;
    }

    TargetSnapshot snapshot;
    BuildTargetSnapshot(player, selectedTarget, TargetSource_Selected, &snapshot);
    if (!snapshot.hasTarget || snapshot.target != requestedTarget)
    {
        return false;
    }

    ApplyTargetSnapshotToUi(snapshot);
    UpdateSelectionActionButtons(player);
    LogTargetSnapshotIfChanged(snapshot);
    g_lastTargetSnapshot = snapshot;
    g_hasLastTargetSnapshot = true;
    return true;
}

void LogActionRequested(const char* actionId)
{
    std::stringstream requested;
    requested << "event=testkit_action_requested action=\"" << actionId << "\"";
    if (g_hasLastTargetSnapshot && g_lastTargetSnapshot.hasTarget)
    {
        requested << " source=\"" << TargetSourceToLogLabel(g_lastTargetSnapshot.source) << "\""
                  << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
    }
    LogInfoLine(requested.str());
}

void OnSaveLoadTransitionStart(const char* source)
{
    if (source)
    {
        std::stringstream line;
        line << "event=testkit_panel_destroyed reason=\"" << source << "\"";
        LogInfoLine(line.str());
    }

    DestroyPanel();
    g_lastPlayerInterface = 0;
    g_hotkeyPrevDown = false;
    g_lastStatusMessage = "Ready";
    ResetInventoryRuntimeState();
    ClearPendingDownedTeleportRestores();
    ResetTargetSnapshot(&g_lastTargetSnapshot);
    g_hasLastTargetSnapshot = false;
}

void PlayerInterface_updateUT_hook(PlayerInterface* thisptr)
{
    PlayerInterface_updateUT_orig(thisptr);

    TickModHubAttachRetry();

    if (!g_pluginEnabled)
    {
        if (g_panel)
        {
            DestroyPanel();
        }
        return;
    }

    TickPendingDownedTeleportRestores();
    TickPanelToggleHotkey();
    EnsurePanel(thisptr);
    TickInventorySearchFocusHotkey();
}

void SaveManager_loadByInfo_hook(SaveManager* thisptr, const SaveInfo& saveInfo, bool resetPos)
{
    OnSaveLoadTransitionStart("SaveManager::load(saveInfo,bool)");
    if (SaveManager_loadByInfo_orig)
    {
        SaveManager_loadByInfo_orig(thisptr, saveInfo, resetPos);
    }
}

void SaveManager_loadByName_hook(SaveManager* thisptr, const std::string& saveName)
{
    OnSaveLoadTransitionStart("SaveManager::load(name)");
    if (SaveManager_loadByName_orig)
    {
        SaveManager_loadByName_orig(thisptr, saveName);
    }
}
}

__declspec(dllexport) void startPlugin()
{
    using namespace test_kit;

    LogInfoLine("startPlugin()");

    KenshiLib::BinaryVersion versionInfo = KenshiLib::GetKenshiVersion();
    if (!IsSupportedVersion(versionInfo))
    {
        std::stringstream error;
        error << "unsupported Kenshi version/platform"
              << " version=" << versionInfo.GetVersion()
              << " platform=" << versionInfo.GetPlatform();
        LogErrorLine(error.str());
        return;
    }

    std::stringstream versionLine;
    versionLine << "supported Kenshi version detected: " << versionInfo.GetVersion();
    LogInfoLine(versionLine.str());

    LoadConfig();
    StartModHubClient();
    if (!g_pluginEnabled)
    {
        LogInfoLine("plugin disabled by config; hooks remain loaded for runtime re-enable");
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&PlayerInterface::updateUT),
        PlayerInterface_updateUT_hook,
        &PlayerInterface_updateUT_orig))
    {
        LogErrorLine("Could not hook PlayerInterface::updateUT");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(static_cast<void (SaveManager::*)(const SaveInfo&, bool)>(&SaveManager::load)),
        SaveManager_loadByInfo_hook,
        &SaveManager_loadByInfo_orig))
    {
        LogWarnLine("Could not hook SaveManager::load(SaveInfo,bool); panel teardown on load is reduced");
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
        KenshiLib::GetRealAddress(static_cast<void (SaveManager::*)(const std::string&)>(&SaveManager::load)),
        SaveManager_loadByName_hook,
        &SaveManager_loadByName_orig))
    {
        LogWarnLine("Could not hook SaveManager::load(std::string); panel teardown on load is reduced");
    }

    std::stringstream info;
    info << "panel framework initialized hotkey=\"" << g_hotkeyDisplay
         << "\" start_hidden=" << (g_panelHidden ? "true" : "false")
         << " start_collapsed=" << (g_panelCollapsed ? "true" : "false");
    LogInfoLine(info.str());

    if (ShouldLogDebug())
    {
        LogDebugLine("step 1 panel shell active; target inspection and state forcing remain unimplemented");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
    using namespace test_kit;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        char dllPath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0)
        {
            const std::string fullPath(dllPath);
            const std::string::size_type separator = fullPath.find_last_of("\\/");
            if (separator != std::string::npos)
            {
                g_configPath = fullPath.substr(0, separator) + "\\" + kConfigFileName;
            }
        }
    }

    return TRUE;
}
