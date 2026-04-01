#include <Debug.h>

#include <emc/mod_hub_client.h>

#include "src/test_kit_config.h"
#include "src/test_kit_panel.h"
#include "src/test_kit_spawn.h"
#include "src/test_kit_stats.h"

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
const int kDownedTeleportRestoreMinDelayTicks = 5;
const int kDownedTeleportRestoreMaxDelayTicks = 20;
const float kForceDyingBloodOffset = 8.0f;
const float kForceDyingAliveBloodMargin = 1.0f;
const float kProbablyDyingBloodMax = 50.0f;
const float kLimbDamageFraction = 0.35f;
const float kMinimumLimbDamageAmount = 5.0f;
const float kFloatChangeEpsilon = 0.001f;
const int kSavedLocationsSectionContentHeight = 210;
const int kSavedLocationsListHeight = 120;
const int kSavedLocationEmptyHeight = 18;
// MyGUI expects the popup length as a visual height, not an item count.
extern const int kInventoryItemDropdownMaxListLength = 224;
const char* const kInventorySpawnGeneralKeywords[] = {
    "BUILDING MATERIAL",
    "IRON PLATE",
    "IRON ORE",
    "RAW IRON",
    "COPPER",
    "ELECTRICAL COMPONENT",
    "FABRIC",
    "FABRICS",
    "STEEL BAR",
    "HACKSAW",
    "LOCKPICK",
    "REPAIR KIT",
    "FIRST AID",
    "MEDKIT",
    "SPLINT",
    "SLEEPING BAG",
    "LANTERN",
    "TORCH",
    "LUXURY GOODS",
    "BLUEPRINT",
    "MAP",
    "BOOK"
};
const char* const kInventorySpawnArmourKeywords[] = {
    "CHAINMAIL",
    "CHAIN SHIRT",
    "CHAINSHIRT",
    "ARMOUR",
    "ARMOR",
    "BODY ARMOUR",
    "BODY ARMOR",
    "CLOTHING",
    "HELMET",
    "MASK",
    "HOOD",
    "HAT",
    "BOOTS",
    "SANDALS",
    "SHIRT",
    "VEST",
    "TURTLENECK",
    "PANTS",
    "TROUSERS",
    "SKIRT",
    "JACKET",
    "COAT",
    "ROBE",
    "LONGCOAT",
    "DUSTCOAT",
    "RAGS"
};
const char* const kInventorySpawnWeaponKeywords[] = {
    "WEAPON",
    "KATANA",
    "SABRE",
    "SCIMITAR",
    "NODACHI",
    "WAKIZASHI",
    "JITTE",
    "POLEARM",
    "NAGINATA",
    "HAMMER",
    "AXE",
    "CLUB",
    "CHOPPER",
    "HACKER",
    "TOPPER",
    "BLADE",
    "SCYTHE",
    "MAUL",
    "MACE",
    "CROSSBOW",
    "BOW",
    "STAFF",
    "PLANK",
    "CLEAVER",
    "SWORD",
    "PALADIN"
};
const char* const kInventorySpawnToolKeywords[] = {
    "HACKSAW",
    "LOCKPICK",
    "REPAIR KIT",
    "TOOL"
};

enum InventorySpawnCategory
{
    InventorySpawnCategory_All = 0,
    InventorySpawnCategory_Food = 1,
    InventorySpawnCategory_General = 2,
    InventorySpawnCategory_Armour = 3,
    InventorySpawnCategory_Weapons = 4
};

struct InventorySpawnOption
{
    std::string displayName;
    std::string searchTextUpper;
    GameData* itemData;
};

struct DownedTeleportState
{
    DownedTeleportState()
        : active(false)
        , proneState(PS_NORMAL)
        , playerWantsMeToGetUp(false)
        , crippled(false)
        , unconscious(false)
        , sub50KO(false)
        , bloodlossTrauma(false)
        , knockoutTimer(0.0f)
    {
    }

    bool active;
    ProneState proneState;
    bool playerWantsMeToGetUp;
    bool crippled;
    bool unconscious;
    bool sub50KO;
    bool bloodlossTrauma;
    float knockoutTimer;
};

struct PendingDownedTeleportRestore
{
    PendingDownedTeleportRestore()
        : character(0)
        , destination(0.0f, 0.0f, 0.0f)
        , ageTicks(0)
        , active(false)
    {
    }

    Character* character;
    std::string actionId;
    std::string targetName;
    DownedTeleportState state;
    Ogre::Vector3 destination;
    int ageTicks;
    bool active;
};

typedef unsigned int InventorySearchCodepoint;
typedef std::vector<InventorySearchCodepoint> InventorySearchText;

enum InventorySearchShortcutKind
{
    InventorySearchShortcutKind_None = 0,
    InventorySearchShortcutKind_CtrlLeft,
    InventorySearchShortcutKind_CtrlRight,
    InventorySearchShortcutKind_CtrlBackspace
};

struct InventorySearchSelection
{
    InventorySearchSelection()
        : active(false)
        , start(0u)
        , length(0u)
    {
    }

    InventorySearchSelection(bool activeValue, std::size_t startValue, std::size_t lengthValue)
        : active(activeValue)
        , start(startValue)
        , length(lengthValue)
    {
    }

    bool active;
    std::size_t start;
    std::size_t length;
};

struct InventorySearchSnapshot
{
    InventorySearchSnapshot()
        : cursor(0u)
    {
    }

    InventorySearchSnapshot(
        const InventorySearchText& textValue,
        std::size_t cursorValue,
        const InventorySearchSelection& selectionValue)
        : text(textValue)
        , cursor(cursorValue)
        , selection(selectionValue)
    {
    }

    InventorySearchText text;
    std::size_t cursor;
    InventorySearchSelection selection;
};

struct InventorySearchEditResult
{
    InventorySearchEditResult()
        : handled(false)
        , rewriteText(false)
        , cursor(0u)
    {
    }

    bool handled;
    bool rewriteText;
    InventorySearchText text;
    std::size_t cursor;
    InventorySearchSelection selection;
};

struct PendingInventorySearchShortcut
{
    PendingInventorySearchShortcut()
        : active(false)
        , keyValue(0)
    {
    }

    bool active;
    int keyValue;
    InventorySearchEditResult editResult;
};

bool g_inventorySearchCtrlFPrevDown = false;
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

std::vector<InventorySpawnOption> g_inventoryFoodItemOptions;
std::vector<size_t> g_filteredInventoryFoodItemOptionIndexes;
std::vector<int> g_filteredStatsRegistryIndexes;
std::vector<StatsClipboardEntry> g_statsClipboardEntries;
std::vector<size_t> g_filteredSavedLocationIndexes;
std::string g_savedLocationRenameId;
std::string g_savedLocationSearchText;
std::string g_selectedSavedLocationId;
StatsEnumerated g_selectedStatsStat = STAT_NONE;
bool g_statsApplyToAllSelected = false;
StatsSectionFilter g_activeStatsSectionFilter = StatsSectionFilter_All;
std::string g_statsClipboardSourceName;
bool g_savedLocationsCollapsed = false;
bool g_inventoryFoodItemOptionsLoaded = false;
PendingInventorySearchShortcut g_pendingInventorySearchShortcut;
bool g_haveInventorySearchEditSnapshot = false;
InventorySearchSnapshot g_inventorySearchEditSnapshot;
std::vector<PendingDownedTeleportRestore> g_pendingDownedTeleportRestores;

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
void OnSaveSelectedLocationButtonClicked(MyGUI::Widget*);
bool TryResolveSelectedStatsEntry(const StatsRegistryEntry** entryOut);
bool TryResolveSelectedInventoryFoodItem(GameData** itemDataOut, std::string* itemLabelOut);
bool TryGetSelectedSavedLocation(size_t* indexOut, SavedLocation* locationOut);
void RefreshSavedLocationActionButtons(PlayerInterface* player);
void RefreshInventorySpawnButtonState();
void OnSavedLocationsCollapseButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSaveLocationNameAccepted(MyGUI::EditBox*);
void OnSavedLocationSearchTextChanged(MyGUI::EditBox*);
void OnSavedLocationsListSelectionChanged(MyGUI::ListBox*, size_t);
void OnSavedLocationTeleportButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationPinButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationRenameButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationDeleteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
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
void OnInventorySearchResultsSelectionChanged(MyGUI::ListBox*, size_t);
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

std::string BuildInventorySpawnOptionLabel(GameData* itemData)
{
    if (!itemData)
    {
        return "";
    }

    const std::string name = TrimAscii(itemData->name);
    if (!name.empty())
    {
        return name;
    }

    const std::string stringId = TrimAscii(itemData->stringID);
    if (!stringId.empty())
    {
        return stringId;
    }

    std::stringstream fallback;
    fallback << "Item " << itemData->id;
    return fallback.str();
}

std::string BuildInventorySpawnOptionSearchText(GameData* itemData, const std::string& displayName)
{
    std::string searchTextUpper = ToUpperAscii(displayName);
    if (!itemData)
    {
        return searchTextUpper;
    }

    const std::string stringId = TrimAscii(itemData->stringID);
    if (!stringId.empty())
    {
        searchTextUpper += " ";
        searchTextUpper += ToUpperAscii(stringId);
    }

    switch (itemData->type)
    {
    case WEAPON:
        searchTextUpper += " WEAPON";
        break;
    case CROSSBOW:
        searchTextUpper += " CROSSBOW WEAPON";
        break;
    case ARMOUR:
        searchTextUpper += " ARMOUR ARMOR";
        break;
    default:
        break;
    }

    for (boost::unordered::unordered_map<std::string, std::string>::const_iterator it = itemData->sdata.begin();
         it != itemData->sdata.end();
         ++it)
    {
        if (!it->first.empty())
        {
            searchTextUpper += " ";
            searchTextUpper += ToUpperAscii(it->first);
        }
        if (!it->second.empty())
        {
            searchTextUpper += " ";
            searchTextUpper += ToUpperAscii(it->second);
        }
    }

    for (boost::unordered::unordered_map<std::string, std::string>::const_iterator it = itemData->filesdata.begin();
         it != itemData->filesdata.end();
         ++it)
    {
        if (!it->first.empty())
        {
            searchTextUpper += " ";
            searchTextUpper += ToUpperAscii(it->first);
        }
        if (!it->second.empty())
        {
            searchTextUpper += " ";
            searchTextUpper += ToUpperAscii(it->second);
        }
    }

    return searchTextUpper;
}

bool IsInventorySpawnWeaponDataType(const GameData* itemData)
{
    return itemData && (itemData->type == WEAPON || itemData->type == CROSSBOW);
}

bool IsInventorySpawnArmourDataType(const GameData* itemData)
{
    return itemData && itemData->type == ARMOUR;
}

std::string NormalizeGameDataKey(const std::string& value)
{
    std::string normalized;
    normalized.reserve(value.size());

    for (size_t index = 0; index < value.size(); ++index)
    {
        const unsigned char ch = static_cast<unsigned char>(value[index]);
        if (std::isalnum(ch) == 0)
        {
            continue;
        }

        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }

    return normalized;
}

bool DoesSearchTextContainAnyKeyword(
    const std::string& searchTextUpper,
    const char* const* keywords,
    size_t keywordCount)
{
    for (size_t index = 0; index < keywordCount; ++index)
    {
        if (searchTextUpper.find(keywords[index]) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool TryGetGameDataIntValue(const GameData* itemData, const char* key, int* outValue)
{
    if (!itemData || !key || !outValue)
    {
        return false;
    }

    boost::unordered::unordered_map<std::string, int>::const_iterator it = itemData->idata.find(key);
    if (it == itemData->idata.end())
    {
        return false;
    }

    *outValue = it->second;
    return true;
}

bool TryGetGameDataIntValueByNormalizedKeys(
    const GameData* itemData,
    const char* const* normalizedKeys,
    size_t normalizedKeyCount,
    int* outValue)
{
    if (!itemData || !normalizedKeys || normalizedKeyCount == 0 || !outValue)
    {
        return false;
    }

    for (boost::unordered::unordered_map<std::string, int>::const_iterator it = itemData->idata.begin();
         it != itemData->idata.end();
         ++it)
    {
        const std::string normalizedKey = NormalizeGameDataKey(it->first);
        if (normalizedKey.empty())
        {
            continue;
        }

        for (size_t keyIndex = 0; keyIndex < normalizedKeyCount; ++keyIndex)
        {
            if (normalizedKey == normalizedKeys[keyIndex])
            {
                *outValue = it->second;
                return true;
            }
        }
    }

    return false;
}

bool TryGetInventorySpawnItemFunction(const GameData* itemData, ItemFunction* outItemFunction)
{
    if (!itemData || !outItemFunction)
    {
        return false;
    }

    static const char* const kItemFunctionNormalizedKeys[] = {
        "itemfunction"
    };

    int value = 0;
    if (!TryGetGameDataIntValueByNormalizedKeys(
            itemData,
            kItemFunctionNormalizedKeys,
            sizeof(kItemFunctionNormalizedKeys) / sizeof(kItemFunctionNormalizedKeys[0]),
            &value))
    {
        return false;
    }

    if (value < ITEM_NO_FUNCTION || value > ITEM_SEVERED_LIMB)
    {
        return false;
    }

    *outItemFunction = static_cast<ItemFunction>(value);
    return true;
}

bool TryGetInventorySpawnArmourType(const GameData* itemData, int* outArmourType)
{
    static const char* const kArmourTypeNormalizedKeys[] = {
        "armourtype",
        "armortype"
    };

    return TryGetGameDataIntValueByNormalizedKeys(
        itemData,
        kArmourTypeNormalizedKeys,
        sizeof(kArmourTypeNormalizedKeys) / sizeof(kArmourTypeNormalizedKeys[0]),
        outArmourType);
}

bool IsInventorySpawnToolItem(const GameData* itemData, const std::string& searchTextUpper);

bool IsInventorySpawnGeneralItem(const GameData* itemData, const std::string& searchTextUpper)
{
    if (!itemData || IsInventorySpawnWeaponDataType(itemData) || IsInventorySpawnArmourDataType(itemData))
    {
        return false;
    }

    if (IsInventorySpawnToolItem(itemData, searchTextUpper))
    {
        return true;
    }

    return DoesSearchTextContainAnyKeyword(
        searchTextUpper,
        kInventorySpawnGeneralKeywords,
        sizeof(kInventorySpawnGeneralKeywords) / sizeof(kInventorySpawnGeneralKeywords[0]));
}

bool IsInventorySpawnArmourItem(const GameData* itemData, const std::string& searchTextUpper)
{
    if (IsInventorySpawnArmourDataType(itemData))
    {
        return true;
    }

    ItemFunction itemFunction = ITEM_NO_FUNCTION;
    if (TryGetInventorySpawnItemFunction(itemData, &itemFunction) && itemFunction == ITEM_CLOTHING)
    {
        return true;
    }

    int armourType = 0;
    if (TryGetInventorySpawnArmourType(itemData, &armourType))
    {
        return true;
    }

    return DoesSearchTextContainAnyKeyword(
        searchTextUpper,
        kInventorySpawnArmourKeywords,
        sizeof(kInventorySpawnArmourKeywords) / sizeof(kInventorySpawnArmourKeywords[0]));
}

bool IsInventorySpawnWeaponItem(const GameData* itemData, const std::string& searchTextUpper)
{
    if (IsInventorySpawnWeaponDataType(itemData))
    {
        return true;
    }

    ItemFunction itemFunction = ITEM_NO_FUNCTION;
    if (TryGetInventorySpawnItemFunction(itemData, &itemFunction) && itemFunction == ITEM_WEAPON)
    {
        return true;
    }

    return DoesSearchTextContainAnyKeyword(
        searchTextUpper,
        kInventorySpawnWeaponKeywords,
        sizeof(kInventorySpawnWeaponKeywords) / sizeof(kInventorySpawnWeaponKeywords[0]));
}

bool IsInventorySpawnToolItem(const GameData* itemData, const std::string& searchTextUpper)
{
    ItemFunction itemFunction = ITEM_NO_FUNCTION;
    if (TryGetInventorySpawnItemFunction(itemData, &itemFunction) && itemFunction == ITEM_TOOL)
    {
        return true;
    }

    return DoesSearchTextContainAnyKeyword(
        searchTextUpper,
        kInventorySpawnToolKeywords,
        sizeof(kInventorySpawnToolKeywords) / sizeof(kInventorySpawnToolKeywords[0]));
}

bool DoesInventorySpawnItemMatchCategory(
    const GameData* itemData,
    const std::string& searchTextUpper,
    InventorySpawnCategory category)
{
    const bool isFood = Item::isFood(const_cast<GameData*>(itemData));
    const bool isGeneral = IsInventorySpawnGeneralItem(itemData, searchTextUpper);
    const bool isArmour = IsInventorySpawnArmourItem(itemData, searchTextUpper);
    const bool isWeapon = IsInventorySpawnWeaponItem(itemData, searchTextUpper);

    switch (category)
    {
    case InventorySpawnCategory_Food:
        return isFood;
    case InventorySpawnCategory_General:
        return isGeneral;
    case InventorySpawnCategory_Armour:
        return isArmour;
    case InventorySpawnCategory_Weapons:
        return isWeapon;
    case InventorySpawnCategory_All:
    default:
        return isFood || isGeneral || isArmour || isWeapon;
    }
}

InventorySpawnCategory GetSelectedInventorySpawnCategory()
{
    if (!g_itemCategoryDropdown)
    {
        return InventorySpawnCategory_All;
    }

    switch (g_itemCategoryDropdown->getIndexSelected())
    {
    case 1:
        return InventorySpawnCategory_Food;
    case 2:
        return InventorySpawnCategory_General;
    case 3:
        return InventorySpawnCategory_Armour;
    case 4:
        return InventorySpawnCategory_Weapons;
    default:
        return InventorySpawnCategory_All;
    }
}

bool DoesInventorySpawnOptionMatchSearch(const InventorySpawnOption& option, const std::string& searchUpper)
{
    return searchUpper.empty() || option.searchTextUpper.find(searchUpper) != std::string::npos;
}

void EnsureInventoryFoodItemOptionsLoaded();
void RefreshInventoryFoodItemDropdown();

void ResetPendingInventorySearchShortcut()
{
    g_pendingInventorySearchShortcut = PendingInventorySearchShortcut();
}

void ResetInventorySearchEditSnapshot()
{
    g_haveInventorySearchEditSnapshot = false;
    g_inventorySearchEditSnapshot = InventorySearchSnapshot();
}

std::size_t ClampInventorySearchCursor(std::size_t cursor, std::size_t textLength)
{
    return cursor > textLength ? textLength : cursor;
}

InventorySearchSelection NormalizeInventorySearchSelection(
    const InventorySearchSelection& selection,
    std::size_t textLength)
{
    if (!selection.active || selection.length == 0u)
    {
        return InventorySearchSelection(false, ClampInventorySearchCursor(selection.start, textLength), 0u);
    }

    const std::size_t start = ClampInventorySearchCursor(selection.start, textLength);
    const std::size_t maxLength = textLength - start;
    const std::size_t length = selection.length > maxLength ? maxLength : selection.length;
    if (length == 0u)
    {
        return InventorySearchSelection(false, start, 0u);
    }

    return InventorySearchSelection(true, start, length);
}

bool IsInventorySearchTokenSeparator(InventorySearchCodepoint value)
{
    if (value < 0x80u)
    {
        const unsigned char byte = static_cast<unsigned char>(value);
        return byte == ':' || std::isspace(byte) != 0 || std::isalnum(byte) == 0;
    }

    return false;
}

std::size_t FindPreviousInventorySearchTokenBoundary(const InventorySearchText& text, std::size_t cursor)
{
    std::size_t position = ClampInventorySearchCursor(cursor, text.size());

    while (position > 0u && IsInventorySearchTokenSeparator(text[position - 1u]))
    {
        --position;
    }

    while (position > 0u && !IsInventorySearchTokenSeparator(text[position - 1u]))
    {
        --position;
    }

    return position;
}

std::size_t FindNextInventorySearchTokenBoundary(const InventorySearchText& text, std::size_t cursor)
{
    const std::size_t length = text.size();
    std::size_t position = ClampInventorySearchCursor(cursor, length);

    while (position < length && !IsInventorySearchTokenSeparator(text[position]))
    {
        ++position;
    }

    while (position < length && IsInventorySearchTokenSeparator(text[position]))
    {
        ++position;
    }

    return position;
}

InventorySearchEditResult ApplyInventorySearchShortcut(
    InventorySearchShortcutKind shortcut,
    const InventorySearchSnapshot& snapshot)
{
    InventorySearchEditResult result;

    if (shortcut == InventorySearchShortcutKind_None)
    {
        return result;
    }

    const std::size_t textLength = snapshot.text.size();
    const std::size_t cursor = ClampInventorySearchCursor(snapshot.cursor, textLength);
    const InventorySearchSelection selection =
        NormalizeInventorySearchSelection(snapshot.selection, textLength);

    result.handled = true;
    result.text = snapshot.text;
    result.cursor = cursor;
    result.selection = InventorySearchSelection(false, cursor, 0u);

    if (shortcut == InventorySearchShortcutKind_CtrlLeft)
    {
        result.cursor = FindPreviousInventorySearchTokenBoundary(snapshot.text, cursor);
        result.selection = InventorySearchSelection(false, result.cursor, 0u);
        return result;
    }

    if (shortcut == InventorySearchShortcutKind_CtrlRight)
    {
        result.cursor = FindNextInventorySearchTokenBoundary(snapshot.text, cursor);
        result.selection = InventorySearchSelection(false, result.cursor, 0u);
        return result;
    }

    if (selection.active)
    {
        result.rewriteText = true;
        result.text.erase(
            result.text.begin() + selection.start,
            result.text.begin() + selection.start + selection.length);
        result.cursor = selection.start;
        result.selection = InventorySearchSelection(false, result.cursor, 0u);
        return result;
    }

    result.rewriteText = true;
    const std::size_t deleteStart = FindPreviousInventorySearchTokenBoundary(snapshot.text, cursor);
    if (deleteStart != cursor)
    {
        result.text.erase(result.text.begin() + deleteStart, result.text.begin() + cursor);
    }
    result.cursor = deleteStart;
    result.selection = InventorySearchSelection(false, result.cursor, 0u);
    return result;
}

bool IsInterestingInventorySearchMyGuiKey(MyGUI::KeyCode keyCode)
{
    const int value = keyCode.getValue();
    return value == MyGUI::KeyCode::LeftControl
        || value == MyGUI::KeyCode::RightControl
        || value == MyGUI::KeyCode::ArrowLeft
        || value == MyGUI::KeyCode::ArrowRight
        || value == MyGUI::KeyCode::Backspace;
}

InventorySearchText ToInventorySearchInputText(const MyGUI::UString& text)
{
    InventorySearchText result;
    const std::size_t length = text.size();
    result.reserve(length);
    for (std::size_t index = 0; index < length; ++index)
    {
        result.push_back(static_cast<InventorySearchCodepoint>(text[index]));
    }
    return result;
}

MyGUI::UString ToInventorySearchMyGuiText(const InventorySearchText& text)
{
    MyGUI::UString result;
    const std::size_t length = text.size();
    for (std::size_t index = 0; index < length; ++index)
    {
        result.push_back(static_cast<MyGUI::UString::unicode_char>(text[index]));
    }
    return result;
}

InventorySearchSelection CaptureInventorySearchEditSelection(
    MyGUI::EditBox* searchEdit,
    std::size_t textLength)
{
    if (!searchEdit || !searchEdit->isTextSelection())
    {
        return InventorySearchSelection();
    }

    const std::size_t selectionStart = searchEdit->getTextSelectionStart();
    if (selectionStart == MyGUI::ITEM_NONE)
    {
        return InventorySearchSelection();
    }

    const std::size_t selectionLength = searchEdit->getTextSelectionLength();
    return NormalizeInventorySearchSelection(
        InventorySearchSelection(true, selectionStart, selectionLength),
        textLength);
}

InventorySearchSnapshot BuildInventorySearchInputSnapshot(
    const MyGUI::UString& text,
    std::size_t cursorPosition,
    const InventorySearchSelection& selection)
{
    return InventorySearchSnapshot(
        ToInventorySearchInputText(text),
        cursorPosition,
        NormalizeInventorySearchSelection(selection, text.size()));
}

InventorySearchSnapshot CaptureInventorySearchEditSnapshot(MyGUI::EditBox* searchEdit)
{
    if (!searchEdit)
    {
        return InventorySearchSnapshot();
    }

    const MyGUI::UString text = searchEdit->getOnlyText();
    const std::size_t textLength = text.size();
    const std::size_t cursorPosition =
        ClampInventorySearchCursor(searchEdit->getTextCursor(), textLength);
    return BuildInventorySearchInputSnapshot(
        text,
        cursorPosition,
        CaptureInventorySearchEditSelection(searchEdit, textLength));
}

void RememberInventorySearchEditSnapshot(MyGUI::EditBox* searchEdit)
{
    if (!searchEdit)
    {
        ResetInventorySearchEditSnapshot();
        return;
    }

    g_haveInventorySearchEditSnapshot = true;
    g_inventorySearchEditSnapshot = CaptureInventorySearchEditSnapshot(searchEdit);
}

InventorySearchShortcutKind ClassifyInventorySearchShortcut(MyGUI::KeyCode keyCode)
{
    const int keyValue = keyCode.getValue();
    if (keyValue == MyGUI::KeyCode::ArrowLeft)
    {
        return InventorySearchShortcutKind_CtrlLeft;
    }

    if (keyValue == MyGUI::KeyCode::ArrowRight)
    {
        return InventorySearchShortcutKind_CtrlRight;
    }

    if (keyValue == MyGUI::KeyCode::Backspace)
    {
        return InventorySearchShortcutKind_CtrlBackspace;
    }

    return InventorySearchShortcutKind_None;
}

InventorySearchSnapshot BuildScheduledInventorySearchShortcutSnapshot(
    MyGUI::EditBox* searchEdit,
    InventorySearchShortcutKind shortcut)
{
    InventorySearchSnapshot snapshot = CaptureInventorySearchEditSnapshot(searchEdit);
    if (shortcut == InventorySearchShortcutKind_CtrlBackspace && g_haveInventorySearchEditSnapshot)
    {
        snapshot.text = g_inventorySearchEditSnapshot.text;
        snapshot.cursor =
            ClampInventorySearchCursor(g_inventorySearchEditSnapshot.cursor, snapshot.text.size());
        snapshot.selection = NormalizeInventorySearchSelection(snapshot.selection, snapshot.text.size());
    }

    return snapshot;
}

void ApplyInventorySearchEditSelection(
    MyGUI::EditBox* searchEdit,
    std::size_t cursorPosition,
    const InventorySearchSelection& selection)
{
    if (!searchEdit)
    {
        return;
    }

    const std::size_t textLength = searchEdit->getTextLength();
    const std::size_t clampedCursor = ClampInventorySearchCursor(cursorPosition, textLength);
    const InventorySearchSelection normalizedSelection =
        NormalizeInventorySearchSelection(selection, textLength);
    if (normalizedSelection.active)
    {
        searchEdit->setTextSelection(
            normalizedSelection.start,
            normalizedSelection.start + normalizedSelection.length);
        return;
    }

    searchEdit->setTextCursor(clampedCursor);
    searchEdit->setTextSelection(clampedCursor, clampedCursor);
}

bool ScheduleInventorySearchMyGuiShortcut(MyGUI::EditBox* searchEdit, MyGUI::KeyCode keyCode)
{
    if (!searchEdit)
    {
        return false;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    if (!inputManager || !inputManager->isControlPressed())
    {
        return false;
    }

    const InventorySearchShortcutKind shortcut = ClassifyInventorySearchShortcut(keyCode);
    if (shortcut == InventorySearchShortcutKind_None)
    {
        return false;
    }

    ResetPendingInventorySearchShortcut();
    g_pendingInventorySearchShortcut.active = true;
    g_pendingInventorySearchShortcut.keyValue = keyCode.getValue();
    g_pendingInventorySearchShortcut.editResult = ApplyInventorySearchShortcut(
        shortcut,
        BuildScheduledInventorySearchShortcutSnapshot(searchEdit, shortcut));
    if (!g_pendingInventorySearchShortcut.editResult.handled)
    {
        ResetPendingInventorySearchShortcut();
        return false;
    }

    return true;
}

void ApplyPendingInventorySearchEditShortcut(MyGUI::EditBox* searchEdit, MyGUI::KeyCode keyCode)
{
    if (!g_pendingInventorySearchShortcut.active
        || g_pendingInventorySearchShortcut.keyValue != keyCode.getValue())
    {
        return;
    }

    const PendingInventorySearchShortcut pending = g_pendingInventorySearchShortcut;
    ResetPendingInventorySearchShortcut();

    if (!searchEdit || !pending.editResult.handled)
    {
        return;
    }

    if (pending.editResult.rewriteText)
    {
        searchEdit->setOnlyText(ToInventorySearchMyGuiText(pending.editResult.text));
    }

    ApplyInventorySearchEditSelection(searchEdit, pending.editResult.cursor, pending.editResult.selection);
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

unsigned long long GetCurrentUtcTimestamp()
{
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);

    ULARGE_INTEGER value;
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    return value.QuadPart;
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

void RefreshInventoryFoodItemDropdown()
{
    if (!g_itemSearchResultsList)
    {
        g_filteredInventoryFoodItemOptionIndexes.clear();
        return;
    }

    size_t previouslySelectedOptionIndex = MyGUI::ITEM_NONE;
    const size_t previousSelectedIndex = g_itemSearchResultsList->getIndexSelected();
    if (previousSelectedIndex < g_filteredInventoryFoodItemOptionIndexes.size())
    {
        previouslySelectedOptionIndex = g_filteredInventoryFoodItemOptionIndexes[previousSelectedIndex];
    }

    g_itemSearchResultsList->removeAllItems();
    g_filteredInventoryFoodItemOptionIndexes.clear();

    std::string searchUpper;
    if (g_itemSearchEdit)
    {
        searchUpper = ToUpperAscii(TrimAscii(g_itemSearchEdit->getOnlyText().asUTF8()));
    }
    const InventorySpawnCategory category = GetSelectedInventorySpawnCategory();

    for (size_t index = 0; index < g_inventoryFoodItemOptions.size(); ++index)
    {
        const InventorySpawnOption& option = g_inventoryFoodItemOptions[index];
        if (!DoesInventorySpawnItemMatchCategory(option.itemData, option.searchTextUpper, category))
        {
            continue;
        }
        if (!DoesInventorySpawnOptionMatchSearch(option, searchUpper))
        {
            continue;
        }

        g_filteredInventoryFoodItemOptionIndexes.push_back(index);
        g_itemSearchResultsList->addItem(option.displayName);
    }

    if (g_filteredInventoryFoodItemOptionIndexes.empty())
    {
        if (!g_inventoryFoodItemOptionsLoaded)
        {
            g_itemSearchResultsList->addItem("Loading items...");
        }
        else if (g_inventoryFoodItemOptions.empty())
        {
            g_itemSearchResultsList->addItem("No spawnable items available");
        }
        else
        {
            g_itemSearchResultsList->addItem("No matching items");
        }

        g_itemSearchResultsList->clearIndexSelected();
        g_itemSearchResultsList->beginToItemFirst();
        RefreshInventorySpawnButtonState();
        return;
    }

    size_t nextSelectedIndex = MyGUI::ITEM_NONE;
    if (previouslySelectedOptionIndex != MyGUI::ITEM_NONE)
    {
        for (size_t filteredIndex = 0; filteredIndex < g_filteredInventoryFoodItemOptionIndexes.size(); ++filteredIndex)
        {
            if (g_filteredInventoryFoodItemOptionIndexes[filteredIndex] == previouslySelectedOptionIndex)
            {
                nextSelectedIndex = filteredIndex;
                break;
            }
        }
    }

    if (nextSelectedIndex == MyGUI::ITEM_NONE && g_filteredInventoryFoodItemOptionIndexes.size() == 1u)
    {
        nextSelectedIndex = 0u;
    }

    if (nextSelectedIndex != MyGUI::ITEM_NONE)
    {
        g_itemSearchResultsList->setIndexSelected(nextSelectedIndex);
        g_itemSearchResultsList->beginToItemSelected();
    }
    else
    {
        g_itemSearchResultsList->clearIndexSelected();
        g_itemSearchResultsList->beginToItemFirst();
    }

    RefreshInventorySpawnButtonState();
}

void ResetInventoryFoodItemOptions()
{
    g_inventoryFoodItemOptions.clear();
    g_filteredInventoryFoodItemOptionIndexes.clear();
    g_inventoryFoodItemOptionsLoaded = false;
}

void EnsureInventoryFoodItemOptionsLoaded()
{
    if (g_inventoryFoodItemOptionsLoaded || !ou || !ou->initialized)
    {
        return;
    }

    lektor<GameData*> itemDatas;
    ou->gamedata.getDataOfType(itemDatas, ITEM);
    lektor<GameData*> weaponDatas;
    ou->gamedata.getDataOfType(weaponDatas, WEAPON);
    lektor<GameData*> armourDatas;
    ou->gamedata.getDataOfType(armourDatas, ARMOUR);
    lektor<GameData*> crossbowDatas;
    ou->gamedata.getDataOfType(crossbowDatas, CROSSBOW);

    g_inventoryFoodItemOptions.clear();

    for (lektor<GameData*>::const_iterator it = itemDatas.begin(); it != itemDatas.end(); ++it)
    {
        GameData* itemData = *it;
        if (!itemData || !itemData->isValid())
        {
            continue;
        }

        InventorySpawnOption option;
        option.displayName = BuildInventorySpawnOptionLabel(itemData);
        option.searchTextUpper = BuildInventorySpawnOptionSearchText(itemData, option.displayName);
        if (!DoesInventorySpawnItemMatchCategory(itemData, option.searchTextUpper, InventorySpawnCategory_All))
        {
            continue;
        }

        option.itemData = itemData;
        g_inventoryFoodItemOptions.push_back(option);
    }

    for (lektor<GameData*>::const_iterator it = weaponDatas.begin(); it != weaponDatas.end(); ++it)
    {
        GameData* itemData = *it;
        if (!itemData || !itemData->isValid())
        {
            continue;
        }

        InventorySpawnOption option;
        option.displayName = BuildInventorySpawnOptionLabel(itemData);
        option.searchTextUpper = BuildInventorySpawnOptionSearchText(itemData, option.displayName);
        option.itemData = itemData;
        g_inventoryFoodItemOptions.push_back(option);
    }

    for (lektor<GameData*>::const_iterator it = armourDatas.begin(); it != armourDatas.end(); ++it)
    {
        GameData* itemData = *it;
        if (!itemData || !itemData->isValid())
        {
            continue;
        }

        InventorySpawnOption option;
        option.displayName = BuildInventorySpawnOptionLabel(itemData);
        option.searchTextUpper = BuildInventorySpawnOptionSearchText(itemData, option.displayName);
        option.itemData = itemData;
        g_inventoryFoodItemOptions.push_back(option);
    }

    for (lektor<GameData*>::const_iterator it = crossbowDatas.begin(); it != crossbowDatas.end(); ++it)
    {
        GameData* itemData = *it;
        if (!itemData || !itemData->isValid())
        {
            continue;
        }

        InventorySpawnOption option;
        option.displayName = BuildInventorySpawnOptionLabel(itemData);
        option.searchTextUpper = BuildInventorySpawnOptionSearchText(itemData, option.displayName);
        option.itemData = itemData;
        g_inventoryFoodItemOptions.push_back(option);
    }

    std::sort(
        g_inventoryFoodItemOptions.begin(),
        g_inventoryFoodItemOptions.end(),
        [](const InventorySpawnOption& left, const InventorySpawnOption& right)
        {
            return left.displayName < right.displayName;
        });

    g_inventoryFoodItemOptionsLoaded = true;
    RefreshInventoryFoodItemDropdown();
}

bool TryResolveSelectedInventoryFoodItem(GameData** itemDataOut, std::string* itemLabelOut)
{
    if (itemDataOut)
    {
        *itemDataOut = 0;
    }
    if (itemLabelOut)
    {
        itemLabelOut->clear();
    }

    if (!g_itemSearchResultsList || g_filteredInventoryFoodItemOptionIndexes.empty())
    {
        return false;
    }

    const size_t selectedIndex = g_itemSearchResultsList->getIndexSelected();
    if (selectedIndex >= g_filteredInventoryFoodItemOptionIndexes.size())
    {
        return false;
    }

    const InventorySpawnOption& option =
        g_inventoryFoodItemOptions[g_filteredInventoryFoodItemOptionIndexes[selectedIndex]];
    if (!option.itemData)
    {
        return false;
    }

    if (itemDataOut)
    {
        *itemDataOut = option.itemData;
    }
    if (itemLabelOut)
    {
        *itemLabelOut = option.displayName;
    }

    return true;
}

std::string BuildSavedLocationDisplayName(const SavedLocation& location)
{
    if (location.pinned)
    {
        return std::string("[Pinned] ") + location.name;
    }

    return location.name;
}

void UpdateSavedLocationsCollapseButtonCaption()
{
    if (!g_savedLocationsCollapseButton)
    {
        return;
    }

    g_savedLocationsCollapseButton->setCaption(g_savedLocationsCollapsed ? "+" : "-");
}

std::string BuildSavedLocationListEntry(size_t displayIndex, const SavedLocation& location)
{
    std::stringstream entry;
    entry << (displayIndex + 1u) << ". " << BuildSavedLocationDisplayName(location);
    return entry.str();
}

bool DoesSavedLocationMatchSearch(const SavedLocation& location, const std::string& searchUpper)
{
    if (searchUpper.empty())
    {
        return true;
    }

    return ToUpperAscii(location.name).find(searchUpper) != std::string::npos;
}

void RefreshSaveLocationInputUi()
{
    if (g_saveLocationNameLabelText)
    {
        g_saveLocationNameLabelText->setCaption(
            g_savedLocationRenameId.empty()
                ? "Location Name (Enter to save)"
                : "Rename Location (Enter to confirm)");
    }

    if (g_saveSelectedLocationButton)
    {
        g_saveSelectedLocationButton->setCaption(g_savedLocationRenameId.empty() ? "Save Selected Location" : "Save Rename");
    }
}

void ClearSavedLocationRenameState(bool clearInputText)
{
    g_savedLocationRenameId.clear();
    RefreshSaveLocationInputUi();

    if (clearInputText && g_saveLocationNameEdit)
    {
        g_saveLocationNameEdit->setOnlyText("");
    }
}

void BeginSavedLocationRename(const SavedLocation& location)
{
    g_savedLocationRenameId = location.id;
    RefreshSaveLocationInputUi();

    if (g_saveLocationNameEdit)
    {
        g_saveLocationNameEdit->setOnlyText(location.name);
    }
}

void RefreshSavedLocationsListWidget()
{
    if (!g_savedLocationsListBox || !g_savedLocationsEmptyText)
    {
        return;
    }

    g_filteredSavedLocationIndexes.clear();
    g_savedLocationsListBox->removeAllItems();

    const std::string searchUpper = ToUpperAscii(TrimAscii(g_savedLocationSearchText));
    for (size_t index = 0; index < g_savedLocations.size(); ++index)
    {
        const SavedLocation& location = g_savedLocations[index];
        if (!DoesSavedLocationMatchSearch(location, searchUpper))
        {
            continue;
        }

        g_filteredSavedLocationIndexes.push_back(index);
        g_savedLocationsListBox->addItem(BuildSavedLocationListEntry(g_filteredSavedLocationIndexes.size() - 1u, location));
    }

    if (g_filteredSavedLocationIndexes.empty())
    {
        g_selectedSavedLocationId.clear();
        g_savedLocationsListBox->clearIndexSelected();
        g_savedLocationsListBox->setVisible(false);
        g_savedLocationsEmptyText->setCaption(g_savedLocations.empty() ? "No saved locations yet" : "No matching saved locations");
        g_savedLocationsEmptyText->setVisible(true);
        RefreshSavedLocationActionButtons(g_lastPlayerInterface);
        return;
    }

    g_savedLocationsListBox->setVisible(true);
    g_savedLocationsEmptyText->setVisible(false);

    size_t selectedFilteredIndex = MyGUI::ITEM_NONE;
    if (!g_selectedSavedLocationId.empty())
    {
        for (size_t filteredIndex = 0; filteredIndex < g_filteredSavedLocationIndexes.size(); ++filteredIndex)
        {
            if (g_savedLocations[g_filteredSavedLocationIndexes[filteredIndex]].id == g_selectedSavedLocationId)
            {
                selectedFilteredIndex = filteredIndex;
                break;
            }
        }
    }

    if (selectedFilteredIndex == MyGUI::ITEM_NONE)
    {
        selectedFilteredIndex = 0u;
        g_selectedSavedLocationId = g_savedLocations[g_filteredSavedLocationIndexes[selectedFilteredIndex]].id;
    }

    g_savedLocationsListBox->setIndexSelected(selectedFilteredIndex);
    g_savedLocationsListBox->beginToItemAt(selectedFilteredIndex);
    RefreshSavedLocationActionButtons(g_lastPlayerInterface);
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

void FocusCameraOnTeleportedSelection(PlayerInterface* player, const Ogre::Vector3& destination)
{
    if (!player)
    {
        return;
    }

    __try
    {
        if (player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter() != 0)
        {
            player->focusCameraSelectedCharacter();
            return;
        }

        player->focusCamera(destination);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void StopTeleportedSelectionMovement(PlayerInterface* player)
{
    if (!player)
    {
        return;
    }

    __try
    {
        player->stopCharactersMovement();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool AreFloatsNearlyEqual(float left, float right)
{
    return std::fabs(left - right) <= kFloatChangeEpsilon;
}

bool AreVectorsNearlyEqual(const Ogre::Vector3& left, const Ogre::Vector3& right)
{
    return AreFloatsNearlyEqual(left.x, right.x)
        && AreFloatsNearlyEqual(left.y, right.y)
        && AreFloatsNearlyEqual(left.z, right.z);
}

const char* GetProneStateLabel(ProneState proneState)
{
    switch (proneState)
    {
    case PS_NORMAL:
        return "normal";
    case PS_KO:
        return "ko";
    case PS_PLAYING_DEAD:
        return "playing_dead";
    case PS_CRIPPLED:
        return "crippled";
    default:
        return "unknown";
    }
}

void AppendDownedStateLogFields(
    std::stringstream& line,
    const char* prefix,
    const DownedTeleportState& state)
{
    const std::string fieldPrefix = prefix ? prefix : "state";
    line << " " << fieldPrefix << "_active=" << (state.active ? "true" : "false")
         << " " << fieldPrefix << "_prone=\"" << GetProneStateLabel(state.proneState) << "\""
         << " " << fieldPrefix << "_wants_get_up=" << (state.playerWantsMeToGetUp ? "true" : "false")
         << " " << fieldPrefix << "_crippled=" << (state.crippled ? "true" : "false")
         << " " << fieldPrefix << "_unconscious=" << (state.unconscious ? "true" : "false")
         << " " << fieldPrefix << "_sub50ko=" << (state.sub50KO ? "true" : "false")
         << " " << fieldPrefix << "_bloodloss=" << (state.bloodlossTrauma ? "true" : "false")
         << " " << fieldPrefix << "_knockout_timer=" << state.knockoutTimer;
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

bool TryGetDownedTeleportState(Character* character, DownedTeleportState* outState)
{
    if (outState)
    {
        *outState = DownedTeleportState();
    }

    if (!character || !outState)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = character->getMedical();
        if (!medical)
        {
            return false;
        }

        outState->proneState = character->_NV_getProneState();
        outState->playerWantsMeToGetUp = character->playerWantsMeToGetUp;
        outState->crippled = medical->crippled;
        outState->unconscious = medical->unconcious;
        outState->sub50KO = medical->sub50KO;
        outState->bloodlossTrauma = medical->bloodlossTrauma;
        outState->knockoutTimer = medical->knockoutTimer;
        outState->active =
            outState->proneState == PS_KO
            || outState->proneState == PS_PLAYING_DEAD
            || outState->proneState == PS_CRIPPLED
            || outState->unconscious
            || outState->sub50KO
            || outState->crippled
            || outState->bloodlossTrauma
            || outState->knockoutTimer > 0.0f;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool DoesSnapshotRawPositionMatchDestination(
    const CharacterPositionSnapshot& snapshot,
    const Ogre::Vector3& destination)
{
    return snapshot.hasRawPosition && AreVectorsNearlyEqual(snapshot.rawPosition, destination);
}

bool DoesSnapshotBodyPositionMatchDestination(
    const CharacterPositionSnapshot& snapshot,
    const Ogre::Vector3& destination)
{
    if (snapshot.hasPosition && AreVectorsNearlyEqual(snapshot.position, destination))
    {
        return true;
    }

    return snapshot.hasRawEntityPosition && AreVectorsNearlyEqual(snapshot.rawEntityPosition, destination);
}

bool ShouldRetryExactTeleportWithMovement(
    const CharacterPositionSnapshot& beforeSnapshot,
    const CharacterPositionSnapshot& afterSnapshot)
{
    if (!beforeSnapshot.hasPosition || !afterSnapshot.hasPosition)
    {
        return false;
    }

    if (!AreVectorsNearlyEqual(beforeSnapshot.position, afterSnapshot.position))
    {
        return false;
    }

    if (beforeSnapshot.hasRawEntityPosition && afterSnapshot.hasRawEntityPosition)
    {
        return AreVectorsNearlyEqual(beforeSnapshot.rawEntityPosition, afterSnapshot.rawEntityPosition);
    }

    return true;
}

bool TryRetryExactTeleportWithMovement(
    Character* character,
    const Ogre::Vector3& destination,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    __try
    {
        character->teleportVisuallyOnly(destination, character->getOrientation());
        character->teleportFromAnimation();
        character->setTerrainHeightPosition(destination.y);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }

    return true;
}

bool TryRetryExactTeleportWithRelocation(
    Character* character,
    const Ogre::Vector3& moveBy,
    const Ogre::Vector3& destination,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    __try
    {
        character->relocationTeleport(moveBy);
        character->setTerrainHeightPosition(destination.y);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }

    return true;
}

bool TryTemporarilyClearDownedCharacterForTeleport(
    Character* character,
    const DownedTeleportState& downedState)
{
    if (!character || !downedState.active)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = character->getMedical();
        if (!medical)
        {
            return false;
        }

        character->playerWantsMeToGetUp = true;
        if (downedState.proneState != PS_NORMAL)
        {
            character->_NV_setProneState(PS_NORMAL);
        }

        medical->crippled = false;
        medical->unconcious = false;
        medical->sub50KO = false;
        medical->bloodlossTrauma = false;
        medical->knockoutTimer = 0.0f;
        medical->validateHealthValues();
        medical->_reassessRagdollPartsAssumingWeJustClearedTheEntireRagdoll();
        medical->reassessCollapseMode(false, false);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryRestoreDownedCharacterAfterTeleport(
    Character* character,
    const DownedTeleportState& downedState)
{
    if (!character || !downedState.active)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = character->getMedical();
        if (!medical)
        {
            return false;
        }

        character->playerWantsMeToGetUp = downedState.playerWantsMeToGetUp;
        medical->crippled = downedState.crippled;
        medical->unconcious = downedState.unconscious;
        medical->sub50KO = downedState.sub50KO;
        medical->bloodlossTrauma = downedState.bloodlossTrauma;
        medical->knockoutTimer = downedState.knockoutTimer;
        character->_NV_setProneState(downedState.proneState);
        medical->validateHealthValues();
        medical->_reassessRagdollPartsAssumingWeJustClearedTheEntireRagdoll();
        medical->reassessCollapseMode(false, downedState.bloodlossTrauma);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryRetryExactTeleportWithDelayedDownedRestore(
    Character* character,
    const Ogre::Vector3& destination,
    const DownedTeleportState& downedState,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    __try
    {
        if (!TryTemporarilyClearDownedCharacterForTeleport(character, downedState))
        {
            return false;
        }

        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        TryRestoreDownedCharacterAfterTeleport(character, downedState);
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }
    return true;
}

bool TryAdvancePendingDownedTeleport(
    Character* character,
    const Ogre::Vector3& destination,
    const DownedTeleportState& downedState,
    bool* clearAppliedOut,
    bool* teleportCalledOut,
    bool* relocationSyncCalledOut,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (clearAppliedOut)
    {
        *clearAppliedOut = false;
    }
    if (teleportCalledOut)
    {
        *teleportCalledOut = false;
    }
    if (relocationSyncCalledOut)
    {
        *relocationSyncCalledOut = false;
    }
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    CharacterPositionSnapshot beforeSnapshot;
    const bool hasBeforeSnapshot = TryGetCharacterPositionSnapshot(character, &beforeSnapshot);
    if (hasBeforeSnapshot && DoesSnapshotBodyPositionMatchDestination(beforeSnapshot, destination))
    {
        if (afterSnapshotOut)
        {
            *afterSnapshotOut = beforeSnapshot;
        }
        return true;
    }

    __try
    {
        if (!TryTemporarilyClearDownedCharacterForTeleport(character, downedState))
        {
            return false;
        }
        if (clearAppliedOut)
        {
            *clearAppliedOut = true;
        }

        character->teleport(destination, character->getOrientation());
        if (teleportCalledOut)
        {
            *teleportCalledOut = true;
        }

        character->relocationTeleport(Ogre::Vector3(0.0f, 0.0f, 0.0f));
        if (relocationSyncCalledOut)
        {
            *relocationSyncCalledOut = true;
        }

        character->setTerrainHeightPosition(destination.y);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }

    return true;
}

void LogDownedTeleportFallbackSelectionInvestigation(
    const char* actionId,
    Character* character,
    const Ogre::Vector3& destination,
    bool observedDowned,
    bool useDownedRagdollSync,
    const CharacterPositionSnapshot& beforeSnapshot,
    const CharacterPositionSnapshot& afterSnapshot,
    const DownedTeleportState& downedState)
{
    if (!g_developerMode || !observedDowned)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][teleport_downed] action=\""
         << SanitizeLogValue(actionId ? actionId : "")
         << "\" phase=\"fallback_selected\""
         << " target_name=\"" << SanitizeLogValue(SafeCharacterName(character)) << "\""
         << " sync_mode=\"" << (useDownedRagdollSync ? "delayed_restore" : "relocation_move") << "\""
         << " destination_x=" << destination.x
         << " destination_y=" << destination.y
         << " destination_z=" << destination.z
         << " before_body_at_destination=" << (DoesSnapshotBodyPositionMatchDestination(beforeSnapshot, destination) ? "true" : "false")
         << " before_raw_at_destination=" << (DoesSnapshotRawPositionMatchDestination(beforeSnapshot, destination) ? "true" : "false")
         << " after_body_at_destination=" << (DoesSnapshotBodyPositionMatchDestination(afterSnapshot, destination) ? "true" : "false")
         << " after_raw_at_destination=" << (DoesSnapshotRawPositionMatchDestination(afterSnapshot, destination) ? "true" : "false");
    AppendDownedStateLogFields(line, "detected", downedState);
    LogInfoLine(line.str());
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

void LogSavedLocationPositionInvestigation(
    const char* actionId,
    const std::string& locationName,
    const std::string& targetName,
    const CharacterPositionSnapshot& snapshot,
    const Ogre::Vector3& selectedPosition,
    const char* selectedSource)
{
    if (!g_developerMode)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][saved_location] action=\"" << actionId << "\""
         << " location_name=\"" << SanitizeLogValue(locationName) << "\""
         << " target_name=\"" << SanitizeLogValue(targetName) << "\""
         << " selected_source=\"" << SanitizeLogValue(selectedSource ? selectedSource : "unknown") << "\""
         << " selected_x=" << selectedPosition.x
         << " selected_y=" << selectedPosition.y
         << " selected_z=" << selectedPosition.z;
    if (snapshot.hasPosition)
    {
        line << " position_x=" << snapshot.position.x
             << " position_y=" << snapshot.position.y
             << " position_z=" << snapshot.position.z;
    }
    if (snapshot.hasRawEntityPosition)
    {
        line << " raw_entity_x=" << snapshot.rawEntityPosition.x
             << " raw_entity_y=" << snapshot.rawEntityPosition.y
             << " raw_entity_z=" << snapshot.rawEntityPosition.z;
    }
    if (snapshot.hasRawPosition)
    {
        line << " raw_x=" << snapshot.rawPosition.x
             << " raw_y=" << snapshot.rawPosition.y
             << " raw_z=" << snapshot.rawPosition.z;
    }
    if (snapshot.hasTerrainHeight)
    {
        line << " terrain_height=" << snapshot.terrainHeight;
    }
    LogInfoLine(line.str());
}

void ClearPendingDownedTeleportRestores()
{
    g_pendingDownedTeleportRestores.clear();
}

void SchedulePendingDownedTeleportRestore(
    const char* actionId,
    Character* character,
    const DownedTeleportState& downedState,
    const Ogre::Vector3& destination)
{
    if (!character || !downedState.active)
    {
        return;
    }

    for (std::size_t index = 0; index < g_pendingDownedTeleportRestores.size();)
    {
        if (g_pendingDownedTeleportRestores[index].character == character)
        {
            g_pendingDownedTeleportRestores.erase(g_pendingDownedTeleportRestores.begin() + index);
            continue;
        }

        ++index;
    }

    PendingDownedTeleportRestore pending;
    pending.character = character;
    pending.actionId = actionId ? actionId : "";
    pending.targetName = SafeCharacterName(character);
    pending.state = downedState;
    pending.destination = destination;
    pending.active = true;
    g_pendingDownedTeleportRestores.push_back(pending);
}

void TickPendingDownedTeleportRestores()
{
    for (std::size_t index = 0; index < g_pendingDownedTeleportRestores.size();)
    {
        PendingDownedTeleportRestore& pending = g_pendingDownedTeleportRestores[index];
        if (!pending.active)
        {
            g_pendingDownedTeleportRestores.erase(g_pendingDownedTeleportRestores.begin() + index);
            continue;
        }

        ++pending.ageTicks;
        CharacterPositionSnapshot syncSnapshot;
        bool clearApplied = false;
        bool teleportCalled = false;
        bool relocationSyncCalled = false;
        const bool syncApplied = TryAdvancePendingDownedTeleport(
            pending.character,
            pending.destination,
            pending.state,
            &clearApplied,
            &teleportCalled,
            &relocationSyncCalled,
            &syncSnapshot);
        const bool bodyAtDestination =
            syncApplied && DoesSnapshotBodyPositionMatchDestination(syncSnapshot, pending.destination);

        if (pending.ageTicks < kDownedTeleportRestoreMinDelayTicks)
        {
            ++index;
            continue;
        }

        if (!bodyAtDestination && pending.ageTicks < kDownedTeleportRestoreMaxDelayTicks)
        {
            ++index;
            continue;
        }

        const bool restored = TryRestoreDownedCharacterAfterTeleport(pending.character, pending.state);
        DownedTeleportState restoredState;
        const bool hasRestoredState = TryGetDownedTeleportState(pending.character, &restoredState);
        CharacterPositionSnapshot restoredSnapshot;
        const bool hasRestoredSnapshot = TryGetCharacterPositionSnapshot(pending.character, &restoredSnapshot);
        if (g_developerMode && (!bodyAtDestination || !restored))
        {
            std::stringstream line;
            line << "[investigate][teleport_restore] action=\""
                 << SanitizeLogValue(pending.actionId)
                 << "\" phase=\"final\""
                 << " target_name=\"" << SanitizeLogValue(pending.targetName) << "\""
                 << " tick=" << pending.ageTicks
                 << " restore_reason=\"" << (bodyAtDestination ? "arrived" : "timeout") << "\""
                 << " sync_applied=" << (syncApplied ? "true" : "false")
                 << " clear_applied=" << (clearApplied ? "true" : "false")
                 << " teleport_called=" << (teleportCalled ? "true" : "false")
                 << " relocation_zero_called=" << (relocationSyncCalled ? "true" : "false")
                 << " body_at_destination=" << (bodyAtDestination ? "true" : "false")
                 << " restored=" << (restored ? "true" : "false")
                 << " restored_state_captured=" << (hasRestoredState ? "true" : "false")
                 << " restored_snapshot_captured=" << (hasRestoredSnapshot ? "true" : "false");
            if (hasRestoredState)
            {
                AppendDownedStateLogFields(line, "restored", restoredState);
            }
            if (hasRestoredSnapshot)
            {
                AppendCharacterSnapshotLogFields(line, "restored", restoredSnapshot);
            }
            LogInfoLine(line.str());
        }

        g_pendingDownedTeleportRestores.erase(g_pendingDownedTeleportRestores.begin() + index);
    }
}

std::string NormalizeSavedLocationName(const std::string& name)
{
    return ToUpperAscii(TrimAscii(name));
}

bool DoesSavedLocationNameExist(const std::vector<SavedLocation>& locations, const std::string& candidateName)
{
    const std::string normalizedCandidate = NormalizeSavedLocationName(candidateName);
    if (normalizedCandidate.empty())
    {
        return false;
    }

    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (NormalizeSavedLocationName(locations[index].name) == normalizedCandidate)
        {
            return true;
        }
    }

    return false;
}

bool DoesSavedLocationNameExistExcludingId(
    const std::vector<SavedLocation>& locations,
    const std::string& candidateName,
    const std::string& excludedLocationId)
{
    const std::string normalizedCandidate = NormalizeSavedLocationName(candidateName);
    if (normalizedCandidate.empty())
    {
        return false;
    }

    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (locations[index].id == excludedLocationId)
        {
            continue;
        }

        if (NormalizeSavedLocationName(locations[index].name) == normalizedCandidate)
        {
            return true;
        }
    }

    return false;
}

bool DoesSavedLocationIdExist(const std::vector<SavedLocation>& locations, const std::string& candidateId)
{
    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (locations[index].id == candidateId)
        {
            return true;
        }
    }

    return false;
}

std::string BuildNextSavedLocationId(const std::vector<SavedLocation>& locations)
{
    unsigned int nextNumber = static_cast<unsigned int>(locations.size() + 1u);
    std::string candidateId;
    do
    {
        std::stringstream stream;
        stream << "saved_location_" << nextNumber;
        candidateId = stream.str();
        ++nextNumber;
    }
    while (DoesSavedLocationIdExist(locations, candidateId));

    return candidateId;
}

size_t FindSavedLocationIndexById(const std::vector<SavedLocation>& locations, const std::string& locationId)
{
    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (locations[index].id == locationId)
        {
            return index;
        }
    }

    return locations.size();
}

bool TryGetSelectedSavedLocation(size_t* indexOut, SavedLocation* locationOut)
{
    if (g_selectedSavedLocationId.empty())
    {
        return false;
    }

    const size_t selectedIndex = FindSavedLocationIndexById(g_savedLocations, g_selectedSavedLocationId);
    if (selectedIndex >= g_savedLocations.size())
    {
        return false;
    }

    if (indexOut)
    {
        *indexOut = selectedIndex;
    }
    if (locationOut)
    {
        *locationOut = g_savedLocations[selectedIndex];
    }

    return true;
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

bool IsInventorySearchEditFocused()
{
    if (!g_itemSearchEdit)
    {
        return false;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    return inputManager != 0 && inputManager->getKeyFocusWidget() == g_itemSearchEdit;
}

void FocusInventorySearchEdit(const char* reason)
{
    if (!g_itemSearchEdit)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    if (!inputManager)
    {
        return;
    }

    inputManager->setKeyFocusWidget(g_itemSearchEdit);

    if (ShouldLogDebug())
    {
        std::stringstream line;
        line << "inventory search focused";
        if (reason)
        {
            line << " reason=\"" << reason << "\"";
        }
        LogDebugLine(line.str());
    }
}

bool IsInventorySearchFocusHotkeyDown()
{
    if (!key || !key->keyboard)
    {
        return false;
    }

    if (g_panelHidden || g_panelCollapsed || !g_panel || !g_itemSearchEdit)
    {
        return false;
    }

    if (g_activePanelTab != PanelTab_Inventory)
    {
        return false;
    }

    if (IsInventorySearchEditFocused())
    {
        return false;
    }

    return key->ctrl && key->keyboard->isKeyDown(OIS::KC_F);
}

void TickInventorySearchFocusHotkey()
{
    const bool hotkeyDown = IsInventorySearchFocusHotkeyDown();
    if (hotkeyDown && !g_inventorySearchCtrlFPrevDown)
    {
        EnsureInventoryFoodItemOptionsLoaded();
        RefreshInventoryFoodItemDropdown();
        FocusInventorySearchEdit("ctrl_f_hotkey");
    }

    g_inventorySearchCtrlFPrevDown = hotkeyDown;
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

void RefreshSavedLocationActionButtons(PlayerInterface* player)
{
    SavedLocation location;
    const bool hasSelectedLocation = TryGetSelectedSavedLocation(0, &location);
    const bool hasSelectedCharacters = GetSelectedCharacterCount(player) > 0;

    if (g_savedLocationTeleportButton)
    {
        g_savedLocationTeleportButton->setCaption("Teleport");
        g_savedLocationTeleportButton->setEnabled(hasSelectedLocation && hasSelectedCharacters);
    }

    if (g_savedLocationPinButton)
    {
        g_savedLocationPinButton->setCaption((hasSelectedLocation && location.pinned) ? "Unpin" : "Pin");
        g_savedLocationPinButton->setEnabled(hasSelectedLocation);
    }

    if (g_savedLocationRenameButton)
    {
        g_savedLocationRenameButton->setCaption(
            (hasSelectedLocation && g_savedLocationRenameId == location.id) ? "Cancel" : "Rename");
        g_savedLocationRenameButton->setEnabled(hasSelectedLocation);
    }

    if (g_savedLocationDeleteButton)
    {
        g_savedLocationDeleteButton->setCaption("Delete");
        g_savedLocationDeleteButton->setEnabled(hasSelectedLocation);
    }
}

void RefreshInventorySpawnButtonState()
{
    if (!g_spawnItemButton)
    {
        return;
    }

    const bool hasTarget =
        g_hasLastTargetSnapshot
        && g_lastTargetSnapshot.hasTarget
        && g_lastTargetSnapshot.target != 0;
    GameData* itemData = 0;
    const bool hasSelectedItem = TryResolveSelectedInventoryFoodItem(&itemData, 0) && itemData != 0;
    g_spawnItemButton->setEnabled(hasTarget && hasSelectedItem);
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

void LogTeleportInvestigation(
    const char* actionId,
    const std::string& locationName,
    const Ogre::Vector3& requestedDestination,
    const Ogre::Vector3& resolvedDestination,
    bool validSpawnFound)
{
    if (!g_developerMode)
    {
        return;
    }

    const bool destinationAdjusted =
        requestedDestination.x != resolvedDestination.x
        || requestedDestination.y != resolvedDestination.y
        || requestedDestination.z != resolvedDestination.z;

    std::stringstream line;
    line << "[investigate][teleport] action=\"" << actionId << "\""
         << " location_name=\"" << SanitizeLogValue(locationName) << "\""
         << " requested_x=" << requestedDestination.x
         << " requested_y=" << requestedDestination.y
         << " requested_z=" << requestedDestination.z
         << " resolved_x=" << resolvedDestination.x
         << " resolved_y=" << resolvedDestination.y
         << " resolved_z=" << resolvedDestination.z
         << " valid_spawn_found=" << (validSpawnFound ? "true" : "false")
         << " destination_adjusted=" << (destinationAdjusted ? "true" : "false")
         << " delta_x=" << (resolvedDestination.x - requestedDestination.x)
         << " delta_y=" << (resolvedDestination.y - requestedDestination.y)
         << " delta_z=" << (resolvedDestination.z - requestedDestination.z);
    LogInfoLine(line.str());
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

bool TryAddMoneyToTargetPlatoon(Character* target, int amount, int* beforeMoneyOut, int* afterMoneyOut)
{
    if (beforeMoneyOut)
    {
        *beforeMoneyOut = 0;
    }
    if (afterMoneyOut)
    {
        *afterMoneyOut = 0;
    }

    if (!target || amount <= 0)
    {
        return false;
    }

    __try
    {
        const int beforeMoney = target->getMoney();
        Inventory* inventory = target->getInventory();
        int beforeInventoryMoney = beforeMoney;
        if (inventory)
        {
            beforeInventoryMoney = inventory->getMoney();
        }

        target->takeMoney(-amount);
        if (inventory)
        {
            inventory->refreshGui();
        }

        int afterMoney = target->getMoney();
        if (afterMoney - beforeMoney != amount && inventory)
        {
            inventory->takeMoney(-amount);
            inventory->refreshGui();
            afterMoney = target->getMoney();

            const int afterInventoryMoney = inventory->getMoney();
            if (afterMoney - beforeMoney != amount && afterInventoryMoney - beforeInventoryMoney == amount)
            {
                afterMoney = afterInventoryMoney;
            }
        }

        if (beforeMoneyOut)
        {
            *beforeMoneyOut = beforeMoney;
        }
        if (afterMoneyOut)
        {
            *afterMoneyOut = afterMoney;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TrySpawnItemInTargetInventory(
    Character* target,
    GameData* itemData,
    int quantity,
    int* beforeCountOut,
    int* afterCountOut,
    bool* addAcceptedOut)
{
    if (beforeCountOut)
    {
        *beforeCountOut = 0;
    }
    if (afterCountOut)
    {
        *afterCountOut = 0;
    }
    if (addAcceptedOut)
    {
        *addAcceptedOut = false;
    }

    if (!target || !itemData || quantity <= 0 || !ou || !ou->theFactory)
    {
        return false;
    }

    __try
    {
        Inventory* inventory = target->getInventory();
        if (!inventory)
        {
            return false;
        }

        const int beforeCount = inventory->countItems(itemData);
        Item* item = ou->theFactory->createItem(itemData, hand(), 0, 0, 0, 0);
        if (!item)
        {
            return false;
        }

        const bool addAccepted = inventory->addItem(item, quantity, false, true);
        const int afterCount = inventory->countItems(itemData);

        if (beforeCountOut)
        {
            *beforeCountOut = beforeCount;
        }
        if (afterCountOut)
        {
            *afterCountOut = afterCount;
        }
        if (addAcceptedOut)
        {
            *addAcceptedOut = addAccepted;
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

bool TryTeleportSelectedCharactersToCamera(
    PlayerInterface* player,
    const char* actionId,
    const Ogre::Vector3& destinationCenter,
    bool useSpawnValidation,
    int* selectedCountOut,
    int* teleportedCountOut,
    Ogre::Vector3* requestedDestinationOut,
    Ogre::Vector3* resolvedDestinationOut,
    bool* validSpawnFoundOut)
{
    if (selectedCountOut)
    {
        *selectedCountOut = 0;
    }
    if (teleportedCountOut)
    {
        *teleportedCountOut = 0;
    }
    if (requestedDestinationOut)
    {
        *requestedDestinationOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }
    if (resolvedDestinationOut)
    {
        *resolvedDestinationOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }
    if (validSpawnFoundOut)
    {
        *validSpawnFoundOut = false;
    }

    if (!player || (useSpawnValidation && !ou))
    {
        return false;
    }

    __try
    {
        const ogre_unordered_set<hand>::type& selectedCharacters = player->selectedCharacters;
        int selectedCharacterCount = 0;

        ogre_unordered_set<hand>::type::const_iterator countIt = selectedCharacters.begin();
        for (; countIt != selectedCharacters.end(); ++countIt)
        {
            Character* character = countIt->getCharacter();
            if (character)
            {
                ++selectedCharacterCount;
            }
        }

        const bool useSelectedCharacterFallback =
            selectedCharacterCount <= 0 && player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter() != 0;
        if (useSelectedCharacterFallback)
        {
            selectedCharacterCount = 1;
        }

        if (selectedCountOut)
        {
            *selectedCountOut = selectedCharacterCount;
        }

        if (requestedDestinationOut)
        {
            *requestedDestinationOut = destinationCenter;
        }

        Ogre::Vector3 resolvedDestination = destinationCenter;
        bool validSpawnFound = false;
        if (!useSpawnValidation)
        {
            if (ou)
            {
                ou->fixNaNPosition(resolvedDestination);
            }
        }
        else if (ou)
        {
            Ogre::Vector3 validatedDestination = resolvedDestination;
            if (ou->findValidSpawnPos(validatedDestination, resolvedDestination))
            {
                resolvedDestination = validatedDestination;
                validSpawnFound = true;
            }
        }
        if (resolvedDestinationOut)
        {
            *resolvedDestinationOut = resolvedDestination;
        }
        if (validSpawnFoundOut)
        {
            *validSpawnFoundOut = validSpawnFound;
        }

        int teleportedCount = 0;

        ogre_unordered_set<hand>::type::const_iterator teleportIt = selectedCharacters.begin();
        for (; teleportIt != selectedCharacters.end(); ++teleportIt)
        {
            Character* character = teleportIt->getCharacter();
            if (!character)
            {
                continue;
            }

            Ogre::Vector3 referencePosition(0.0f, 0.0f, 0.0f);
            if (!TryGetCharacterTeleportReferencePosition(
                    character,
                    useSpawnValidation,
                    &referencePosition,
                    0))
            {
                referencePosition = character->getPosition();
            }

            CharacterPositionSnapshot beforeSnapshot;
            const bool capturedBeforeSnapshot =
                !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &beforeSnapshot);

            const bool useAbsoluteTeleportInput = !useSpawnValidation;
            const Ogre::Vector3 moveBy = resolvedDestination - referencePosition;
            const Ogre::Vector3 teleportInput =
                useAbsoluteTeleportInput ? resolvedDestination : moveBy;
            character->teleport(teleportInput, character->getOrientation());
            character->setRagdollNavmeshSafePos();

            CharacterPositionSnapshot afterSnapshot;
            const bool capturedAfterSnapshot =
                !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &afterSnapshot);
            if (!useSpawnValidation
                && capturedBeforeSnapshot
                && capturedAfterSnapshot
                && ShouldRetryExactTeleportWithMovement(beforeSnapshot, afterSnapshot))
            {
                DownedTeleportState downedState;
                const bool observedDowned = TryGetDownedTeleportState(character, &downedState) && downedState.active;
                CharacterPositionSnapshot fallbackSnapshot;
                const bool useDownedRagdollSync = observedDowned
                    && DoesSnapshotRawPositionMatchDestination(afterSnapshot, resolvedDestination);
                LogDownedTeleportFallbackSelectionInvestigation(
                    actionId,
                    character,
                    resolvedDestination,
                    observedDowned,
                    useDownedRagdollSync,
                    beforeSnapshot,
                    afterSnapshot,
                    downedState);
                const bool fallbackApplied = observedDowned
                    ? (useDownedRagdollSync
                        ? TryRetryExactTeleportWithDelayedDownedRestore(character, resolvedDestination, downedState, &fallbackSnapshot)
                        : TryRetryExactTeleportWithRelocation(character, moveBy, resolvedDestination, &fallbackSnapshot))
                    : TryRetryExactTeleportWithMovement(character, resolvedDestination, &fallbackSnapshot);
                if (fallbackApplied && useDownedRagdollSync)
                {
                    SchedulePendingDownedTeleportRestore(actionId, character, downedState, resolvedDestination);
                }
            }
            ++teleportedCount;
        }

        if (useSelectedCharacterFallback)
        {
            Character* character = player->selectedCharacter.getCharacter();
            if (character)
            {
                Ogre::Vector3 referencePosition(0.0f, 0.0f, 0.0f);
                if (!TryGetCharacterTeleportReferencePosition(
                        character,
                        useSpawnValidation,
                        &referencePosition,
                        0))
                {
                    referencePosition = character->getPosition();
                }

                CharacterPositionSnapshot beforeSnapshot;
                const bool capturedBeforeSnapshot =
                    !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &beforeSnapshot);

                const bool useAbsoluteTeleportInput = !useSpawnValidation;
                const Ogre::Vector3 moveBy = resolvedDestination - referencePosition;
                const Ogre::Vector3 teleportInput =
                    useAbsoluteTeleportInput ? resolvedDestination : moveBy;
                character->teleport(teleportInput, character->getOrientation());
                character->setRagdollNavmeshSafePos();

                CharacterPositionSnapshot afterSnapshot;
                const bool capturedAfterSnapshot =
                    !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &afterSnapshot);
                if (!useSpawnValidation
                    && capturedBeforeSnapshot
                    && capturedAfterSnapshot
                    && ShouldRetryExactTeleportWithMovement(beforeSnapshot, afterSnapshot))
                {
                    DownedTeleportState downedState;
                    const bool observedDowned = TryGetDownedTeleportState(character, &downedState) && downedState.active;
                    CharacterPositionSnapshot fallbackSnapshot;
                    const bool useDownedRagdollSync = observedDowned
                        && DoesSnapshotRawPositionMatchDestination(afterSnapshot, resolvedDestination);
                    LogDownedTeleportFallbackSelectionInvestigation(
                        actionId,
                        character,
                        resolvedDestination,
                        observedDowned,
                        useDownedRagdollSync,
                        beforeSnapshot,
                        afterSnapshot,
                        downedState);
                    const bool fallbackApplied = observedDowned
                        ? (useDownedRagdollSync
                            ? TryRetryExactTeleportWithDelayedDownedRestore(character, resolvedDestination, downedState, &fallbackSnapshot)
                            : TryRetryExactTeleportWithRelocation(character, moveBy, resolvedDestination, &fallbackSnapshot))
                        : TryRetryExactTeleportWithMovement(character, resolvedDestination, &fallbackSnapshot);
                    if (fallbackApplied && useDownedRagdollSync)
                    {
                        SchedulePendingDownedTeleportRestore(actionId, character, downedState, resolvedDestination);
                    }
                }
                ++teleportedCount;
            }
        }

        if (teleportedCountOut)
        {
            *teleportedCountOut = teleportedCount;
        }

        if (teleportedCount > 0)
        {
            StopTeleportedSelectionMovement(player);
            FocusCameraOnTeleportedSelection(player, resolvedDestination);
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

void OnAddMoneyButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "add_money";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"add_money\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (!g_moneyAmountEdit)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"add_money\" success=false reason=\"missing_input_widget\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Add Money failed - amount input unavailable");
        return;
    }

    const std::string amountText = TrimAscii(g_moneyAmountEdit->getOnlyText().asUTF8());
    int amount = 0;
    if (!TryParsePositiveInt(amountText, &amount))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"add_money\" success=false reason=\"invalid_amount\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\""
               << " amount_text=\"" << SanitizeLogValue(amountText) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Add Money failed - enter a positive amount");
        return;
    }

    g_moneyAmountEdit->setOnlyText(amountText);

    const std::string targetName = g_lastTargetSnapshot.name;
    int beforeMoney = 0;
    int afterMoney = 0;
    if (!TryAddMoneyToTargetPlatoon(g_lastTargetSnapshot.target, amount, &beforeMoney, &afterMoney))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"add_money\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\""
               << " amount=" << amount;
        LogInfoLine(result.str());
        SetStatusMessage("Add Money failed - target money path unavailable");
        return;
    }

    const long long observedDelta = static_cast<long long>(afterMoney) - static_cast<long long>(beforeMoney);
    const bool success = observedDelta == static_cast<long long>(amount);

    std::stringstream result;
    result << "event=testkit_action_result action=\"add_money\" success="
           << (success ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " amount=" << amount
           << " before_money=" << beforeMoney
           << " after_money=" << afterMoney
           << " observed_delta=" << observedDelta;
    if (!success)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (success)
    {
        std::stringstream status;
        status << "Added " << amount << " Cats to " << targetName;
        SetStatusMessage(status.str());
        return;
    }

    std::stringstream status;
    status << "Add Money requested for " << targetName << " - no money change readback yet";
    SetStatusMessage(status.str());
}

void OnAddMoneyButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnAddMoneyButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnInventoryItemSearchTextChanged(MyGUI::EditBox*)
{
    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
}

void OnInventoryItemSearchFocusChanged(MyGUI::Widget*, MyGUI::Widget*)
{
    ResetPendingInventorySearchShortcut();
}

void OnInventoryItemSearchKeyPressed(MyGUI::Widget* sender, MyGUI::KeyCode keyCode, MyGUI::Char character)
{
    (void)character;

    if (!sender || !IsInterestingInventorySearchMyGuiKey(keyCode))
    {
        return;
    }

    ScheduleInventorySearchMyGuiShortcut(sender->castType<MyGUI::EditBox>(false), keyCode);
}

void OnInventoryItemSearchKeyReleased(MyGUI::Widget* sender, MyGUI::KeyCode keyCode)
{
    if (!sender)
    {
        ResetPendingInventorySearchShortcut();
        ResetInventorySearchEditSnapshot();
        return;
    }

    MyGUI::EditBox* searchEdit = sender->castType<MyGUI::EditBox>(false);
    if (!searchEdit)
    {
        ResetPendingInventorySearchShortcut();
        return;
    }

    if (IsInterestingInventorySearchMyGuiKey(keyCode))
    {
        ApplyPendingInventorySearchEditShortcut(searchEdit, keyCode);
    }

    RememberInventorySearchEditSnapshot(searchEdit);
}

void OnInventoryCategoryChanged(MyGUI::ComboBox*, size_t)
{
    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
}

void OnInventorySearchResultsSelectionChanged(MyGUI::ListBox*, size_t)
{
    RefreshInventorySpawnButtonState();
}

void OnSpawnItemButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "spawn_inventory_item";
    LogActionRequested(actionId);

    if (!g_hasLastTargetSnapshot || !g_lastTargetSnapshot.hasTarget || !g_lastTargetSnapshot.target)
    {
        LogInfoLine("event=testkit_action_result action=\"spawn_inventory_item\" success=false reason=\"no_target\"");
        SetStatusMessage("No target - select a character");
        return;
    }

    if (!g_itemQuantityEdit)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_inventory_item\" success=false reason=\"missing_quantity_widget\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Spawn Item failed - quantity input unavailable");
        return;
    }

    EnsureInventoryFoodItemOptionsLoaded();

    GameData* itemData = 0;
    std::string itemLabel;
    if (!TryResolveSelectedInventoryFoodItem(&itemData, &itemLabel))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_inventory_item\" success=false reason=\"no_item_selected\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Spawn Item failed - select an item");
        return;
    }

    const std::string quantityText = TrimAscii(g_itemQuantityEdit->getOnlyText().asUTF8());
    int quantity = 0;
    if (!TryParsePositiveInt(quantityText, &quantity))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_inventory_item\" success=false reason=\"invalid_quantity\""
               << " target_name=\"" << SanitizeLogValue(g_lastTargetSnapshot.name) << "\""
               << " quantity_text=\"" << SanitizeLogValue(quantityText) << "\""
               << " item_name=\"" << SanitizeLogValue(itemLabel) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Spawn Item failed - enter a positive quantity");
        return;
    }

    g_itemQuantityEdit->setOnlyText(quantityText);

    const std::string targetName = g_lastTargetSnapshot.name;
    bool addAccepted = false;
    int beforeCount = 0;
    int afterCount = 0;
    if (!TrySpawnItemInTargetInventory(
            g_lastTargetSnapshot.target,
            itemData,
            quantity,
            &beforeCount,
            &afterCount,
            &addAccepted))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"spawn_inventory_item\" success=false reason=\"apply_failed\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\""
               << " item_name=\"" << SanitizeLogValue(itemLabel) << "\""
               << " quantity=" << quantity;
        LogInfoLine(result.str());
        SetStatusMessage("Spawn Item failed - target inventory unavailable");
        return;
    }

    const int observedDelta = afterCount - beforeCount;
    const bool success = addAccepted && observedDelta == quantity;

    std::stringstream result;
    result << "event=testkit_action_result action=\"spawn_inventory_item\" success="
           << (success ? "true" : "false")
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " item_name=\"" << SanitizeLogValue(itemLabel) << "\""
           << " quantity=" << quantity
           << " add_accepted=" << (addAccepted ? "true" : "false")
           << " before_count=" << beforeCount
           << " after_count=" << afterCount
           << " observed_delta=" << observedDelta;
    if (!addAccepted)
    {
        result << " reason=\"inventory_rejected_add\"";
    }
    else if (!success)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (success)
    {
        std::stringstream status;
        status << "Spawned " << quantity << " " << itemLabel << " for " << targetName;
        SetStatusMessage(status.str());
        return;
    }

    if (!addAccepted)
    {
        SetStatusMessage("Spawn Item failed - inventory rejected the item");
        return;
    }

    SetStatusMessage("Spawn Item requested for " + targetName + " - no full inventory readback yet");
}

void OnSpawnItemButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSpawnItemButtonClicked(0);

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

void OnSaveLocationNameTextChanged(MyGUI::EditBox*)
{
    UpdateSelectionActionButtons(g_lastPlayerInterface);
}

void OnSaveLocationNameAccepted(MyGUI::EditBox* sender)
{
    if (!sender)
    {
        return;
    }

    if (!g_saveSelectedLocationButton || !g_saveSelectedLocationButton->getEnabled())
    {
        return;
    }

    if (TrimAscii(sender->getOnlyText().asUTF8()).empty())
    {
        return;
    }

    OnSaveSelectedLocationButtonClicked(0);
}

void OnSavedLocationsCollapseButtonClicked(MyGUI::Widget*)
{
    g_savedLocationsCollapsed = !g_savedLocationsCollapsed;
    UpdateSavedLocationsCollapseButtonCaption();
    ApplyPanelLayout();
}

void OnSavedLocationsCollapseButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationsCollapseButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationSearchTextChanged(MyGUI::EditBox* sender)
{
    g_savedLocationSearchText = sender ? sender->getOnlyText().asUTF8() : "";
    RefreshSavedLocationsListWidget();
}

void OnSavedLocationsListSelectionChanged(MyGUI::ListBox*, size_t index)
{
    if (index >= g_filteredSavedLocationIndexes.size())
    {
        g_selectedSavedLocationId.clear();
        RefreshSavedLocationActionButtons(g_lastPlayerInterface);
        return;
    }

    const size_t locationIndex = g_filteredSavedLocationIndexes[index];
    if (locationIndex >= g_savedLocations.size())
    {
        g_selectedSavedLocationId.clear();
        RefreshSavedLocationActionButtons(g_lastPlayerInterface);
        return;
    }

    g_selectedSavedLocationId = g_savedLocations[locationIndex].id;
    RefreshSavedLocationActionButtons(g_lastPlayerInterface);
}

void OnSaveSelectedLocationButtonClicked(MyGUI::Widget*)
{
    const bool renameMode = !g_savedLocationRenameId.empty();
    const char* actionId = renameMode ? "rename_saved_location" : "save_selected_location";
    LogActionRequested(actionId);

    if (!g_saveLocationNameEdit)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"missing_input_widget\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(renameMode ? "Rename Location" : "Save Selected Location") + " failed - name input unavailable");
        return;
    }

    const std::string locationName = TrimAscii(g_saveLocationNameEdit->getOnlyText().asUTF8());
    if (locationName.empty())
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"empty_name\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(renameMode ? "Rename Location" : "Save Selected Location") + " failed - enter a location name");
        return;
    }

    g_saveLocationNameEdit->setOnlyText(locationName);

    if (renameMode)
    {
        const size_t locationIndex = FindSavedLocationIndexById(g_savedLocations, g_savedLocationRenameId);
        if (locationIndex >= g_savedLocations.size())
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"rename_saved_location\" success=false reason=\"missing_location\""
                   << " location_id=\"" << SanitizeLogValue(g_savedLocationRenameId) << "\"";
            LogInfoLine(result.str());
            ClearSavedLocationRenameState(true);
            RefreshSavedLocationsListWidget();
            UpdateSelectionActionButtons(g_lastPlayerInterface);
            SetStatusMessage("Rename Location failed - saved location no longer exists");
            return;
        }

        if (DoesSavedLocationNameExistExcludingId(g_savedLocations, locationName, g_savedLocationRenameId))
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"rename_saved_location\" success=false reason=\"duplicate_name\""
                   << " location_id=\"" << SanitizeLogValue(g_savedLocationRenameId) << "\""
                   << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
            LogInfoLine(result.str());
            SetStatusMessage("Rename Location failed - name already exists");
            return;
        }

        std::vector<SavedLocation> updatedLocations = g_savedLocations;
        const std::string previousName = updatedLocations[locationIndex].name;
        updatedLocations[locationIndex].name = locationName;
        SortSavedLocationsForDisplay(&updatedLocations);

        std::string persistError;
        if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"rename_saved_location\" success=false reason=\""
                   << persistError << "\""
                   << " location_id=\"" << SanitizeLogValue(g_savedLocationRenameId) << "\""
                   << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
            LogInfoLine(result.str());
            SetStatusMessage("Rename Location failed - could not persist config");
            return;
        }

        const std::string renamedLocationId = g_savedLocationRenameId;
        g_savedLocations.swap(updatedLocations);
        ClearSavedLocationRenameState(true);
        g_selectedSavedLocationId = renamedLocationId;
        g_savedLocationSearchText.clear();
        if (g_savedLocationSearchEdit)
        {
            g_savedLocationSearchEdit->setOnlyText("");
        }
        RefreshSavedLocationsListWidget();
        UpdateSelectionActionButtons(g_lastPlayerInterface);

        std::stringstream result;
        result << "event=testkit_action_result action=\"rename_saved_location\" success=true"
               << " location_id=\"" << SanitizeLogValue(renamedLocationId) << "\""
               << " old_name=\"" << SanitizeLogValue(previousName) << "\""
               << " new_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());

        SetStatusMessage(std::string("Renamed location ") + previousName + " to " + locationName);
        return;
    }

    if (DoesSavedLocationNameExist(g_savedLocations, locationName))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\"duplicate_name\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - name already exists");
        return;
    }

    if (!g_lastPlayerInterface)
    {
        LogInfoLine("event=testkit_action_result action=\"save_selected_location\" success=false reason=\"no_player_interface\"");
        SetStatusMessage("Save Selected Location failed - player interface unavailable");
        return;
    }

    Character* selectedCharacter = TryGetPrimarySelectedCharacter(g_lastPlayerInterface);
    if (!selectedCharacter)
    {
        LogInfoLine("event=testkit_action_result action=\"save_selected_location\" success=false reason=\"no_selected_character\"");
        SetStatusMessage("No selected character to save location from");
        return;
    }

    CharacterPositionSnapshot positionSnapshot;
    if (!TryGetCharacterPositionSnapshot(selectedCharacter, &positionSnapshot))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\"position_read_failed\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - selected character position unavailable");
        return;
    }

    Ogre::Vector3 selectedPosition(0.0f, 0.0f, 0.0f);
    const char* selectedSource = "unavailable";
    if (!TryGetCharacterTeleportReferencePosition(
            selectedCharacter,
            false,
            &selectedPosition,
            &selectedSource))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\"position_read_failed\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - selected character position unavailable");
        return;
    }
    const std::string targetName = SafeCharacterName(selectedCharacter);
    LogSavedLocationPositionInvestigation(
        "save_selected_location",
        locationName,
        targetName,
        positionSnapshot,
        selectedPosition,
        selectedSource);

    std::vector<SavedLocation> updatedLocations = g_savedLocations;
    SavedLocation location;
    location.id = BuildNextSavedLocationId(updatedLocations);
    location.name = locationName;
    location.position = selectedPosition;
    updatedLocations.push_back(location);
    SortSavedLocationsForDisplay(&updatedLocations);

    std::string persistError;
    if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\""
               << persistError << "\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - could not persist config");
        return;
    }

    g_savedLocations.swap(updatedLocations);
    ClearSavedLocationRenameState(true);
    g_selectedSavedLocationId = location.id;
    g_savedLocationSearchText.clear();
    if (g_savedLocationSearchEdit)
    {
        g_savedLocationSearchEdit->setOnlyText("");
    }
    RefreshSavedLocationsListWidget();
    UpdateSelectionActionButtons(g_lastPlayerInterface);

    std::stringstream result;
    result << "event=testkit_action_result action=\"save_selected_location\" success=true"
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\""
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " x=" << location.position.x
           << " y=" << location.position.y
           << " z=" << location.position.z
           << " saved_count=" << g_savedLocations.size();
    LogInfoLine(result.str());

    SetStatusMessage(std::string("Saved location ") + location.name + " from " + targetName);
}

void OnSaveSelectedLocationButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSaveSelectedLocationButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationTeleportButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "teleport_selected_to_saved_location";
    LogActionRequested(actionId);

    SavedLocation location;
    size_t locationIndex = 0u;
    if (!TryGetSelectedSavedLocation(&locationIndex, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Teleport failed - no saved location selected");
        return;
    }

    if (!g_lastPlayerInterface)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"no_player_interface\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string("Teleport to ") + location.name + " failed - player interface unavailable");
        return;
    }

    int selectedCount = 0;
    int teleportedCount = 0;
    Ogre::Vector3 requestedDestination(0.0f, 0.0f, 0.0f);
    Ogre::Vector3 resolvedDestination(0.0f, 0.0f, 0.0f);
    bool validSpawnFound = false;
    if (!TryTeleportSelectedCharactersToCamera(
            g_lastPlayerInterface,
            "teleport_selected_to_saved_location",
            location.position,
            false,
            &selectedCount,
            &teleportedCount,
            &requestedDestination,
            &resolvedDestination,
            &validSpawnFound))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"apply_failed\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\""
               << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string("Teleport to ") + location.name + " failed - apply path unavailable");
        return;
    }

    if (selectedCount <= 0)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"no_selection\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\""
               << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string("No selected characters to teleport to ") + location.name);
        return;
    }

    const bool destinationAdjusted =
        requestedDestination.x != resolvedDestination.x
        || requestedDestination.y != resolvedDestination.y
        || requestedDestination.z != resolvedDestination.z;
    LogTeleportInvestigation(
        "teleport_selected_to_saved_location",
        location.name,
        requestedDestination,
        resolvedDestination,
        validSpawnFound);

    if (teleportedCount > 0)
    {
        std::vector<SavedLocation> updatedLocations = g_savedLocations;
        if (locationIndex < updatedLocations.size())
        {
            updatedLocations[locationIndex].lastUsedUtc = GetCurrentUtcTimestamp();
            SortSavedLocationsForDisplay(&updatedLocations);

            std::string persistError;
            if (TryPersistSavedLocationsConfig(updatedLocations, &persistError))
            {
                g_savedLocations.swap(updatedLocations);
                RefreshSavedLocationsListWidget();
                UpdateSelectionActionButtons(g_lastPlayerInterface);
            }
            else
            {
                std::stringstream persistLine;
                persistLine << "event=testkit_saved_location_recent_persist_failed location_id=\""
                            << SanitizeLogValue(location.id)
                            << "\" reason=\"" << persistError << "\"";
                LogWarnLine(persistLine.str());
            }
        }
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success="
           << (teleportedCount > 0 ? "true" : "false")
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\""
           << " selected_count=" << selectedCount
           << " teleported_count=" << teleportedCount
           << " requested_x=" << requestedDestination.x
           << " requested_y=" << requestedDestination.y
           << " requested_z=" << requestedDestination.z
           << " destination_x=" << resolvedDestination.x
           << " destination_y=" << resolvedDestination.y
           << " destination_z=" << resolvedDestination.z
           << " valid_spawn_found=" << (validSpawnFound ? "true" : "false")
           << " destination_adjusted=" << (destinationAdjusted ? "true" : "false");
    if (teleportedCount <= 0)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (teleportedCount == selectedCount)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " selected character(s) to " << location.name;
        SetStatusMessage(status.str());
    }
    else if (teleportedCount > 0)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " of " << selectedCount << " selected characters to "
               << location.name;
        SetStatusMessage(status.str());
    }
    else
    {
        SetStatusMessage(std::string("Teleport to ") + location.name + " requested - no selected characters moved");
    }

    UpdateTargetInspection(g_lastPlayerInterface);
}

void OnSavedLocationTeleportButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationTeleportButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationPinButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "toggle_saved_location_pin";
    LogActionRequested(actionId);

    SavedLocation location;
    size_t locationIndex = 0u;
    if (!TryGetSelectedSavedLocation(&locationIndex, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"toggle_saved_location_pin\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Pin failed - no saved location selected");
        return;
    }

    std::vector<SavedLocation> updatedLocations = g_savedLocations;
    const bool pinned = !updatedLocations[locationIndex].pinned;
    updatedLocations[locationIndex].pinned = pinned;
    SortSavedLocationsForDisplay(&updatedLocations);

    std::string persistError;
    if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"toggle_saved_location_pin\" success=false reason=\""
               << persistError << "\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(location.pinned ? "Unpin" : "Pin") + " failed - could not persist config");
        return;
    }

    g_savedLocations.swap(updatedLocations);
    RefreshSavedLocationsListWidget();
    UpdateSelectionActionButtons(g_lastPlayerInterface);

    std::stringstream result;
    result << "event=testkit_action_result action=\"toggle_saved_location_pin\" success=true"
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\""
           << " pinned=" << (pinned ? "true" : "false");
    LogInfoLine(result.str());

    SetStatusMessage(std::string(pinned ? "Pinned location " : "Unpinned location ") + location.name);
}

void OnSavedLocationPinButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationPinButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationRenameButtonClicked(MyGUI::Widget*)
{
    SavedLocation location;
    if (!TryGetSelectedSavedLocation(0, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"rename_saved_location\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Rename Location failed - no saved location selected");
        return;
    }

    if (g_savedLocationRenameId == location.id)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"rename_saved_location\" success=true mode=\"cancel\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        ClearSavedLocationRenameState(true);
        RefreshSavedLocationsListWidget();
        UpdateSelectionActionButtons(g_lastPlayerInterface);
        SetStatusMessage(std::string("Rename cancelled for ") + location.name);
        return;
    }

    BeginSavedLocationRename(location);
    RefreshSavedLocationsListWidget();
    UpdateSelectionActionButtons(g_lastPlayerInterface);

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    if (inputManager && g_saveLocationNameEdit)
    {
        inputManager->setKeyFocusWidget(g_saveLocationNameEdit);
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"rename_saved_location\" success=true mode=\"begin\""
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
    LogInfoLine(result.str());
    SetStatusMessage(std::string("Rename location ") + location.name + " and click Save Rename");
}

void OnSavedLocationRenameButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationRenameButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationDeleteButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "delete_saved_location";
    LogActionRequested(actionId);

    SavedLocation location;
    size_t locationIndex = 0u;
    if (!TryGetSelectedSavedLocation(&locationIndex, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"delete_saved_location\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Delete Location failed - no saved location selected");
        return;
    }

    std::vector<SavedLocation> updatedLocations = g_savedLocations;
    updatedLocations.erase(updatedLocations.begin() + locationIndex);

    std::string persistError;
    if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"delete_saved_location\" success=false reason=\""
               << persistError << "\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Delete Location failed - could not persist config");
        return;
    }

    const bool clearedRenameState = g_savedLocationRenameId == location.id;
    g_savedLocations.swap(updatedLocations);
    if (clearedRenameState)
    {
        ClearSavedLocationRenameState(true);
    }
    RefreshSavedLocationsListWidget();
    UpdateSelectionActionButtons(g_lastPlayerInterface);

    std::stringstream result;
    result << "event=testkit_action_result action=\"delete_saved_location\" success=true"
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
    LogInfoLine(result.str());

    SetStatusMessage(std::string("Deleted location ") + location.name);
}

void OnSavedLocationDeleteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationDeleteButtonClicked(0);

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
    g_inventorySearchCtrlFPrevDown = false;
    g_lastStatusMessage = "Ready";
    ResetInventoryFoodItemOptions();
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
