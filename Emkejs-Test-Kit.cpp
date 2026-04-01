#include <Debug.h>

#include <emc/mod_hub_client.h>

#include "src/test_kit_config.h"
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
const DWORD kDangerArmTimeoutMs = 3000;
const float kForceUnconsciousDurationSeconds = 30.0f;
const float kForceDyingBloodOffset = 8.0f;
const float kForceDyingAliveBloodMargin = 1.0f;
const float kProbablyDyingBloodMax = 50.0f;
const float kLimbDamageFraction = 0.35f;
const float kMinimumLimbDamageAmount = 5.0f;
const float kFloatChangeEpsilon = 0.001f;
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

void UpdateForceDyingButtonCaption()
{
    if (!g_forceDyingButton)
    {
        return;
    }

    if (g_confirmDangerousActions && g_forceDyingArmed)
    {
        g_forceDyingButton->setCaption("Confirm Force Dying");
        return;
    }

    g_forceDyingButton->setCaption("Force Dying");
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

void ClearForceDyingArm(const char* reason, bool updateStatus)
{
    if (!g_forceDyingArmed)
    {
        return;
    }

    g_forceDyingArmed = false;
    g_forceDyingArmedAtMs = 0;
    UpdateForceDyingButtonCaption();

    if (reason)
    {
        std::stringstream line;
        line << "event=testkit_action_arm action=\"force_dying\" armed=false reason=\"" << reason << "\"";
        LogInfoLine(line.str());
    }

    if (updateStatus)
    {
        SetStatusMessage("Force Dying arm cleared");
    }
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

bool TryResolveStateSummary(
    Character* target,
    std::string* outLabel,
    bool* unconsciousOut,
    bool* playingDeadOut,
    bool* dyingOut,
    bool* deadOut)
{
    if (!target || !outLabel || !unconsciousOut || !playingDeadOut || !dyingOut || !deadOut)
    {
        return false;
    }

    bool unconscious = false;
    bool playingDead = false;
    bool dying = false;
    bool dead = false;
    bool recoveryComa = false;
    __try
    {
        const ProneState proneState = target->_currentProneState;
        const bool deadByCharacter = target->isDead();
        const bool deadByMedicalMethod = target->medical.isDead();
        const bool deadByMedicalFlag = target->medical.dead;
        dead = deadByCharacter || deadByMedicalMethod || deadByMedicalFlag;

        unconscious = target->medical.unconcious || proneState == PS_KO;
        playingDead = (proneState == PS_PLAYING_DEAD);
        if (!dead && unconscious && !playingDead)
        {
            const bool isProbablyDying = target->medical.isProbablyDying();
            const bool dyingByTrauma = target->medical.isInBloodlossTrauma();
            const bool sub50Ko = target->medical.sub50KO;
            const float bloodLevel = target->medical.blood;
            const float pointOfNoReturn = target->medical.pointOfNoReturn();
            const bool dyingByBloodThreshold = (bloodLevel <= pointOfNoReturn);
            const bool dyingByProbablyLowBlood = (isProbablyDying && bloodLevel <= kProbablyDyingBloodMax);
            const bool dyingByActiveBleed =
                isProbablyDying
                && (target->medical.currentBleedRate > 0.0f || target->medical.extraBloodLossFromBodyparts > 0.0f);
            const bool recoveryComaByCannotWake = (!target->medical.canGetUpWakeUp() && sub50Ko);
            const bool knockoutTimerElapsed = (target->medical.knockoutTimer <= 0.0f);

            recoveryComa = recoveryComaByCannotWake
                && knockoutTimerElapsed
                && !isProbablyDying
                && !dyingByBloodThreshold
                && !dyingByTrauma
                && !dyingByActiveBleed
                && !dyingByProbablyLowBlood;

            dying = dyingByBloodThreshold || sub50Ko;
            if (recoveryComa)
            {
                dying = false;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    *unconsciousOut = unconscious;
    *playingDeadOut = playingDead;
    *dyingOut = dying;
    *deadOut = dead;

    if (dead)
    {
        *outLabel = "Dead";
    }
    else if (dying)
    {
        *outLabel = "Dying";
    }
    else if (recoveryComa)
    {
        *outLabel = "Recovery Coma";
    }
    else if (playingDead)
    {
        *outLabel = "Playing Dead";
    }
    else if (unconscious)
    {
        *outLabel = "Unconscious";
    }
    else
    {
        *outLabel = "None detected";
    }

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

bool TryForceUnconscious(Character* target, bool alreadyUnconscious, float* knockoutTimerOut)
{
    if (!target)
    {
        return false;
    }

    if (knockoutTimerOut)
    {
        *knockoutTimerOut = 0.0f;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        if (!alreadyUnconscious)
        {
            medical->knockout(0.0f);
        }
        medical->knockoutForceTimer(kForceUnconsciousDurationSeconds);

        if (knockoutTimerOut)
        {
            *knockoutTimerOut = medical->knockoutTimer;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryFullRestore(Character* target, bool* fullyRestoredOut, float* bloodOut, float* maxBloodOut)
{
    if (fullyRestoredOut)
    {
        *fullyRestoredOut = false;
    }
    if (bloodOut)
    {
        *bloodOut = 0.0f;
    }
    if (maxBloodOut)
    {
        *maxBloodOut = 0.0f;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        target->healCompletely();
        target->playerWantsMeToGetUp = true;

        const ProneState proneState = target->_NV_getProneState();
        if (proneState == PS_KO || proneState == PS_PLAYING_DEAD || proneState == PS_CRIPPLED)
        {
            target->_NV_setProneState(PS_NORMAL);
        }

        const float maxBlood = medical->getMaxBlood();
        medical->blood = maxBlood;
        medical->knockoutTimer = 0.0f;
        medical->currentBleedRate = 0.0f;
        medical->extraBloodLossFromBodyparts = 0.0f;
        medical->crippled = false;
        medical->unconcious = false;
        medical->sub50KO = false;
        medical->bloodlossTrauma = false;
        medical->validateHealthValues();

        bool fullyRestored = true;
        const int partCount = medical->getPartCount();
        for (int index = 0; index < partCount; ++index)
        {
            MedicalSystem::HealthPartStatus* part = medical->getPart(static_cast<unsigned __int64>(index));
            if (!part)
            {
                fullyRestored = false;
                break;
            }

            part->updateDerivedHealths();

            if ((part->maxHealth() - part->flesh > kFloatChangeEpsilon)
                || (part->flesh - part->maxHealth() > kFloatChangeEpsilon)
                || (part->fleshStun > kFloatChangeEpsilon)
                || (part->wearDamage > kFloatChangeEpsilon)
                || (1.0f - part->derivedFleshHealthPercent > kFloatChangeEpsilon))
            {
                fullyRestored = false;
                break;
            }
        }

        medical->updateDamageState();
        medical->reassessCollapseMode(false, false);

        if (fullyRestoredOut)
        {
            *fullyRestoredOut = fullyRestored
                && (maxBlood - medical->blood <= kFloatChangeEpsilon)
                && (medical->currentBleedRate <= kFloatChangeEpsilon)
                && (medical->extraBloodLossFromBodyparts <= kFloatChangeEpsilon)
                && !medical->unconcious
                && !medical->sub50KO
                && !medical->bloodlossTrauma
                && !medical->dead;
        }
        if (bloodOut)
        {
            *bloodOut = medical->blood;
        }
        if (maxBloodOut)
        {
            *maxBloodOut = maxBlood;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryInvokeSelectedCharactersLayingLow(
    PlayerInterface* player,
    Character* target,
    bool* attemptedOut,
    bool* commandAcceptedOut)
{
    if (attemptedOut)
    {
        *attemptedOut = false;
    }
    if (commandAcceptedOut)
    {
        *commandAcceptedOut = false;
    }

    if (!player || !target)
    {
        return true;
    }

    __try
    {
        if (!target->isPlayerCharacter())
        {
            return true;
        }

        if (!player->selectedCharacter.isValid() || player->selectedCharacter.getCharacter() != target)
        {
            return true;
        }

        if (attemptedOut)
        {
            *attemptedOut = true;
        }
        if (commandAcceptedOut)
        {
            *commandAcceptedOut = player->selectedCharactersLayingLow();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryForcePlayingDeadFallback(
    Character* target,
    bool alreadyUnconscious,
    bool alreadyPlayingDead,
    bool* knockedOutOut,
    float* knockoutTimerOut)
{
    if (knockedOutOut)
    {
        *knockedOutOut = false;
    }
    if (knockoutTimerOut)
    {
        *knockoutTimerOut = 0.0f;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        target->playerWantsMeToGetUp = false;

        if (!alreadyUnconscious && !alreadyPlayingDead)
        {
            MedicalSystem* medical = target->getMedical();
            if (!medical)
            {
                return false;
            }

            medical->knockout(0.0f);

            if (knockedOutOut)
            {
                *knockedOutOut = true;
            }
            if (knockoutTimerOut)
            {
                *knockoutTimerOut = medical->knockoutTimer;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryForceDying(
    Character* target,
    bool alreadyUnconscious,
    float* knockoutTimerOut,
    float* bloodOut,
    float* pointOfNoReturnOut,
    bool* usedSub50KoOut,
    float* currentBleedRateOut,
    float* extraBloodLossOut,
    bool* probablyDyingOut,
    bool* canWakeOut,
    bool* bloodlossTraumaOut)
{
    if (knockoutTimerOut)
    {
        *knockoutTimerOut = 0.0f;
    }
    if (bloodOut)
    {
        *bloodOut = 0.0f;
    }
    if (pointOfNoReturnOut)
    {
        *pointOfNoReturnOut = 0.0f;
    }
    if (usedSub50KoOut)
    {
        *usedSub50KoOut = false;
    }
    if (currentBleedRateOut)
    {
        *currentBleedRateOut = 0.0f;
    }
    if (extraBloodLossOut)
    {
        *extraBloodLossOut = 0.0f;
    }
    if (probablyDyingOut)
    {
        *probablyDyingOut = false;
    }
    if (canWakeOut)
    {
        *canWakeOut = false;
    }
    if (bloodlossTraumaOut)
    {
        *bloodlossTraumaOut = false;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        const float pointOfNoReturn = medical->pointOfNoReturn();
        const float maxBlood = medical->getMaxBlood();
        const float aliveFloor = -maxBlood + kForceDyingAliveBloodMargin;
        float forcedBlood = pointOfNoReturn - kForceDyingBloodOffset;

        if (!alreadyUnconscious)
        {
            medical->knockout(0.0f);
        }

        if (forcedBlood <= aliveFloor)
        {
            forcedBlood = aliveFloor;
        }

        medical->blood = forcedBlood;
        medical->knockoutTimer = 0.0f;
        medical->currentBleedRate = 1.0f;
        medical->extraBloodLossFromBodyparts = 1.0f;
        medical->bloodlossTrauma = true;
        medical->reassessCollapseMode(false, true);
        target->playerWantsMeToGetUp = false;

        if (knockoutTimerOut)
        {
            *knockoutTimerOut = medical->knockoutTimer;
        }
        if (bloodOut)
        {
            *bloodOut = medical->blood;
        }
        if (pointOfNoReturnOut)
        {
            *pointOfNoReturnOut = pointOfNoReturn;
        }
        if (usedSub50KoOut)
        {
            *usedSub50KoOut = medical->sub50KO;
        }
        if (currentBleedRateOut)
        {
            *currentBleedRateOut = medical->currentBleedRate;
        }
        if (extraBloodLossOut)
        {
            *extraBloodLossOut = medical->extraBloodLossFromBodyparts;
        }
        if (probablyDyingOut)
        {
            *probablyDyingOut = medical->isProbablyDying();
        }
        if (canWakeOut)
        {
            *canWakeOut = medical->canGetUpWakeUp();
        }
        if (bloodlossTraumaOut)
        {
            *bloodlossTraumaOut = medical->isInBloodlossTrauma();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryApplyLimbDamage(
    Character* target,
    MedicalSystem::HealthPartStatus::PartType partType,
    LeftRight side,
    float* partMaxHealthOut,
    float* beforeFleshOut,
    float* afterFleshOut,
    float* beforeFleshStunOut,
    float* afterFleshStunOut,
    float* beforeDerivedHealthOut,
    float* afterDerivedHealthOut,
    float* appliedDamageOut)
{
    if (partMaxHealthOut)
    {
        *partMaxHealthOut = 0.0f;
    }
    if (beforeFleshOut)
    {
        *beforeFleshOut = 0.0f;
    }
    if (afterFleshOut)
    {
        *afterFleshOut = 0.0f;
    }
    if (appliedDamageOut)
    {
        *appliedDamageOut = 0.0f;
    }
    if (beforeFleshStunOut)
    {
        *beforeFleshStunOut = 0.0f;
    }
    if (afterFleshStunOut)
    {
        *afterFleshStunOut = 0.0f;
    }
    if (beforeDerivedHealthOut)
    {
        *beforeDerivedHealthOut = 0.0f;
    }
    if (afterDerivedHealthOut)
    {
        *afterDerivedHealthOut = 0.0f;
    }

    if (!target)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = target->getMedical();
        if (!medical)
        {
            return false;
        }

        MedicalSystem::HealthPartStatus* part = medical->getPart(partType, side);
        if (!part)
        {
            return false;
        }

        const float partMaxHealth = part->maxHealth();
        const float beforeFlesh = part->flesh;
        const float beforeFleshStun = part->fleshStun;
        const float beforeDerivedHealth = part->derivedFleshHealthPercent;
        float damageAmount = partMaxHealth * kLimbDamageFraction;
        if (damageAmount < kMinimumLimbDamageAmount)
        {
            damageAmount = kMinimumLimbDamageAmount;
        }

        __declspec(align(16)) unsigned char damageStorage[sizeof(Damages)] = {};
        Damages* damage = reinterpret_cast<Damages*>(damageStorage);
        damage->cut = 0.0f;
        damage->blunt = damageAmount;
        damage->pierce = 0.0f;
        damage->extraStun = 0.0f;
        damage->bleedMult = 0.0f;
        damage->armourPenetration = 0.0f;

        part->applyDamage(*damage);
        part->updateDerivedHealths();
        medical->updateDamageState();
        medical->reassessCollapseMode(false, false);

        if (partMaxHealthOut)
        {
            *partMaxHealthOut = partMaxHealth;
        }
        if (beforeFleshOut)
        {
            *beforeFleshOut = beforeFlesh;
        }
        if (afterFleshOut)
        {
            *afterFleshOut = part->flesh;
        }
        if (beforeFleshStunOut)
        {
            *beforeFleshStunOut = beforeFleshStun;
        }
        if (afterFleshStunOut)
        {
            *afterFleshStunOut = part->fleshStun;
        }
        if (beforeDerivedHealthOut)
        {
            *beforeDerivedHealthOut = beforeDerivedHealth;
        }
        if (afterDerivedHealthOut)
        {
            *afterDerivedHealthOut = part->derivedFleshHealthPercent;
        }
        if (appliedDamageOut)
        {
            *appliedDamageOut = damageAmount;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

void ReportShellOnlyAction(const char* actionId, const char* actionLabel)
{
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId
               << "\" success=false reason=\"no_target\"";
        LogInfoLine(result.str());
        SetStatusMessage("No target - select a character");
        return;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId
           << "\" success=false reason=\"slice2_shell_only\""
           << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
    LogInfoLine(result.str());

    std::stringstream status;
    status << actionLabel << " unavailable in slice 2 shell";
    SetStatusMessage(status.str());
}

void OnForceUnconsciousButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "force_unconscious";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"force_unconscious\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_unconscious\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Unconscious failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;
    const bool alreadyUnconscious = g_lastTargetSnapshot.unconscious;

    float knockoutTimer = 0.0f;
    if (!TryForceUnconscious(g_lastTargetSnapshot.target, alreadyUnconscious, &knockoutTimer))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_unconscious\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Unconscious failed - apply path unavailable");
        return;
    }

    bool observedUnconscious = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        observedUnconscious = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.unconscious;
    }
    else
    {
        std::string stateLabel;
        bool unconscious = false;
        bool playingDead = false;
        bool dying = false;
        bool dead = false;
        observedUnconscious = TryResolveStateSummary(
                g_lastTargetSnapshot.target,
                &stateLabel,
                &unconscious,
                &playingDead,
                &dying,
                &dead)
            && unconscious;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"force_unconscious\" success="
           << (observedUnconscious ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " already_unconscious=" << (alreadyUnconscious ? "true" : "false")
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " knockout_timer=" << knockoutTimer;
    if (!observedUnconscious)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    std::stringstream status;
    if (observedUnconscious)
    {
        status << (alreadyUnconscious ? "Force Unconscious refreshed for " : "Force Unconscious applied to ")
               << targetName;
    }
    else
    {
        status << "Force Unconscious requested for " << targetName << " - no KO readback yet";
    }
    SetStatusMessage(status.str());
}

void OnForceUnconsciousButtonPressed(MyGUI::Widget* widget, int left, int top, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnForceUnconsciousButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnForcePlayingDeadButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "force_playing_dead";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;
    const bool alreadyPlayingDead = g_lastTargetSnapshot.playingDead;
    const bool alreadyUnconscious = g_lastTargetSnapshot.unconscious;

    if (alreadyPlayingDead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=true"
               << " target_name=\"" << SanitizeLogValue(targetName) << "\""
               << " already_playing_dead=true";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead confirmed for " + targetName);
        return;
    }

    bool selectionPathAttempted = false;
    bool selectionCommandAccepted = false;
    if (!TryInvokeSelectedCharactersLayingLow(
            g_lastPlayerInterface,
            g_lastTargetSnapshot.target,
            &selectionPathAttempted,
            &selectionCommandAccepted))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"selection_path_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead failed - selection path crashed");
        return;
    }

    bool knockedOut = false;
    float knockoutTimer = 0.0f;
    if (!selectionCommandAccepted
        && !TryForcePlayingDeadFallback(
                g_lastTargetSnapshot.target,
                alreadyUnconscious,
                alreadyPlayingDead,
                &knockedOut,
                &knockoutTimer))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_playing_dead\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Playing Dead failed - apply path unavailable");
        return;
    }

    bool observedPlayingDead = false;
    bool observedUnconscious = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        observedPlayingDead = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.playingDead;
        observedUnconscious = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.unconscious;
    }
    else
    {
        std::string stateLabel;
        bool unconscious = false;
        bool playingDead = false;
        bool dying = false;
        bool dead = false;
        if (TryResolveStateSummary(
                g_lastTargetSnapshot.target,
                &stateLabel,
                &unconscious,
                &playingDead,
                &dying,
                &dead))
        {
            observedPlayingDead = playingDead;
            observedUnconscious = unconscious;
        }
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"force_playing_dead\" success="
           << (observedPlayingDead ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " selection_path_attempted=" << (selectionPathAttempted ? "true" : "false")
           << " selection_command_accepted=" << (selectionCommandAccepted ? "true" : "false")
           << " fallback_knockout=" << (knockedOut ? "true" : "false")
           << " observed_playing_dead=" << (observedPlayingDead ? "true" : "false")
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " knockout_timer=" << knockoutTimer;
    if (!observedPlayingDead)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (observedPlayingDead)
    {
        SetStatusMessage("Force Playing Dead applied to " + targetName);
        return;
    }

    if (observedUnconscious)
    {
        SetStatusMessage("Force Playing Dead requested for " + targetName + " - waiting for recovery into play dead");
        return;
    }

    SetStatusMessage("Force Playing Dead requested for " + targetName + " - no readback yet");
}

void OnForcePlayingDeadButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnForcePlayingDeadButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnFullRestoreButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "full_restore";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"full_restore\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"full_restore\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Full Restore failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;

    bool fullyRestored = false;
    float bloodLevel = 0.0f;
    float maxBlood = 0.0f;
    if (!TryFullRestore(g_lastTargetSnapshot.target, &fullyRestored, &bloodLevel, &maxBlood))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"full_restore\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Full Restore failed - apply path unavailable");
        return;
    }

    ClearForceDyingArm("full_restore", false);

    std::string observedStateLabel = "Unknown";
    bool observedUnconscious = false;
    bool observedPlayingDead = false;
    bool observedDying = false;
    bool observedDead = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        if (g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget)
        {
            observedStateLabel = g_lastTargetSnapshot.stateLabel;
            observedUnconscious = g_lastTargetSnapshot.unconscious;
            observedPlayingDead = g_lastTargetSnapshot.playingDead;
            observedDying = g_lastTargetSnapshot.dying;
            observedDead = g_lastTargetSnapshot.dead;
        }
    }
    else
    {
        TryResolveStateSummary(
            g_lastTargetSnapshot.target,
            &observedStateLabel,
            &observedUnconscious,
            &observedPlayingDead,
            &observedDying,
            &observedDead);
    }

    const bool observedRecovered = !observedUnconscious && !observedPlayingDead && !observedDying && !observedDead;
    const bool success = fullyRestored && observedRecovered;

    std::stringstream result;
    result << "event=testkit_action_result action=\"full_restore\" success="
           << (success ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " fully_restored=" << (fullyRestored ? "true" : "false")
           << " observed_state=\"" << observedStateLabel << "\""
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " observed_playing_dead=" << (observedPlayingDead ? "true" : "false")
           << " observed_dying=" << (observedDying ? "true" : "false")
           << " observed_dead=" << (observedDead ? "true" : "false")
           << " blood=" << bloodLevel
           << " max_blood=" << maxBlood;
    if (!success)
    {
        result << " reason=\"not_fully_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (success)
    {
        SetStatusMessage("Full Restore applied to " + targetName);
        return;
    }

    SetStatusMessage("Full Restore requested for " + targetName + " - no full restore readback yet");
}

void OnFullRestoreButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnFullRestoreButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageLimbButtonClicked(
    const char* actionId,
    const char* actionLabel,
    MedicalSystem::HealthPartStatus::PartType partType,
    LeftRight side)
{
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_target\"";
        LogInfoLine(result.str());
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(actionLabel) + " failed - target is dead");
        return;
    }

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;

    float partMaxHealth = 0.0f;
    float beforeFlesh = 0.0f;
    float afterFlesh = 0.0f;
    float beforeFleshStun = 0.0f;
    float afterFleshStun = 0.0f;
    float beforeDerivedHealth = 0.0f;
    float afterDerivedHealth = 0.0f;
    float appliedDamage = 0.0f;
    if (!TryApplyLimbDamage(
            g_lastTargetSnapshot.target,
            partType,
            side,
            &partMaxHealth,
            &beforeFlesh,
            &afterFlesh,
            &beforeFleshStun,
            &afterFleshStun,
            &beforeDerivedHealth,
            &afterDerivedHealth,
            &appliedDamage))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(actionLabel) + " failed - apply path unavailable");
        return;
    }

    std::string observedStateLabel = "Unknown";
    bool observedUnconscious = false;
    bool observedPlayingDead = false;
    bool observedDying = false;
    bool observedDead = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        if (g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget)
        {
            observedStateLabel = g_lastTargetSnapshot.stateLabel;
            observedUnconscious = g_lastTargetSnapshot.unconscious;
            observedPlayingDead = g_lastTargetSnapshot.playingDead;
            observedDying = g_lastTargetSnapshot.dying;
            observedDead = g_lastTargetSnapshot.dead;
        }
    }
    else
    {
        TryResolveStateSummary(
            g_lastTargetSnapshot.target,
            &observedStateLabel,
            &observedUnconscious,
            &observedPlayingDead,
            &observedDying,
            &observedDead);
    }

    const bool fleshChanged = (beforeFlesh - afterFlesh > kFloatChangeEpsilon) || (afterFlesh - beforeFlesh > kFloatChangeEpsilon);
    const bool fleshStunChanged =
        (beforeFleshStun - afterFleshStun > kFloatChangeEpsilon) || (afterFleshStun - beforeFleshStun > kFloatChangeEpsilon);
    const bool derivedHealthChanged =
        (beforeDerivedHealth - afterDerivedHealth > kFloatChangeEpsilon)
        || (afterDerivedHealth - beforeDerivedHealth > kFloatChangeEpsilon);
    const bool observedEffect =
        fleshChanged || fleshStunChanged || derivedHealthChanged || observedUnconscious || observedDying || observedDead;

    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId << "\" success="
           << (observedEffect ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " part_max_health=" << partMaxHealth
           << " before_flesh=" << beforeFlesh
           << " after_flesh=" << afterFlesh
           << " before_flesh_stun=" << beforeFleshStun
           << " after_flesh_stun=" << afterFleshStun
           << " before_derived_health=" << beforeDerivedHealth
           << " after_derived_health=" << afterDerivedHealth
           << " applied_damage=" << appliedDamage
           << " observed_state=\"" << observedStateLabel << "\""
           << " observed_unconscious=" << (observedUnconscious ? "true" : "false")
           << " observed_playing_dead=" << (observedPlayingDead ? "true" : "false")
           << " observed_dying=" << (observedDying ? "true" : "false")
           << " observed_dead=" << (observedDead ? "true" : "false");
    if (!observedEffect)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (observedEffect)
    {
        SetStatusMessage(std::string(actionLabel) + " applied to " + targetName);
        return;
    }

    SetStatusMessage(std::string(actionLabel) + " requested for " + targetName + " - no limb change readback yet");
}

void OnDamageLeftArmButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_left_arm",
        "Damage Left Arm",
        MedicalSystem::HealthPartStatus::PART_ARM,
        SIDE_LEFT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageRightArmButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_right_arm",
        "Damage Right Arm",
        MedicalSystem::HealthPartStatus::PART_ARM,
        SIDE_RIGHT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageLeftLegButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_left_leg",
        "Damage Left Leg",
        MedicalSystem::HealthPartStatus::PART_LEG,
        SIDE_LEFT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnDamageRightLegButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnDamageLimbButtonClicked(
        "damage_right_leg",
        "Damage Right Leg",
        MedicalSystem::HealthPartStatus::PART_LEG,
        SIDE_RIGHT);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnForceDyingButtonClicked(MyGUI::Widget*)
{
    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"force_dying\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (g_lastTargetSnapshot.dead)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_dying\" success=false reason=\"target_dead\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Dying failed - target is already dead");
        return;
    }

    if (g_confirmDangerousActions && !g_forceDyingArmed)
    {
        g_forceDyingArmed = true;
        g_forceDyingArmedAtMs = GetTickCount();
        UpdateForceDyingButtonCaption();

        LogInfoLine("event=testkit_action_arm action=\"force_dying\" armed=true");
        SetStatusMessage("Force Dying armed - click again to confirm");
        return;
    }

    LogActionRequested("force_dying");

    const Character* const expectedTarget = g_lastTargetSnapshot.target;
    const std::string targetName = g_lastTargetSnapshot.name;
    const bool alreadyUnconscious = g_lastTargetSnapshot.unconscious;
    ClearForceDyingArm("confirmed", false);

    float knockoutTimer = 0.0f;
    float bloodLevel = 0.0f;
    float pointOfNoReturn = 0.0f;
    bool usedSub50Ko = false;
    float currentBleedRate = 0.0f;
    float extraBloodLoss = 0.0f;
    bool probablyDying = false;
    bool canWake = false;
    bool bloodlossTrauma = false;
    if (!TryForceDying(
            g_lastTargetSnapshot.target,
            alreadyUnconscious,
            &knockoutTimer,
            &bloodLevel,
            &pointOfNoReturn,
            &usedSub50Ko,
            &currentBleedRate,
            &extraBloodLoss,
            &probablyDying,
            &canWake,
            &bloodlossTrauma))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"force_dying\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Force Dying failed - apply path unavailable");
        return;
    }

    bool observedDying = false;
    if (g_lastPlayerInterface)
    {
        UpdateTargetInspection(g_lastPlayerInterface);
        observedDying = g_hasLastTargetSnapshot
            && g_lastTargetSnapshot.hasTarget
            && g_lastTargetSnapshot.target == expectedTarget
            && g_lastTargetSnapshot.dying;
    }
    else
    {
        std::string stateLabel;
        bool unconscious = false;
        bool playingDead = false;
        bool dying = false;
        bool dead = false;
        observedDying = TryResolveStateSummary(
                g_lastTargetSnapshot.target,
                &stateLabel,
                &unconscious,
                &playingDead,
                &dying,
                &dead)
            && dying;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"force_dying\" success="
           << (observedDying ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " observed_dying=" << (observedDying ? "true" : "false")
           << " knockout_timer=" << knockoutTimer
           << " blood=" << bloodLevel
           << " point_of_no_return=" << pointOfNoReturn
           << " used_sub50ko_fallback=" << (usedSub50Ko ? "true" : "false")
           << " current_bleed_rate=" << currentBleedRate
           << " extra_blood_loss=" << extraBloodLoss
           << " probably_dying=" << (probablyDying ? "true" : "false")
           << " can_wake=" << (canWake ? "true" : "false")
           << " bloodloss_trauma=" << (bloodlossTrauma ? "true" : "false");
    if (!observedDying)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (observedDying)
    {
        SetStatusMessage("Force Dying applied to " + targetName);
        return;
    }

    SetStatusMessage("Force Dying requested for " + targetName + " - no dying readback yet");
}

void OnForceDyingButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnForceDyingButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void TickForceDyingArmTimeout()
{
    if (!g_forceDyingArmed)
    {
        return;
    }

    const DWORD nowMs = GetTickCount();
    if (nowMs - g_forceDyingArmedAtMs < kDangerArmTimeoutMs)
    {
        return;
    }

    ClearForceDyingArm("timeout", true);
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
