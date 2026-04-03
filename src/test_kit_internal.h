#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/GameData.h>
#include <kenshi/PlayerInterface.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_ComboBox.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_KeyCode.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>

#include <sstream>
#include <string>
#include <vector>

class Faction;

namespace test_kit
{
enum TargetSource
{
    TargetSource_None = 0,
    TargetSource_Selected = 1,
    TargetSource_Hovered = 2,
    TargetSource_Conversation = 3
};

enum PanelTab
{
    PanelTab_Health = 0,
    PanelTab_Stats = 1,
    PanelTab_Teleport = 2,
    PanelTab_Inventory = 3,
    PanelTab_Spawn = 4
};

enum LoggingLevel
{
    LoggingLevel_Info = 0,
    LoggingLevel_Debug = 1
};

enum DangerousHealthAction
{
    DangerousHealthAction_None = 0,
    DangerousHealthAction_ForceDead = 1,
    DangerousHealthAction_ForceDying = 2
};

struct TargetSnapshot
{
    bool hasTarget;
    TargetSource source;
    Character* target;
    std::string name;
    std::string factionName;
    std::string alignment;
    std::string membership;
    std::string stateLabel;
    bool unconscious;
    bool playingDead;
    bool dying;
    bool dead;
};

struct CharacterPositionSnapshot
{
    CharacterPositionSnapshot()
        : position(0.0f, 0.0f, 0.0f)
        , rawPosition(0.0f, 0.0f, 0.0f)
        , rawEntityPosition(0.0f, 0.0f, 0.0f)
        , terrainHeight(0.0f)
        , hasPosition(false)
        , hasRawPosition(false)
        , hasRawEntityPosition(false)
        , hasTerrainHeight(false)
    {
    }

    Ogre::Vector3 position;
    Ogre::Vector3 rawPosition;
    Ogre::Vector3 rawEntityPosition;
    float terrainHeight;
    bool hasPosition;
    bool hasRawPosition;
    bool hasRawEntityPosition;
    bool hasTerrainHeight;
};

struct SavedLocation
{
    SavedLocation()
        : position(0.0f, 0.0f, 0.0f)
        , pinned(false)
        , lastUsedUtc(0u)
    {
    }

    std::string id;
    std::string name;
    Ogre::Vector3 position;
    bool pinned;
    unsigned long long lastUsedUtc;
};

enum StatsEditOperation
{
    StatsEditOperation_Set = 0,
    StatsEditOperation_Add = 1,
    StatsEditOperation_Subtract = 2
};

enum StatsGroup
{
    StatsGroup_Core = 0,
    StatsGroup_MovementUtility = 1,
    StatsGroup_Combat = 2,
    StatsGroup_WeaponSkills = 3,
    StatsGroup_Labor = 4
};

enum StatsSectionFilter
{
    StatsSectionFilter_All = 0,
    StatsSectionFilter_CommonTest = 1,
    StatsSectionFilter_Core = 2,
    StatsSectionFilter_MovementUtility = 3,
    StatsSectionFilter_Combat = 4,
    StatsSectionFilter_WeaponSkills = 5,
    StatsSectionFilter_Labor = 6
};

struct StatsRegistryEntry
{
    StatsEnumerated stat;
    const char* label;
    StatsGroup group;
    bool commonTestStat;
    int safeMinValue;
    int safeMaxValue;
};

struct StatsClipboardEntry
{
    StatsEnumerated stat;
    int value;
};

extern const int kPanelLeft;
extern const int kPanelTop;
extern const int kPanelExpandedHeight;
extern const int kPanelExpandedHeightLowerBound;
extern const int kPanelExpandedHeightUpperBound;
extern const int kPanelCollapsedHeight;
extern const int kPanelViewportPadding;
extern const int kPanelDragThreshold;
extern const int kPanelHeaderHeight;
extern const int kPanelBodyOverlapLowerBound;
extern const int kPanelBodyOverlapUpperBound;
extern const int kPanelBodyScrollPadding;
extern const int kPanelBodyBottomPadding;
extern const int kPanelStatusGap;
extern const int kPanelEdgeSnapDistance;
extern const int kPanelMinimumVisibleWidth;
extern const int kPanelMinimumVisibleHeight;
extern const int kPanelHeaderTitleFontHeightLowerBound;
extern const int kPanelHeaderTitleFontHeightUpperBound;
extern const int kPanelHeaderButtonSizeLowerBound;
extern const int kPanelHeaderButtonSizeUpperBound;
extern const int kPanelHeaderButtonGap;
extern const int kPanelHeaderButtonRightPadding;
extern const int kInventoryItemDropdownMaxListLength;
extern const int kPanelWidthDefault;
extern const int kPanelWidthLowerBound;
extern const int kPanelWidthUpperBound;
extern const int kPanelMinExpandedHeightDefault;
extern const int kPanelHeaderTitleFontHeightDefault;
extern const int kPanelCollapseButtonSizeDefault;
extern const int kPanelCloseButtonSizeDefault;
extern const int kPanelBodyOverlapDefault;
extern const char* kDeveloperModeConfigKey;
extern const char* kSavedLocationsConfigKey;
extern const char* kDefaultTogglePanelKey;

extern int kPanelWidth;

extern std::string g_configPath;
extern bool g_pluginEnabled;
extern bool g_developerMode;
extern LoggingLevel g_loggingLevel;
extern bool g_togglePanelRequireCtrl;
extern bool g_togglePanelRequireShift;
extern bool g_togglePanelRequireAlt;
extern std::string g_togglePanelKey;
extern bool g_hotkeyEnabled;
extern int g_hotkeyVirtualKey;
extern std::string g_hotkeyDisplay;
extern bool g_hotkeyPrevDown;
extern bool g_confirmDangerousActions;
extern bool g_panelHidden;
extern bool g_panelCollapsed;
extern int g_panelMinExpandedHeight;
extern int g_panelMaxExpandedHeight;
extern int g_panelHeaderTitleFontHeight;
extern int g_panelCollapseButtonSize;
extern int g_panelCloseButtonSize;
extern int g_panelBodyOverlap;
extern bool g_runtimePanelPositionSet;
extern int g_runtimePanelLeft;
extern int g_runtimePanelTop;
extern bool g_panelDragging;
extern bool g_panelDragMoved;
extern int g_panelDragLastMouseX;
extern int g_panelDragLastMouseY;
extern int g_panelDragMovedDistance;
extern DangerousHealthAction g_armedDangerousHealthAction;
extern DWORD g_dangerousHealthActionArmedAtMs;
extern std::string g_lastStatusMessage;
extern PanelTab g_activePanelTab;
extern TargetSnapshot g_lastTargetSnapshot;
extern bool g_hasLastTargetSnapshot;
extern PlayerInterface* g_lastPlayerInterface;
extern bool g_loggedPanelCreateFailure;

extern MyGUI::Widget* g_panel;
extern MyGUI::Button* g_headerBackground;
extern MyGUI::Widget* g_headerFrame;
extern MyGUI::TextBox* g_headerTitleText;
extern MyGUI::Button* g_collapseButton;
extern MyGUI::Button* g_closeButton;
extern MyGUI::Button* g_bodyFrame;
extern MyGUI::ScrollView* g_bodyScrollView;
extern MyGUI::TextBox* g_targetSectionText;
extern MyGUI::TextBox* g_targetNameText;
extern MyGUI::TextBox* g_targetFactionText;
extern MyGUI::TextBox* g_targetAlignmentText;
extern MyGUI::TextBox* g_targetMembershipText;
extern MyGUI::TextBox* g_targetStateText;
extern MyGUI::TextBox* g_noTargetText;
extern MyGUI::Button* g_healthTabButton;
extern MyGUI::Button* g_statsTabButton;
extern MyGUI::Button* g_teleportTabButton;
extern MyGUI::Button* g_inventoryTabButton;
extern MyGUI::Button* g_spawnTabButton;
extern MyGUI::TextBox* g_statesSectionText;
extern MyGUI::Button* g_fullRestoreButton;
extern MyGUI::Button* g_forceUnconsciousButton;
extern MyGUI::TextBox* g_limbDamageSectionText;
extern MyGUI::Button* g_damageLeftArmButton;
extern MyGUI::Button* g_damageRightArmButton;
extern MyGUI::Button* g_damageLeftLegButton;
extern MyGUI::Button* g_damageRightLegButton;
extern MyGUI::TextBox* g_statsSectionText;
extern MyGUI::TextBox* g_statsScopeText;
extern MyGUI::Button* g_statsApplyToAllButton;
extern MyGUI::TextBox* g_statsClipboardText;
extern MyGUI::Button* g_statsCopyButton;
extern MyGUI::Button* g_statsPasteButton;
extern MyGUI::TextBox* g_statsSectionFilterText;
extern MyGUI::Button* g_statsAllSectionButton;
extern MyGUI::Button* g_statsCommonSectionButton;
extern MyGUI::Button* g_statsCoreSectionButton;
extern MyGUI::Button* g_statsUtilitySectionButton;
extern MyGUI::Button* g_statsCombatSectionButton;
extern MyGUI::Button* g_statsWeaponsSectionButton;
extern MyGUI::Button* g_statsLaborSectionButton;
extern MyGUI::TextBox* g_statsSearchLabelText;
extern MyGUI::EditBox* g_statsSearchEdit;
extern MyGUI::TextBox* g_statsResultCountText;
extern MyGUI::ListBox* g_statsResultsList;
extern MyGUI::TextBox* g_statsSelectedSummaryText;
extern MyGUI::TextBox* g_statsCurrentValueText;
extern MyGUI::TextBox* g_statsInputLabelText;
extern MyGUI::EditBox* g_statsInputEdit;
extern MyGUI::Button* g_statsSetButton;
extern MyGUI::Button* g_statsAddButton;
extern MyGUI::Button* g_statsSubtractButton;
extern MyGUI::TextBox* g_statsPreviewText;
extern MyGUI::TextBox* g_teleportSectionText;
extern MyGUI::TextBox* g_saveLocationNameLabelText;
extern MyGUI::EditBox* g_saveLocationNameEdit;
extern MyGUI::Button* g_saveSelectedLocationButton;
extern MyGUI::TextBox* g_savedLocationsSectionText;
extern MyGUI::Button* g_savedLocationsCollapseButton;
extern MyGUI::Widget* g_savedLocationsRowsRoot;
extern MyGUI::TextBox* g_savedLocationSearchLabelText;
extern MyGUI::EditBox* g_savedLocationSearchEdit;
extern MyGUI::ListBox* g_savedLocationsListBox;
extern MyGUI::TextBox* g_savedLocationsEmptyText;
extern MyGUI::Button* g_savedLocationTeleportButton;
extern MyGUI::Button* g_savedLocationPinButton;
extern MyGUI::Button* g_savedLocationRenameButton;
extern MyGUI::Button* g_savedLocationDeleteButton;
extern MyGUI::TextBox* g_inventorySectionText;
extern MyGUI::TextBox* g_moneyAmountLabelText;
extern MyGUI::EditBox* g_moneyAmountEdit;
extern MyGUI::Button* g_addMoneyButton;
extern MyGUI::TextBox* g_spawnFoodSectionText;
extern MyGUI::TextBox* g_itemCategoryLabelText;
extern MyGUI::ComboBox* g_itemCategoryDropdown;
extern MyGUI::TextBox* g_itemSearchLabelText;
extern MyGUI::EditBox* g_itemSearchEdit;
extern MyGUI::ListBox* g_itemSearchResultsList;
extern MyGUI::TextBox* g_itemQuantityLabelText;
extern MyGUI::EditBox* g_itemQuantityEdit;
extern MyGUI::Button* g_spawnItemButton;
extern MyGUI::TextBox* g_spawnSectionText;
extern MyGUI::TextBox* g_spawnCategoryLabelText;
extern MyGUI::ComboBox* g_spawnCategoryDropdown;
extern MyGUI::TextBox* g_spawnSearchLabelText;
extern MyGUI::EditBox* g_spawnSearchEdit;
extern MyGUI::TextBox* g_spawnResultCountText;
extern MyGUI::ListBox* g_spawnResultsList;
extern MyGUI::TextBox* g_spawnSelectedSummaryText;
extern MyGUI::TextBox* g_spawnQuantityLabelText;
extern MyGUI::EditBox* g_spawnQuantityEdit;
extern MyGUI::TextBox* g_spawnFactionLabelText;
extern MyGUI::ComboBox* g_spawnFactionDropdown;
extern MyGUI::TextBox* g_spawnCustomFactionLabelText;
extern MyGUI::EditBox* g_spawnCustomFactionSearchEdit;
extern MyGUI::ListBox* g_spawnCustomFactionResultsList;
extern MyGUI::TextBox* g_spawnAllegianceLabelText;
extern MyGUI::ComboBox* g_spawnAllegianceDropdown;
extern MyGUI::TextBox* g_spawnRadiusLabelText;
extern MyGUI::ComboBox* g_spawnRadiusDropdown;
extern MyGUI::TextBox* g_spawnCreatureAgeLabelText;
extern MyGUI::ComboBox* g_spawnCreatureAgeDropdown;
extern MyGUI::TextBox* g_spawnModeLabelText;
extern MyGUI::ComboBox* g_spawnModeDropdown;
extern MyGUI::TextBox* g_spawnPreviewText;
extern MyGUI::Button* g_spawnCharactersButton;
extern MyGUI::TextBox* g_dangerousSectionText;
extern MyGUI::Button* g_forceDeadButton;
extern MyGUI::Button* g_forceDyingButton;
extern MyGUI::TextBox* g_statusText;

extern std::vector<int> g_filteredStatsRegistryIndexes;
extern std::vector<StatsClipboardEntry> g_statsClipboardEntries;
extern std::vector<SavedLocation> g_savedLocations;
extern StatsEnumerated g_selectedStatsStat;
extern bool g_statsApplyToAllSelected;
extern StatsSectionFilter g_activeStatsSectionFilter;
extern std::string g_statsClipboardSourceName;
extern std::vector<size_t> g_filteredSavedLocationIndexes;
extern std::string g_savedLocationRenameId;
extern std::string g_savedLocationSearchText;
extern std::string g_selectedSavedLocationId;
extern bool g_savedLocationsCollapsed;

int GetSelectedCharacterCount(PlayerInterface* player);
bool HasPrimarySelectedCharacter(PlayerInterface* player);
Character* TryGetPrimarySelectedCharacter(PlayerInterface* player);
int ClampIntValue(int value, int minValue, int maxValue);
int ClampPanelWidthValue(int value);
float ComputeHorizontalDistance(const Ogre::Vector3& a, const Ogre::Vector3& b);
std::string TrimAscii(const std::string& value);
std::string ToUpperAscii(const std::string& value);
std::string SanitizeLogValue(const std::string& value);
std::string SafeCharacterName(Character* target);
std::string SafeFactionName(Faction* faction);
std::string FormatPointerValue(const void* pointer);
const char* TargetSourceToLogLabel(TargetSource source);
bool IsProbablyReadableEnginePointer(const void* pointer);
bool TryParsePositiveInt(const std::string& value, int* outValue);
bool TryParseNonNegativeInt(const std::string& value, int* outValue);
bool TryGetCharacterPositionSnapshot(Character* character, CharacterPositionSnapshot* outSnapshot);
bool TryResolveCharacterFaction(Character* character, Faction** outFaction);
bool TryGetCharacterTeleportReferencePosition(
    Character* character,
    bool useSpawnValidation,
    Ogre::Vector3* outPosition,
    const char** outSourceLabel);
void LogInfoLine(const std::string& message);
void LogWarnLine(const std::string& message);
void LogErrorLine(const std::string& message);
void LogDebugLine(const std::string& message);
void LogActionRequested(const char* actionId);
void SetStatusMessage(const std::string& message);
void SetActivePanelTab(PanelTab tab);
void RefreshStatusWidget();
void SetActionButtonsEnabled(bool enabled);
void SetSelectionActionButtonsEnabled(bool enabled);
void ResetTargetSnapshot(TargetSnapshot* snapshot);
void ApplyTargetSnapshotToUi(const TargetSnapshot& snapshot);
void UpdateTargetInspection(PlayerInterface* player);
bool TryRestoreRequestedSelectedSpawnTarget(PlayerInterface* player, Character* requestedTarget);
void AppendCharacterSnapshotLogFields(
    std::stringstream& line,
    const char* prefix,
    const CharacterPositionSnapshot& snapshot);
}
