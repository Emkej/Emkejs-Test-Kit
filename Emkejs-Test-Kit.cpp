#include <Debug.h>

#include <emc/mod_hub_client.h>

#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/Damages.h>
#include <kenshi/Dialogue.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Kenshi.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/SaveManager.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_ComboBox.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>
#include <ois/OISKeyboard.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cctype>
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
const char* kPluginName = "Emkejs-Test-Kit";
const char* kConfigFileName = "mod-config.json";
const char* kDefaultTogglePanelKey = "D";
const int kPanelLeft = 18;
const int kPanelTop = 140;
const int kPanelWidth = 360;
const int kPanelExpandedHeight = 708;
const int kPanelCollapsedHeight = 42;
const int kPanelViewportPadding = 16;
const int kPanelDragThreshold = 3;
const DWORD kDangerArmTimeoutMs = 3000;
const float kForceUnconsciousDurationSeconds = 30.0f;
const float kForceDyingBloodOffset = 8.0f;
const float kForceDyingAliveBloodMargin = 1.0f;
const float kProbablyDyingBloodMax = 50.0f;
const float kLimbDamageFraction = 0.35f;
const float kMinimumLimbDamageAmount = 5.0f;
const char* kTeleportDestinationLabel = "Test Spot";
const Ogre::Vector3 kTeleportDestinationCenter(-56164.4f, 1605.11f, 20653.6f);
const float kFloatChangeEpsilon = 0.001f;
// MyGUI expects the popup length as a visual height, not an item count.
const int kInventoryItemDropdownMaxListLength = 224;
const char* kModHubNamespaceId = "emkej.qol";
const char* kModHubNamespaceDisplayName = "Emkej QoL";
const char* kModHubModId = "emkejs_test_kit";
const char* kModHubModDisplayName = "Emkejs Test Kit";
const char* kModHubTogglePanelKeyLabel = "Debug Panel Key";
const char* kModHubTogglePanelKeyDescription =
    "Primary key for showing or hiding the debug panel. Use the modifier toggles below for Ctrl, Shift, and Alt. Unbind to disable.";
const char* kModHubTogglePanelCtrlLabel = "Require Ctrl";
const char* kModHubTogglePanelCtrlDescription = "Require Ctrl for the debug panel hotkey.";
const char* kModHubTogglePanelShiftLabel = "Require Shift";
const char* kModHubTogglePanelShiftDescription = "Require Shift for the debug panel hotkey.";
const char* kModHubTogglePanelAltLabel = "Require Alt";
const char* kModHubTogglePanelAltDescription = "Require Alt for the debug panel hotkey.";
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

enum LoggingLevel
{
    LoggingLevel_Info = 0,
    LoggingLevel_Debug = 1
};

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
    PanelTab_Teleport = 1,
    PanelTab_Inventory = 2
};

enum InventorySpawnCategory
{
    InventorySpawnCategory_All = 0,
    InventorySpawnCategory_Food = 1,
    InventorySpawnCategory_General = 2,
    InventorySpawnCategory_Armour = 3,
    InventorySpawnCategory_Weapons = 4
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

struct InventorySpawnOption
{
    std::string displayName;
    std::string searchTextUpper;
    GameData* itemData;
};

std::string g_configPath;
bool g_pluginEnabled = true;
LoggingLevel g_loggingLevel = LoggingLevel_Info;
bool g_togglePanelRequireCtrl = true;
bool g_togglePanelRequireShift = true;
bool g_togglePanelRequireAlt = false;
std::string g_togglePanelKey = kDefaultTogglePanelKey;
bool g_hotkeyEnabled = true;
int g_hotkeyVirtualKey = 'D';
std::string g_hotkeyDisplay = "CTRL+SHIFT+D";
bool g_hotkeyPrevDown = false;
emc::ModHubClient g_modHubClient;
bool g_modHubClientConfigured = false;
bool g_confirmDangerousActions = true;
bool g_panelHidden = false;
bool g_panelCollapsed = false;
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
MyGUI::Button* g_headerFrame = 0;
MyGUI::TextBox* g_headerTitleText = 0;
MyGUI::Button* g_collapseButton = 0;
MyGUI::Button* g_bodyFrame = 0;
MyGUI::TextBox* g_targetSectionText = 0;
MyGUI::TextBox* g_targetNameText = 0;
MyGUI::TextBox* g_targetFactionText = 0;
MyGUI::TextBox* g_targetAlignmentText = 0;
MyGUI::TextBox* g_targetMembershipText = 0;
MyGUI::TextBox* g_targetStateText = 0;
MyGUI::TextBox* g_noTargetText = 0;
MyGUI::Button* g_healthTabButton = 0;
MyGUI::Button* g_teleportTabButton = 0;
MyGUI::Button* g_inventoryTabButton = 0;
MyGUI::TextBox* g_statesSectionText = 0;
MyGUI::Button* g_fullRestoreButton = 0;
MyGUI::Button* g_forceUnconsciousButton = 0;
MyGUI::Button* g_forcePlayingDeadButton = 0;
MyGUI::TextBox* g_limbDamageSectionText = 0;
MyGUI::Button* g_damageLeftArmButton = 0;
MyGUI::Button* g_damageRightArmButton = 0;
MyGUI::Button* g_damageLeftLegButton = 0;
MyGUI::Button* g_damageRightLegButton = 0;
MyGUI::TextBox* g_teleportSectionText = 0;
MyGUI::Button* g_teleportSelectedToCameraButton = 0;
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
MyGUI::TextBox* g_itemDropdownLabelText = 0;
MyGUI::ComboBox* g_itemDropdown = 0;
MyGUI::TextBox* g_itemQuantityLabelText = 0;
MyGUI::EditBox* g_itemQuantityEdit = 0;
MyGUI::Button* g_spawnItemButton = 0;
MyGUI::TextBox* g_dangerousSectionText = 0;
MyGUI::Button* g_forceDyingButton = 0;
MyGUI::TextBox* g_statusText = 0;

std::vector<InventorySpawnOption> g_inventoryFoodItemOptions;
std::vector<size_t> g_filteredInventoryFoodItemOptionIndexes;
bool g_inventoryFoodItemOptionsLoaded = false;

void (*PlayerInterface_updateUT_orig)(PlayerInterface*) = 0;
void (*SaveManager_loadByInfo_orig)(SaveManager*, const SaveInfo&, bool) = 0;
void (*SaveManager_loadByName_orig)(SaveManager*, const std::string&) = 0;

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

bool TryPopulateInventoryFoodSelection(size_t optionIndex)
{
    if (optionIndex == MyGUI::ITEM_NONE || optionIndex >= g_inventoryFoodItemOptions.size())
    {
        return false;
    }

    EnsureInventoryFoodItemOptionsLoaded();

    if (g_itemSearchEdit)
    {
        g_itemSearchEdit->setOnlyText(g_inventoryFoodItemOptions[optionIndex].displayName);
    }

    RefreshInventoryFoodItemDropdown();
    if (!g_itemDropdown)
    {
        return false;
    }

    for (size_t filteredIndex = 0; filteredIndex < g_filteredInventoryFoodItemOptionIndexes.size(); ++filteredIndex)
    {
        if (g_filteredInventoryFoodItemOptionIndexes[filteredIndex] != optionIndex)
        {
            continue;
        }

        g_itemDropdown->setIndexSelected(filteredIndex);
        if (g_itemSearchResultsList)
        {
            g_itemSearchResultsList->setIndexSelected(filteredIndex);
            g_itemSearchResultsList->beginToItemSelected();
        }
        return true;
    }

    return false;
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

bool TryResolveModConfigPath(std::string* outPath)
{
    if (!outPath || g_configPath.empty())
    {
        return false;
    }

    *outPath = g_configPath;
    return true;
}

bool TryReadTextFile(const std::string& path, std::string* outContent)
{
    if (!outContent)
    {
        return false;
    }

    std::ifstream input(path.c_str(), std::ios::in | std::ios::binary);
    if (!input)
    {
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof())
    {
        return false;
    }

    *outContent = buffer.str();
    return true;
}

bool TryWriteTextFile(const std::string& path, const std::string& content)
{
    std::ofstream output(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }

    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output.good())
    {
        return false;
    }

    output.close();
    return output.good();
}

std::string EscapeJsonStringValue(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (size_t index = 0; index < value.size(); ++index)
    {
        const char current = value[index];
        switch (current)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(current);
            break;
        }
    }

    return escaped;
}

bool TryParseJsonBoolByKey(const std::string& content, const char* key, bool* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content.find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content.find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content.size()
        && std::isspace(static_cast<unsigned char>(content[valuePos])) != 0)
    {
        ++valuePos;
    }

    if (content.compare(valuePos, 4, "true") == 0)
    {
        *outValue = true;
        return true;
    }

    if (content.compare(valuePos, 5, "false") == 0)
    {
        *outValue = false;
        return true;
    }

    return false;
}

bool TryReplaceJsonBoolByKey(std::string* content, const char* key, bool value)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content->find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content->find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content->size()
        && std::isspace(static_cast<unsigned char>((*content)[valuePos])) != 0)
    {
        ++valuePos;
    }

    std::string replacement = value ? "true" : "false";
    if (content->compare(valuePos, 4, "true") == 0)
    {
        content->replace(valuePos, 4, replacement);
        return true;
    }

    if (content->compare(valuePos, 5, "false") == 0)
    {
        content->replace(valuePos, 5, replacement);
        return true;
    }

    return false;
}

bool TryReplaceJsonStringByKey(std::string* content, const char* key, const std::string& value)
{
    if (!content || !key)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content->find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content->find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content->size()
        && std::isspace(static_cast<unsigned char>((*content)[valuePos])) != 0)
    {
        ++valuePos;
    }

    if (valuePos >= content->size() || (*content)[valuePos] != '"')
    {
        return false;
    }

    std::string::size_type endPos = valuePos + 1;
    while (endPos < content->size())
    {
        if ((*content)[endPos] == '"' && (*content)[endPos - 1] != '\\')
        {
            break;
        }

        ++endPos;
    }

    if (endPos >= content->size())
    {
        return false;
    }

    const std::string replacement = std::string("\"") + EscapeJsonStringValue(value) + "\"";
    content->replace(valuePos, endPos - valuePos + 1, replacement);
    return true;
}

bool TryParseJsonStringByKey(const std::string& content, const char* key, std::string* outValue)
{
    if (!key || !outValue)
    {
        return false;
    }

    const std::string needle = std::string("\"") + key + "\"";
    const std::string::size_type keyPos = content.find(needle);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    std::string::size_type valuePos = content.find(':', keyPos + needle.size());
    if (valuePos == std::string::npos)
    {
        return false;
    }

    ++valuePos;
    while (valuePos < content.size()
        && std::isspace(static_cast<unsigned char>(content[valuePos])) != 0)
    {
        ++valuePos;
    }

    if (valuePos >= content.size() || content[valuePos] != '"')
    {
        return false;
    }

    ++valuePos;
    outValue->clear();

    while (valuePos < content.size())
    {
        char current = content[valuePos];
        if (current == '"')
        {
            return true;
        }

        if (current == '\\')
        {
            ++valuePos;
            if (valuePos >= content.size())
            {
                return false;
            }

            current = content[valuePos];
        }

        outValue->push_back(current);
        ++valuePos;
    }

    return false;
}

void CopyModHubErrorMessage(char* err_buf, uint32_t err_buf_size, const char* message)
{
    if (!err_buf || err_buf_size == 0u)
    {
        return;
    }

    err_buf[0] = '\0';
    if (!message)
    {
        return;
    }

    const size_t maxCopyLength = static_cast<size_t>(err_buf_size - 1u);
    std::strncpy(err_buf, message, maxCopyLength);
    err_buf[maxCopyLength] = '\0';
}

bool TryParsePrimaryKeyToken(const std::string& tokenValue, int* virtualKeyOut, std::string* canonicalTokenOut)
{
    if (!virtualKeyOut || !canonicalTokenOut)
    {
        return false;
    }

    const std::string tokenUpper = ToUpperAscii(TrimAscii(tokenValue));
    if (tokenUpper.empty())
    {
        return false;
    }

    if (tokenUpper.size() == 1)
    {
        const char ch = tokenUpper[0];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
        {
            *virtualKeyOut = static_cast<int>(ch);
            canonicalTokenOut->assign(1, ch);
            return true;
        }
    }

    if (tokenUpper == "SPACE")
    {
        *virtualKeyOut = VK_SPACE;
        *canonicalTokenOut = "SPACE";
        return true;
    }

    if (tokenUpper == "TAB")
    {
        *virtualKeyOut = VK_TAB;
        *canonicalTokenOut = "TAB";
        return true;
    }

    if (tokenUpper == "ENTER" || tokenUpper == "RETURN")
    {
        *virtualKeyOut = VK_RETURN;
        *canonicalTokenOut = "ENTER";
        return true;
    }

    if (tokenUpper == "ESC" || tokenUpper == "ESCAPE")
    {
        *virtualKeyOut = VK_ESCAPE;
        *canonicalTokenOut = "ESC";
        return true;
    }

    if (tokenUpper == "BACKSPACE")
    {
        *virtualKeyOut = VK_BACK;
        *canonicalTokenOut = "BACKSPACE";
        return true;
    }

    if (tokenUpper == "DELETE")
    {
        *virtualKeyOut = VK_DELETE;
        *canonicalTokenOut = "DELETE";
        return true;
    }

    if (tokenUpper == "INSERT")
    {
        *virtualKeyOut = VK_INSERT;
        *canonicalTokenOut = "INSERT";
        return true;
    }

    if (tokenUpper == "HOME")
    {
        *virtualKeyOut = VK_HOME;
        *canonicalTokenOut = "HOME";
        return true;
    }

    if (tokenUpper == "END")
    {
        *virtualKeyOut = VK_END;
        *canonicalTokenOut = "END";
        return true;
    }

    if (tokenUpper == "PAGEUP" || tokenUpper == "PGUP")
    {
        *virtualKeyOut = VK_PRIOR;
        *canonicalTokenOut = "PAGEUP";
        return true;
    }

    if (tokenUpper == "PAGEDOWN" || tokenUpper == "PGDN")
    {
        *virtualKeyOut = VK_NEXT;
        *canonicalTokenOut = "PAGEDOWN";
        return true;
    }

    if (tokenUpper == "UP")
    {
        *virtualKeyOut = VK_UP;
        *canonicalTokenOut = "UP";
        return true;
    }

    if (tokenUpper == "DOWN")
    {
        *virtualKeyOut = VK_DOWN;
        *canonicalTokenOut = "DOWN";
        return true;
    }

    if (tokenUpper == "LEFT")
    {
        *virtualKeyOut = VK_LEFT;
        *canonicalTokenOut = "LEFT";
        return true;
    }

    if (tokenUpper == "RIGHT")
    {
        *virtualKeyOut = VK_RIGHT;
        *canonicalTokenOut = "RIGHT";
        return true;
    }

    if (tokenUpper.size() >= 2 && tokenUpper[0] == 'F')
    {
        int functionIndex = 0;
        for (size_t index = 1; index < tokenUpper.size(); ++index)
        {
            const unsigned char ch = static_cast<unsigned char>(tokenUpper[index]);
            if (std::isdigit(ch) == 0)
            {
                return false;
            }

            functionIndex = (functionIndex * 10) + (tokenUpper[index] - '0');
        }

        if (functionIndex >= 1 && functionIndex <= 24)
        {
            *virtualKeyOut = VK_F1 + (functionIndex - 1);

            std::stringstream label;
            label << "F" << functionIndex;
            *canonicalTokenOut = label.str();
            return true;
        }
    }

    return false;
}

bool TryMapTogglePanelTokenToOisKeycode(const std::string& tokenValue, int32_t* outKeycode)
{
    if (!outKeycode)
    {
        return false;
    }

    const std::string tokenUpper = ToUpperAscii(TrimAscii(tokenValue));
    if (tokenUpper == "NONE" || tokenUpper == "UNBOUND")
    {
        *outKeycode = EMC_KEY_UNBOUND;
        return true;
    }

    int virtualKey = 0;
    std::string canonicalToken;
    if (!TryParsePrimaryKeyToken(tokenValue, &virtualKey, &canonicalToken))
    {
        return false;
    }

    if (canonicalToken.size() == 1)
    {
        switch (canonicalToken[0])
        {
        case '0': *outKeycode = OIS::KC_0; return true;
        case '1': *outKeycode = OIS::KC_1; return true;
        case '2': *outKeycode = OIS::KC_2; return true;
        case '3': *outKeycode = OIS::KC_3; return true;
        case '4': *outKeycode = OIS::KC_4; return true;
        case '5': *outKeycode = OIS::KC_5; return true;
        case '6': *outKeycode = OIS::KC_6; return true;
        case '7': *outKeycode = OIS::KC_7; return true;
        case '8': *outKeycode = OIS::KC_8; return true;
        case '9': *outKeycode = OIS::KC_9; return true;
        case 'A': *outKeycode = OIS::KC_A; return true;
        case 'B': *outKeycode = OIS::KC_B; return true;
        case 'C': *outKeycode = OIS::KC_C; return true;
        case 'D': *outKeycode = OIS::KC_D; return true;
        case 'E': *outKeycode = OIS::KC_E; return true;
        case 'F': *outKeycode = OIS::KC_F; return true;
        case 'G': *outKeycode = OIS::KC_G; return true;
        case 'H': *outKeycode = OIS::KC_H; return true;
        case 'I': *outKeycode = OIS::KC_I; return true;
        case 'J': *outKeycode = OIS::KC_J; return true;
        case 'K': *outKeycode = OIS::KC_K; return true;
        case 'L': *outKeycode = OIS::KC_L; return true;
        case 'M': *outKeycode = OIS::KC_M; return true;
        case 'N': *outKeycode = OIS::KC_N; return true;
        case 'O': *outKeycode = OIS::KC_O; return true;
        case 'P': *outKeycode = OIS::KC_P; return true;
        case 'Q': *outKeycode = OIS::KC_Q; return true;
        case 'R': *outKeycode = OIS::KC_R; return true;
        case 'S': *outKeycode = OIS::KC_S; return true;
        case 'T': *outKeycode = OIS::KC_T; return true;
        case 'U': *outKeycode = OIS::KC_U; return true;
        case 'V': *outKeycode = OIS::KC_V; return true;
        case 'W': *outKeycode = OIS::KC_W; return true;
        case 'X': *outKeycode = OIS::KC_X; return true;
        case 'Y': *outKeycode = OIS::KC_Y; return true;
        case 'Z': *outKeycode = OIS::KC_Z; return true;
        default:
            return false;
        }
    }

    if (canonicalToken.size() >= 2 && canonicalToken[0] == 'F')
    {
        const int functionIndex = std::atoi(canonicalToken.c_str() + 1);
        switch (functionIndex)
        {
        case 1: *outKeycode = OIS::KC_F1; return true;
        case 2: *outKeycode = OIS::KC_F2; return true;
        case 3: *outKeycode = OIS::KC_F3; return true;
        case 4: *outKeycode = OIS::KC_F4; return true;
        case 5: *outKeycode = OIS::KC_F5; return true;
        case 6: *outKeycode = OIS::KC_F6; return true;
        case 7: *outKeycode = OIS::KC_F7; return true;
        case 8: *outKeycode = OIS::KC_F8; return true;
        case 9: *outKeycode = OIS::KC_F9; return true;
        case 10: *outKeycode = OIS::KC_F10; return true;
        case 11: *outKeycode = OIS::KC_F11; return true;
        case 12: *outKeycode = OIS::KC_F12; return true;
        case 13: *outKeycode = OIS::KC_F13; return true;
        case 14: *outKeycode = OIS::KC_F14; return true;
        case 15: *outKeycode = OIS::KC_F15; return true;
        default:
            return false;
        }
    }

    if (canonicalToken == "SPACE")
    {
        *outKeycode = OIS::KC_SPACE;
        return true;
    }
    if (canonicalToken == "TAB")
    {
        *outKeycode = OIS::KC_TAB;
        return true;
    }
    if (canonicalToken == "ENTER")
    {
        *outKeycode = OIS::KC_RETURN;
        return true;
    }
    if (canonicalToken == "ESC")
    {
        *outKeycode = OIS::KC_ESCAPE;
        return true;
    }
    if (canonicalToken == "BACKSPACE")
    {
        *outKeycode = OIS::KC_BACK;
        return true;
    }
    if (canonicalToken == "DELETE")
    {
        *outKeycode = OIS::KC_DELETE;
        return true;
    }
    if (canonicalToken == "INSERT")
    {
        *outKeycode = OIS::KC_INSERT;
        return true;
    }
    if (canonicalToken == "HOME")
    {
        *outKeycode = OIS::KC_HOME;
        return true;
    }
    if (canonicalToken == "END")
    {
        *outKeycode = OIS::KC_END;
        return true;
    }
    if (canonicalToken == "PAGEUP")
    {
        *outKeycode = OIS::KC_PGUP;
        return true;
    }
    if (canonicalToken == "PAGEDOWN")
    {
        *outKeycode = OIS::KC_PGDOWN;
        return true;
    }
    if (canonicalToken == "UP")
    {
        *outKeycode = OIS::KC_UP;
        return true;
    }
    if (canonicalToken == "DOWN")
    {
        *outKeycode = OIS::KC_DOWN;
        return true;
    }
    if (canonicalToken == "LEFT")
    {
        *outKeycode = OIS::KC_LEFT;
        return true;
    }
    if (canonicalToken == "RIGHT")
    {
        *outKeycode = OIS::KC_RIGHT;
        return true;
    }

    return false;
}

bool TryMapOisKeycodeToTogglePanelToken(int32_t keycode, std::string* outToken)
{
    if (!outToken)
    {
        return false;
    }

    outToken->clear();

    if (keycode == EMC_KEY_UNBOUND)
    {
        *outToken = "NONE";
        return true;
    }

    switch (keycode)
    {
    case OIS::KC_0: *outToken = "0"; return true;
    case OIS::KC_1: *outToken = "1"; return true;
    case OIS::KC_2: *outToken = "2"; return true;
    case OIS::KC_3: *outToken = "3"; return true;
    case OIS::KC_4: *outToken = "4"; return true;
    case OIS::KC_5: *outToken = "5"; return true;
    case OIS::KC_6: *outToken = "6"; return true;
    case OIS::KC_7: *outToken = "7"; return true;
    case OIS::KC_8: *outToken = "8"; return true;
    case OIS::KC_9: *outToken = "9"; return true;
    case OIS::KC_A: *outToken = "A"; return true;
    case OIS::KC_B: *outToken = "B"; return true;
    case OIS::KC_C: *outToken = "C"; return true;
    case OIS::KC_D: *outToken = "D"; return true;
    case OIS::KC_E: *outToken = "E"; return true;
    case OIS::KC_F: *outToken = "F"; return true;
    case OIS::KC_G: *outToken = "G"; return true;
    case OIS::KC_H: *outToken = "H"; return true;
    case OIS::KC_I: *outToken = "I"; return true;
    case OIS::KC_J: *outToken = "J"; return true;
    case OIS::KC_K: *outToken = "K"; return true;
    case OIS::KC_L: *outToken = "L"; return true;
    case OIS::KC_M: *outToken = "M"; return true;
    case OIS::KC_N: *outToken = "N"; return true;
    case OIS::KC_O: *outToken = "O"; return true;
    case OIS::KC_P: *outToken = "P"; return true;
    case OIS::KC_Q: *outToken = "Q"; return true;
    case OIS::KC_R: *outToken = "R"; return true;
    case OIS::KC_S: *outToken = "S"; return true;
    case OIS::KC_T: *outToken = "T"; return true;
    case OIS::KC_U: *outToken = "U"; return true;
    case OIS::KC_V: *outToken = "V"; return true;
    case OIS::KC_W: *outToken = "W"; return true;
    case OIS::KC_X: *outToken = "X"; return true;
    case OIS::KC_Y: *outToken = "Y"; return true;
    case OIS::KC_Z: *outToken = "Z"; return true;
    case OIS::KC_F1: *outToken = "F1"; return true;
    case OIS::KC_F2: *outToken = "F2"; return true;
    case OIS::KC_F3: *outToken = "F3"; return true;
    case OIS::KC_F4: *outToken = "F4"; return true;
    case OIS::KC_F5: *outToken = "F5"; return true;
    case OIS::KC_F6: *outToken = "F6"; return true;
    case OIS::KC_F7: *outToken = "F7"; return true;
    case OIS::KC_F8: *outToken = "F8"; return true;
    case OIS::KC_F9: *outToken = "F9"; return true;
    case OIS::KC_F10: *outToken = "F10"; return true;
    case OIS::KC_F11: *outToken = "F11"; return true;
    case OIS::KC_F12: *outToken = "F12"; return true;
    case OIS::KC_F13: *outToken = "F13"; return true;
    case OIS::KC_F14: *outToken = "F14"; return true;
    case OIS::KC_F15: *outToken = "F15"; return true;
    case OIS::KC_SPACE: *outToken = "SPACE"; return true;
    case OIS::KC_TAB: *outToken = "TAB"; return true;
    case OIS::KC_RETURN: *outToken = "ENTER"; return true;
    case OIS::KC_ESCAPE: *outToken = "ESC"; return true;
    case OIS::KC_BACK: *outToken = "BACKSPACE"; return true;
    case OIS::KC_DELETE: *outToken = "DELETE"; return true;
    case OIS::KC_INSERT: *outToken = "INSERT"; return true;
    case OIS::KC_HOME: *outToken = "HOME"; return true;
    case OIS::KC_END: *outToken = "END"; return true;
    case OIS::KC_PGUP: *outToken = "PAGEUP"; return true;
    case OIS::KC_PGDOWN: *outToken = "PAGEDOWN"; return true;
    case OIS::KC_UP: *outToken = "UP"; return true;
    case OIS::KC_DOWN: *outToken = "DOWN"; return true;
    case OIS::KC_LEFT: *outToken = "LEFT"; return true;
    case OIS::KC_RIGHT: *outToken = "RIGHT"; return true;
    default:
        return false;
    }
}

bool TrySaveTogglePanelHotkeyConfig(const char** outError)
{
    if (outError)
    {
        *outError = "";
    }

    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        if (outError)
        {
            *outError = "config_path_unavailable";
        }
        return false;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        if (outError)
        {
            *outError = "config_read_failed";
        }
        return false;
    }

    if (!TryReplaceJsonStringByKey(&configText, "toggle_panel_key", g_togglePanelKey)
        || !TryReplaceJsonBoolByKey(&configText, "toggle_panel_ctrl", g_togglePanelRequireCtrl)
        || !TryReplaceJsonBoolByKey(&configText, "toggle_panel_shift", g_togglePanelRequireShift)
        || !TryReplaceJsonBoolByKey(&configText, "toggle_panel_alt", g_togglePanelRequireAlt))
    {
        if (outError)
        {
            *outError = "config_key_missing";
        }
        return false;
    }

    if (!TryWriteTextFile(configPath, configText))
    {
        if (outError)
        {
            *outError = "config_write_failed";
        }
        return false;
    }

    return true;
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

void UpdateCollapseButtonCaption()
{
    if (!g_collapseButton)
    {
        return;
    }

    g_collapseButton->setCaption(g_panelCollapsed ? "Expand" : "Collapse");
}

void SetWidgetVisible(MyGUI::Widget* widget, bool visible)
{
    if (widget)
    {
        widget->setVisible(visible);
    }
}

void RefreshInventoryFoodItemDropdown()
{
    if (!g_itemDropdown)
    {
        g_filteredInventoryFoodItemOptionIndexes.clear();
        return;
    }

    g_itemDropdown->removeAllItems();
    if (g_itemSearchResultsList)
    {
        g_itemSearchResultsList->removeAllItems();
    }
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
        g_itemDropdown->addItem(option.displayName);
        if (g_itemSearchResultsList)
        {
            g_itemSearchResultsList->addItem(option.displayName);
        }
    }

    if (g_filteredInventoryFoodItemOptionIndexes.empty())
    {
        if (!g_inventoryFoodItemOptionsLoaded)
        {
            g_itemDropdown->addItem("Loading items...");
            if (g_itemSearchResultsList)
            {
                g_itemSearchResultsList->addItem("Loading items...");
            }
        }
        else if (g_inventoryFoodItemOptions.empty())
        {
            g_itemDropdown->addItem("No spawnable items available");
            if (g_itemSearchResultsList)
            {
                g_itemSearchResultsList->addItem("No spawnable items available");
            }
        }
        else
        {
            g_itemDropdown->addItem("No matching items");
            if (g_itemSearchResultsList)
            {
                g_itemSearchResultsList->addItem("No matching items");
            }
        }

        g_itemDropdown->setIndexSelected(0);
        if (g_itemSearchResultsList)
        {
            g_itemSearchResultsList->clearIndexSelected();
        }
        return;
    }

    if (g_filteredInventoryFoodItemOptionIndexes.size() == 1)
    {
        g_itemDropdown->setIndexSelected(0);
        if (g_itemSearchResultsList)
        {
            g_itemSearchResultsList->setIndexSelected(0);
            g_itemSearchResultsList->beginToItemSelected();
        }
        return;
    }

    g_itemDropdown->setIndexSelected(MyGUI::ITEM_NONE);
    if (g_itemSearchResultsList)
    {
        g_itemSearchResultsList->clearIndexSelected();
        g_itemSearchResultsList->beginToItemFirst();
    }
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

    if (!g_itemDropdown || g_filteredInventoryFoodItemOptionIndexes.empty())
    {
        return false;
    }

    const size_t selectedIndex = g_itemDropdown->getIndexSelected();
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

void UpdatePanelTabButtonCaptions()
{
    if (g_healthTabButton)
    {
        g_healthTabButton->setCaption(g_activePanelTab == PanelTab_Health ? "[Health]" : "Health");
    }

    if (g_teleportTabButton)
    {
        g_teleportTabButton->setCaption(g_activePanelTab == PanelTab_Teleport ? "[Teleport]" : "Teleport");
    }

    if (g_inventoryTabButton)
    {
        g_inventoryTabButton->setCaption(g_activePanelTab == PanelTab_Inventory ? "[Inventory]" : "Inventory");
    }
}

void UpdatePanelBodyWidgetVisibility(bool bodyVisible)
{
    const bool healthVisible = bodyVisible && g_activePanelTab == PanelTab_Health;
    const bool teleportVisible = bodyVisible && g_activePanelTab == PanelTab_Teleport;
    const bool inventoryVisible = bodyVisible && g_activePanelTab == PanelTab_Inventory;

    UpdatePanelTabButtonCaptions();

    SetWidgetVisible(g_targetSectionText, bodyVisible);
    SetWidgetVisible(g_targetNameText, bodyVisible);
    SetWidgetVisible(g_targetFactionText, bodyVisible);
    SetWidgetVisible(g_targetAlignmentText, bodyVisible);
    SetWidgetVisible(g_targetMembershipText, bodyVisible);
    SetWidgetVisible(g_targetStateText, bodyVisible);
    SetWidgetVisible(g_noTargetText, bodyVisible);
    SetWidgetVisible(g_healthTabButton, bodyVisible);
    SetWidgetVisible(g_teleportTabButton, bodyVisible);
    SetWidgetVisible(g_inventoryTabButton, bodyVisible);

    SetWidgetVisible(g_statesSectionText, healthVisible);
    SetWidgetVisible(g_fullRestoreButton, healthVisible);
    SetWidgetVisible(g_forceUnconsciousButton, healthVisible);
    SetWidgetVisible(g_forcePlayingDeadButton, healthVisible);
    SetWidgetVisible(g_limbDamageSectionText, healthVisible);
    SetWidgetVisible(g_damageLeftArmButton, healthVisible);
    SetWidgetVisible(g_damageRightArmButton, healthVisible);
    SetWidgetVisible(g_damageLeftLegButton, healthVisible);
    SetWidgetVisible(g_damageRightLegButton, healthVisible);
    SetWidgetVisible(g_dangerousSectionText, healthVisible);
    SetWidgetVisible(g_forceDyingButton, healthVisible);

    SetWidgetVisible(g_teleportSectionText, teleportVisible);
    SetWidgetVisible(g_teleportSelectedToCameraButton, teleportVisible);

    SetWidgetVisible(g_inventorySectionText, inventoryVisible);
    SetWidgetVisible(g_moneyAmountLabelText, inventoryVisible);
    SetWidgetVisible(g_moneyAmountEdit, inventoryVisible);
    SetWidgetVisible(g_addMoneyButton, inventoryVisible);
    SetWidgetVisible(g_spawnFoodSectionText, inventoryVisible);
    SetWidgetVisible(g_itemCategoryLabelText, inventoryVisible);
    SetWidgetVisible(g_itemCategoryDropdown, inventoryVisible);
    SetWidgetVisible(g_itemSearchLabelText, inventoryVisible);
    SetWidgetVisible(g_itemSearchEdit, inventoryVisible);
    SetWidgetVisible(g_itemSearchResultsList, inventoryVisible);
    SetWidgetVisible(g_itemDropdownLabelText, inventoryVisible);
    SetWidgetVisible(g_itemDropdown, inventoryVisible);
    SetWidgetVisible(g_itemQuantityLabelText, inventoryVisible);
    SetWidgetVisible(g_itemQuantityEdit, inventoryVisible);
    SetWidgetVisible(g_spawnItemButton, inventoryVisible);

    SetWidgetVisible(g_statusText, bodyVisible);
}

void PersistCollapsedStateSetting()
{
    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        LogWarnLine("collapsed-state persistence skipped: could not resolve mod config path");
        return;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        std::stringstream line;
        line << "collapsed-state persistence skipped: could not read " << configPath;
        LogWarnLine(line.str());
        return;
    }

    if (!TryReplaceJsonBoolByKey(&configText, "start_collapsed", g_panelCollapsed))
    {
        std::stringstream line;
        line << "collapsed-state persistence skipped: missing start_collapsed in " << configPath;
        LogWarnLine(line.str());
        return;
    }

    if (!TryWriteTextFile(configPath, configText))
    {
        std::stringstream line;
        line << "collapsed-state persistence failed: could not write " << configPath;
        LogWarnLine(line.str());
        return;
    }

    std::stringstream line;
    line << "event=testkit_panel_collapsed_persisted collapsed=" << (g_panelCollapsed ? "true" : "false");
    LogDebugLine(line.str());
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

bool TryGetViewportSize(int* widthOut, int* heightOut)
{
    if (!widthOut || !heightOut)
    {
        return false;
    }

    MyGUI::RenderManager* renderManager = MyGUI::RenderManager::getInstancePtr();
    if (!renderManager)
    {
        return false;
    }

    const MyGUI::IntSize view = renderManager->getViewSize();
    if (view.width <= 0 || view.height <= 0)
    {
        return false;
    }

    *widthOut = view.width;
    *heightOut = view.height;
    return true;
}

bool TryGetMousePosition(int* xOut, int* yOut)
{
    if (!xOut || !yOut)
    {
        return false;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    if (!inputManager)
    {
        return false;
    }

    const MyGUI::IntPoint mouse = inputManager->getMousePosition();
    *xOut = mouse.left;
    *yOut = mouse.top;
    return true;
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

MyGUI::IntCoord BuildPanelCoordFromAnchor(int left, int top)
{
    const int height = g_panelCollapsed ? kPanelCollapsedHeight : kPanelExpandedHeight;
    return MyGUI::IntCoord(left, top, kPanelWidth, height);
}

MyGUI::IntCoord ClampPanelCoordToViewport(const MyGUI::IntCoord& inputCoord)
{
    int left = inputCoord.left;
    int top = inputCoord.top;
    const int width = (inputCoord.width > 0) ? inputCoord.width : kPanelWidth;
    const int height = (inputCoord.height > 0)
        ? inputCoord.height
        : (g_panelCollapsed ? kPanelCollapsedHeight : kPanelExpandedHeight);

    int viewWidth = 0;
    int viewHeight = 0;
    if (!TryGetViewportSize(&viewWidth, &viewHeight))
    {
        if (left < kPanelViewportPadding)
        {
            left = kPanelViewportPadding;
        }
        if (top < kPanelViewportPadding)
        {
            top = kPanelViewportPadding;
        }
        return MyGUI::IntCoord(left, top, width, height);
    }

    int minLeft = kPanelViewportPadding;
    int minTop = kPanelViewportPadding;
    int maxLeft = viewWidth - width - kPanelViewportPadding;
    int maxTop = viewHeight - height - kPanelViewportPadding;

    if (maxLeft < minLeft)
    {
        minLeft = 0;
        maxLeft = viewWidth - width;
        if (maxLeft < minLeft)
        {
            maxLeft = minLeft;
        }
    }

    if (maxTop < minTop)
    {
        minTop = 0;
        maxTop = viewHeight - height;
        if (maxTop < minTop)
        {
            maxTop = minTop;
        }
    }

    left = ClampIntValue(left, minLeft, maxLeft);
    top = ClampIntValue(top, minTop, maxTop);
    return MyGUI::IntCoord(left, top, width, height);
}

void StorePanelRuntimePosition(const MyGUI::IntCoord& panelCoord)
{
    g_runtimePanelPositionSet = true;
    g_runtimePanelLeft = panelCoord.left;
    g_runtimePanelTop = panelCoord.top;
}

MyGUI::IntCoord ResolvePanelCoord()
{
    if (g_runtimePanelPositionSet)
    {
        return ClampPanelCoordToViewport(BuildPanelCoordFromAnchor(g_runtimePanelLeft, g_runtimePanelTop));
    }

    return ClampPanelCoordToViewport(BuildPanelCoordFromAnchor(kPanelLeft, kPanelTop));
}

void ApplyPanelLayout(const MyGUI::IntCoord& panelCoord)
{
    if (!g_panel)
    {
        return;
    }

    const MyGUI::IntCoord clampedCoord = ClampPanelCoordToViewport(panelCoord);
    StorePanelRuntimePosition(clampedCoord);

    g_panel->setCoord(clampedCoord);
    g_panel->setVisible(!g_panelHidden);

    const bool bodyVisible = !g_panelHidden && !g_panelCollapsed;

    if (g_headerFrame)
    {
        g_headerFrame->setCoord(MyGUI::IntCoord(0, 0, kPanelWidth, 38));
    }

    if (g_headerTitleText)
    {
        g_headerTitleText->setCoord(MyGUI::IntCoord(12, 8, kPanelWidth - 116, 22));
    }

    if (g_collapseButton)
    {
        g_collapseButton->setCoord(MyGUI::IntCoord(kPanelWidth - 96, 4, 84, 30));
    }

    if (g_bodyFrame)
    {
        g_bodyFrame->setCoord(MyGUI::IntCoord(0, 38, kPanelWidth, kPanelExpandedHeight - 38));
        g_bodyFrame->setVisible(bodyVisible);
    }

    UpdatePanelBodyWidgetVisibility(bodyVisible);
}

void ApplyPanelLayout()
{
    if (!g_panel)
    {
        return;
    }

    ApplyPanelLayout(ResolvePanelCoord());
}

void MovePanelByDelta(int deltaX, int deltaY)
{
    if (!g_panel || (deltaX == 0 && deltaY == 0))
    {
        return;
    }

    const int moveX = (deltaX < 0) ? -deltaX : deltaX;
    const int moveY = (deltaY < 0) ? -deltaY : deltaY;
    g_panelDragMovedDistance += moveX + moveY;
    if (g_panelDragMovedDistance >= kPanelDragThreshold)
    {
        g_panelDragMoved = true;
    }

    const MyGUI::IntCoord currentCoord = g_panel->getCoord();
    const MyGUI::IntCoord movedCoord = ClampPanelCoordToViewport(
        BuildPanelCoordFromAnchor(currentCoord.left + deltaX, currentCoord.top + deltaY));
    ApplyPanelLayout(movedCoord);
}

void FinalizePanelDrag(const char* source)
{
    if (!g_panelDragging)
    {
        return;
    }

    g_panelDragging = false;
    if (!g_panel)
    {
        g_panelDragMoved = false;
        g_panelDragMovedDistance = 0;
        return;
    }

    const MyGUI::IntCoord clampedCoord = ClampPanelCoordToViewport(g_panel->getCoord());
    ApplyPanelLayout(clampedCoord);

    if (g_panelDragMoved)
    {
        std::stringstream line;
        line << "event=testkit_panel_moved left=" << clampedCoord.left
             << " top=" << clampedCoord.top;
        if (source)
        {
            line << " source=\"" << source << "\"";
        }
        LogInfoLine(line.str());
    }

    g_panelDragMoved = false;
    g_panelDragMovedDistance = 0;
}

void OnHeaderMousePressed(MyGUI::Widget*, int left, int top, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    g_panelDragging = true;
    g_panelDragMoved = false;
    if (!TryGetMousePosition(&g_panelDragLastMouseX, &g_panelDragLastMouseY))
    {
        g_panelDragLastMouseX = left;
        g_panelDragLastMouseY = top;
    }
    g_panelDragMovedDistance = 0;
}

void OnHeaderMouseDrag(MyGUI::Widget*, int left, int top, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    if (!g_panelDragging || !g_panel)
    {
        return;
    }

    int mouseX = left;
    int mouseY = top;
    TryGetMousePosition(&mouseX, &mouseY);

    const int deltaX = mouseX - g_panelDragLastMouseX;
    const int deltaY = mouseY - g_panelDragLastMouseY;
    if (deltaX == 0 && deltaY == 0)
    {
        return;
    }

    MovePanelByDelta(deltaX, deltaY);
    g_panelDragLastMouseX = mouseX;
    g_panelDragLastMouseY = mouseY;
}

void OnHeaderMouseMove(MyGUI::Widget*, int left, int top)
{
    if (!g_panelDragging)
    {
        return;
    }

    OnHeaderMouseDrag(0, left, top, MyGUI::MouseButton::Left);
}

void OnHeaderMouseReleased(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    FinalizePanelDrag("drag_release");
}

void TickPanelDrag()
{
    if (!g_panelDragging || !g_panel)
    {
        return;
    }

    int mouseX = 0;
    int mouseY = 0;
    if (TryGetMousePosition(&mouseX, &mouseY))
    {
        const int deltaX = mouseX - g_panelDragLastMouseX;
        const int deltaY = mouseY - g_panelDragLastMouseY;
        MovePanelByDelta(deltaX, deltaY);
        g_panelDragLastMouseX = mouseX;
        g_panelDragLastMouseY = mouseY;
    }

    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0)
    {
        FinalizePanelDrag("drag_release_poll");
    }
}

void ResetPanelWidgetPointers()
{
    g_panel = 0;
    g_headerFrame = 0;
    g_headerTitleText = 0;
    g_collapseButton = 0;
    g_bodyFrame = 0;
    g_targetSectionText = 0;
    g_targetNameText = 0;
    g_targetFactionText = 0;
    g_targetAlignmentText = 0;
    g_targetMembershipText = 0;
    g_targetStateText = 0;
    g_noTargetText = 0;
    g_healthTabButton = 0;
    g_teleportTabButton = 0;
    g_inventoryTabButton = 0;
    g_statesSectionText = 0;
    g_fullRestoreButton = 0;
    g_forceUnconsciousButton = 0;
    g_forcePlayingDeadButton = 0;
    g_limbDamageSectionText = 0;
    g_damageLeftArmButton = 0;
    g_damageRightArmButton = 0;
    g_damageLeftLegButton = 0;
    g_damageRightLegButton = 0;
    g_teleportSectionText = 0;
    g_teleportSelectedToCameraButton = 0;
    g_inventorySectionText = 0;
    g_moneyAmountLabelText = 0;
    g_moneyAmountEdit = 0;
    g_addMoneyButton = 0;
    g_spawnFoodSectionText = 0;
    g_itemCategoryLabelText = 0;
    g_itemCategoryDropdown = 0;
    g_itemSearchLabelText = 0;
    g_itemSearchEdit = 0;
    g_itemSearchResultsList = 0;
    g_itemDropdownLabelText = 0;
    g_itemDropdown = 0;
    g_itemQuantityLabelText = 0;
    g_itemQuantityEdit = 0;
    g_spawnItemButton = 0;
    g_dangerousSectionText = 0;
    g_forceDyingButton = 0;
    g_statusText = 0;
}

void DestroyPanel()
{
    if (g_panel)
    {
        StorePanelRuntimePosition(ClampPanelCoordToViewport(g_panel->getCoord()));
    }

    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui && g_panel)
    {
        gui->destroyWidget(g_panel);
    }

    ResetPanelWidgetPointers();
    g_panelDragging = false;
    g_panelDragMoved = false;
    g_panelDragLastMouseX = 0;
    g_panelDragLastMouseY = 0;
    g_panelDragMovedDistance = 0;
    g_forceDyingArmed = false;
    g_forceDyingArmedAtMs = 0;
}

void LogHotkeyBindingFallback()
{
    std::stringstream line;
    line << "invalid toggle_panel_key=\"" << g_togglePanelKey
         << "\"; falling back to \"" << kDefaultTogglePanelKey << "\"";
    LogWarnLine(line.str());
}

void RefreshHotkeyBinding()
{
    const std::string keyToken = TrimAscii(g_togglePanelKey);
    const std::string keyUpper = ToUpperAscii(keyToken);
    if (keyUpper == "NONE" || keyUpper == "UNBOUND")
    {
        g_hotkeyEnabled = false;
        g_hotkeyVirtualKey = 0;
        g_hotkeyDisplay = "NONE";
        g_hotkeyPrevDown = false;
        g_togglePanelKey = "NONE";
        return;
    }

    std::string canonicalKey;
    int virtualKey = 0;
    if (!TryParsePrimaryKeyToken(keyToken, &virtualKey, &canonicalKey))
    {
        LogHotkeyBindingFallback();
        canonicalKey = kDefaultTogglePanelKey;
        virtualKey = 'D';
    }

    g_hotkeyEnabled = true;
    g_hotkeyVirtualKey = virtualKey;
    g_togglePanelKey = canonicalKey;

    std::stringstream display;
    if (g_togglePanelRequireCtrl)
    {
        display << "CTRL+";
    }
    if (g_togglePanelRequireAlt)
    {
        display << "ALT+";
    }
    if (g_togglePanelRequireShift)
    {
        display << "SHIFT+";
    }
    display << canonicalKey;
    g_hotkeyDisplay = display.str();
}

void LoadConfig()
{
    g_pluginEnabled = true;
    g_loggingLevel = LoggingLevel_Info;
    g_togglePanelRequireCtrl = true;
    g_togglePanelRequireShift = true;
    g_togglePanelRequireAlt = false;
    g_togglePanelKey = kDefaultTogglePanelKey;
    g_confirmDangerousActions = true;
    g_panelHidden = false;
    g_panelCollapsed = false;

    std::string configPath;
    if (!TryResolveModConfigPath(&configPath))
    {
        LogWarnLine("mod config load skipped: could not resolve plugin directory (using defaults)");
        RefreshHotkeyBinding();
        return;
    }

    std::string configText;
    if (!TryReadTextFile(configPath, &configText))
    {
        std::stringstream line;
        line << "mod config load skipped: could not read " << configPath << " (using defaults)";
        LogWarnLine(line.str());
        RefreshHotkeyBinding();
        return;
    }

    bool parsedBool = false;
    std::string parsedString;

    if (TryParseJsonBoolByKey(configText, "enabled", &parsedBool))
    {
        g_pluginEnabled = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "toggle_panel_ctrl", &parsedBool))
    {
        g_togglePanelRequireCtrl = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "toggle_panel_shift", &parsedBool))
    {
        g_togglePanelRequireShift = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "toggle_panel_alt", &parsedBool))
    {
        g_togglePanelRequireAlt = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "start_hidden", &parsedBool))
    {
        g_panelHidden = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "start_collapsed", &parsedBool))
    {
        g_panelCollapsed = parsedBool;
    }
    if (TryParseJsonBoolByKey(configText, "confirm_dangerous_actions", &parsedBool))
    {
        g_confirmDangerousActions = parsedBool;
    }

    if (TryParseJsonStringByKey(configText, "toggle_panel_key", &parsedString))
    {
        g_togglePanelKey = parsedString;
    }

    if (TryParseJsonStringByKey(configText, "logging_level", &parsedString))
    {
        const std::string levelUpper = ToUpperAscii(TrimAscii(parsedString));
        if (levelUpper == "DEBUG")
        {
            g_loggingLevel = LoggingLevel_Debug;
        }
    }
    else if (TryParseJsonBoolByKey(configText, "debugLogging", &parsedBool) && parsedBool)
    {
        g_loggingLevel = LoggingLevel_Debug;
    }

    RefreshHotkeyBinding();

    std::stringstream info;
    info << "mod config loaded enabled=" << (g_pluginEnabled ? "true" : "false")
         << " hotkey=\"" << g_hotkeyDisplay << "\""
         << " start_hidden=" << (g_panelHidden ? "true" : "false")
         << " start_collapsed=" << (g_panelCollapsed ? "true" : "false")
         << " confirm_dangerous_actions=" << (g_confirmDangerousActions ? "true" : "false");
    LogInfoLine(info.str());
}

bool IsAnyVirtualKeyDown(int primaryVk, int leftVk, int rightVk)
{
    return (GetAsyncKeyState(primaryVk) & 0x8000) != 0
        || (GetAsyncKeyState(leftVk) & 0x8000) != 0
        || (GetAsyncKeyState(rightVk) & 0x8000) != 0;
}

bool IsPanelToggleHotkeyDown()
{
    if (!g_hotkeyEnabled || g_hotkeyVirtualKey == 0)
    {
        return false;
    }

    if (g_togglePanelRequireCtrl && !IsAnyVirtualKeyDown(VK_CONTROL, VK_LCONTROL, VK_RCONTROL))
    {
        return false;
    }

    if (g_togglePanelRequireAlt && !IsAnyVirtualKeyDown(VK_MENU, VK_LMENU, VK_RMENU))
    {
        return false;
    }

    if (g_togglePanelRequireShift && !IsAnyVirtualKeyDown(VK_SHIFT, VK_LSHIFT, VK_RSHIFT))
    {
        return false;
    }

    return (GetAsyncKeyState(g_hotkeyVirtualKey) & 0x8000) != 0;
}

void LogPanelToggleEvent(bool visible, const char* source)
{
    std::stringstream line;
    line << "event=testkit_panel_toggled visible=" << (visible ? "true" : "false");
    if (source)
    {
        line << " source=\"" << source << "\"";
    }
    line << " hotkey=\"" << g_hotkeyDisplay << "\"";
    LogInfoLine(line.str());
}

void LogPanelCollapsedEvent(bool collapsed, const char* source)
{
    std::stringstream line;
    line << "event=testkit_panel_collapsed collapsed=" << (collapsed ? "true" : "false");
    if (source)
    {
        line << " source=\"" << source << "\"";
    }
    LogInfoLine(line.str());
}

EMC_Result __cdecl GetTogglePanelHotkeyKeybind(void*, EMC_KeybindValueV1* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    int32_t keycode = EMC_KEY_UNBOUND;
    if (g_hotkeyEnabled && !TryMapTogglePanelTokenToOisKeycode(g_togglePanelKey, &keycode))
    {
        keycode = EMC_KEY_UNBOUND;
    }

    outValue->keycode = keycode;
    outValue->modifiers = 0u;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelHotkeyKeybind(void*, EMC_KeybindValueV1 value, char* errBuf, uint32_t errBufSize)
{
    if (value.modifiers != 0u)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "use_modifier_toggles");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    std::string updatedToken;
    if (!TryMapOisKeycodeToTogglePanelToken(value.keycode, &updatedToken))
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "invalid_keybind");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const std::string previousToken = g_togglePanelKey;
    g_togglePanelKey = updatedToken;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelKey = previousToken;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetTogglePanelRequireCtrl(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_togglePanelRequireCtrl ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelRequireCtrl(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_togglePanelRequireCtrl;
    g_togglePanelRequireCtrl = value != 0;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelRequireCtrl = previousValue;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetTogglePanelRequireShift(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_togglePanelRequireShift ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelRequireShift(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_togglePanelRequireShift;
    g_togglePanelRequireShift = value != 0;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelRequireShift = previousValue;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

EMC_Result __cdecl GetTogglePanelRequireAlt(void*, int32_t* outValue)
{
    if (!outValue)
    {
        return EMC_ERR_INVALID_ARGUMENT;
    }

    *outValue = g_togglePanelRequireAlt ? 1 : 0;
    return EMC_OK;
}

EMC_Result __cdecl SetTogglePanelRequireAlt(void*, int32_t value, char* errBuf, uint32_t errBufSize)
{
    if (value != 0 && value != 1)
    {
        CopyModHubErrorMessage(errBuf, errBufSize, "value_must_be_bool");
        return EMC_ERR_INVALID_ARGUMENT;
    }

    const bool previousValue = g_togglePanelRequireAlt;
    g_togglePanelRequireAlt = value != 0;
    RefreshHotkeyBinding();

    const char* saveError = "";
    if (!TrySaveTogglePanelHotkeyConfig(&saveError))
    {
        g_togglePanelRequireAlt = previousValue;
        RefreshHotkeyBinding();
        CopyModHubErrorMessage(errBuf, errBufSize, saveError);
        return EMC_ERR_CALLBACK_FAILED;
    }

    CopyModHubErrorMessage(errBuf, errBufSize, 0);
    return EMC_OK;
}

const EMC_ModDescriptorV1 kModHubModDescriptor = {
    kModHubNamespaceId,
    kModHubNamespaceDisplayName,
    kModHubModId,
    kModHubModDisplayName,
    0
};

const EMC_KeybindSettingDefV1 kModHubTogglePanelKeySetting = {
    "toggle_panel_key",
    kModHubTogglePanelKeyLabel,
    kModHubTogglePanelKeyDescription,
    0,
    &GetTogglePanelHotkeyKeybind,
    &SetTogglePanelHotkeyKeybind
};

const EMC_BoolSettingDefV1 kModHubTogglePanelCtrlSetting = {
    "toggle_panel_ctrl",
    kModHubTogglePanelCtrlLabel,
    kModHubTogglePanelCtrlDescription,
    0,
    &GetTogglePanelRequireCtrl,
    &SetTogglePanelRequireCtrl
};

const EMC_BoolSettingDefV1 kModHubTogglePanelShiftSetting = {
    "toggle_panel_shift",
    kModHubTogglePanelShiftLabel,
    kModHubTogglePanelShiftDescription,
    0,
    &GetTogglePanelRequireShift,
    &SetTogglePanelRequireShift
};

const EMC_BoolSettingDefV1 kModHubTogglePanelAltSetting = {
    "toggle_panel_alt",
    kModHubTogglePanelAltLabel,
    kModHubTogglePanelAltDescription,
    0,
    &GetTogglePanelRequireAlt,
    &SetTogglePanelRequireAlt
};

const emc::ModHubClientSettingRowV1 kModHubRows[] = {
    { emc::MOD_HUB_CLIENT_SETTING_KIND_KEYBIND, &kModHubTogglePanelKeySetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelCtrlSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelShiftSetting },
    { emc::MOD_HUB_CLIENT_SETTING_KIND_BOOL, &kModHubTogglePanelAltSetting }
};

const emc::ModHubClientTableRegistrationV1 kModHubRegistration = {
    &kModHubModDescriptor,
    kModHubRows,
    static_cast<uint32_t>(sizeof(kModHubRows) / sizeof(kModHubRows[0]))
};

void EnsureModHubClientConfigured()
{
    if (g_modHubClientConfigured)
    {
        return;
    }

    emc::ModHubClient::Config config;
    config.table_registration = &kModHubRegistration;
    g_modHubClient.SetConfig(config);
    g_modHubClientConfigured = true;
}

void LogModHubClientAttemptResult(const char* phase, emc::ModHubClient::AttemptResult result)
{
    std::stringstream line;
    line << "event=testkit_mod_hub_attach phase=\"" << (phase ? phase : "unknown") << "\"";

    switch (result)
    {
    case emc::ModHubClient::ATTACH_SUCCESS:
        line << " result=\"success\"";
        break;
    case emc::ModHubClient::ATTACH_FAILED:
        line << " result=\"attach_failed\"";
        break;
    case emc::ModHubClient::REGISTRATION_FAILED:
        line << " result=\"registration_failed\"";
        break;
    case emc::ModHubClient::INVALID_CONFIGURATION:
        line << " result=\"invalid_configuration\"";
        break;
    default:
        line << " result=\"unknown\"";
        break;
    }

    line << " failure_code=" << g_modHubClient.LastAttemptFailureResult()
         << " use_hub_ui=" << (g_modHubClient.UseHubUi() ? "true" : "false")
         << " retry_pending=" << (g_modHubClient.IsAttachRetryPending() ? "true" : "false")
         << " retried=" << (g_modHubClient.HasAttachRetryAttempted() ? "true" : "false");
    LogInfoLine(line.str());
}

void StartModHubClient()
{
    EnsureModHubClientConfigured();
    LogModHubClientAttemptResult("startup", g_modHubClient.OnStartup());
}

void TickModHubAttachRetry()
{
    if (!g_modHubClient.IsAttachRetryPending() || g_modHubClient.HasAttachRetryAttempted())
    {
        return;
    }

    LogModHubClientAttemptResult("retry", g_modHubClient.OnOptionsWindowInit());
}

void TogglePanelHidden(const char* source)
{
    g_panelHidden = !g_panelHidden;

    if (g_panelHidden)
    {
        ClearForceDyingArm("panel_hidden", false);
    }
    else
    {
        SetStatusMessage("Panel shown");
    }

    ApplyPanelLayout();
    LogPanelToggleEvent(!g_panelHidden, source);
}

void TogglePanelCollapsed(const char* source)
{
    g_panelCollapsed = !g_panelCollapsed;
    if (g_panelCollapsed)
    {
        ClearForceDyingArm("panel_collapsed", false);
    }

    UpdateCollapseButtonCaption();
    ApplyPanelLayout();
    PersistCollapsedStateSetting();
    SetStatusMessage(g_panelCollapsed ? "Panel collapsed" : "Panel expanded");
    LogPanelCollapsedEvent(g_panelCollapsed, source);
}

void TickPanelToggleHotkey()
{
    const bool hotkeyDown = IsPanelToggleHotkeyDown();
    if (hotkeyDown && !g_hotkeyPrevDown)
    {
        std::stringstream line;
        line << "event=testkit_hotkey_triggered hotkey=\"" << g_hotkeyDisplay << "\"";
        LogInfoLine(line.str());
        TogglePanelHidden("hotkey");
    }

    g_hotkeyPrevDown = hotkeyDown;
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

std::string SafeFactionName(Character* target)
{
    if (!target || !target->owner)
    {
        return "Unknown";
    }

    const std::string name = target->owner->getName();
    if (name.empty())
    {
        return "Unknown";
    }

    return name;
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
}

void SetSelectionActionButtonsEnabled(bool enabled)
{
    if (g_teleportSelectedToCameraButton)
    {
        g_teleportSelectedToCameraButton->setEnabled(enabled);
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
    SetSelectionActionButtonsEnabled(GetSelectedCharacterCount(player) > 0);
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
        g_noTargetText->setCaption("No target - select a character");
        SetActionButtonsEnabled(false);
        return;
    }

    g_targetNameText->setCaption("Name: " + snapshot.name);
    g_targetFactionText->setCaption("Faction: " + snapshot.factionName);
    g_targetAlignmentText->setCaption("Alignment: " + snapshot.alignment);
    g_targetMembershipText->setCaption("Membership: " + snapshot.membership);
    g_targetStateText->setCaption("State: " + snapshot.stateLabel);
    g_noTargetText->setCaption(std::string("Source: ") + TargetSourceToUiLabel(snapshot.source));
    SetActionButtonsEnabled(true);
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
    int* selectedCountOut,
    int* teleportedCountOut,
    Ogre::Vector3* destinationCenterOut)
{
    if (selectedCountOut)
    {
        *selectedCountOut = 0;
    }
    if (teleportedCountOut)
    {
        *teleportedCountOut = 0;
    }
    if (destinationCenterOut)
    {
        *destinationCenterOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }

    if (!player || !ou)
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

        const Ogre::Vector3 destinationCenter = kTeleportDestinationCenter;
        if (destinationCenterOut)
        {
            *destinationCenterOut = destinationCenter;
        }

        Ogre::Vector3 resolvedDestination = destinationCenter;
        if (ou)
        {
            Ogre::Vector3 validatedDestination = resolvedDestination;
            if (ou->findValidSpawnPos(validatedDestination, resolvedDestination))
            {
                resolvedDestination = validatedDestination;
            }
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

            character->teleport(resolvedDestination - character->getPosition(), character->getOrientation());
            character->setRagdollNavmeshSafePos();
            ++teleportedCount;
        }

        if (useSelectedCharacterFallback)
        {
            Character* character = player->selectedCharacter.getCharacter();
            if (character)
            {
                character->teleport(resolvedDestination - character->getPosition(), character->getOrientation());
                character->setRagdollNavmeshSafePos();
                ++teleportedCount;
            }
        }

        if (teleportedCountOut)
        {
            *teleportedCountOut = teleportedCount;
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

void OnCollapseButtonClicked(MyGUI::Widget*)
{
    TogglePanelCollapsed("button");
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

void OnInventoryCategoryChanged(MyGUI::ComboBox*, size_t)
{
    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
}

void OnInventorySearchResultsActivated(MyGUI::ListBox*, size_t index)
{
    if (index >= g_filteredInventoryFoodItemOptionIndexes.size())
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    TryPopulateInventoryFoodSelection(g_filteredInventoryFoodItemOptionIndexes[index]);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
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

void OnTeleportSelectedToCameraButtonClicked(MyGUI::Widget*)
{
    std::stringstream requested;
    requested << "event=testkit_action_requested action=\"teleport_selected_to_test_spot\"";
    LogInfoLine(requested.str());

    if (!g_lastPlayerInterface)
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_test_spot\" success=false reason=\"no_player_interface\"");
        SetStatusMessage("Teleport failed - player interface unavailable");
        return;
    }

    int selectedCount = 0;
    int teleportedCount = 0;
    Ogre::Vector3 destinationCenter(0.0f, 0.0f, 0.0f);
    if (!TryTeleportSelectedCharactersToCamera(
            g_lastPlayerInterface,
            &selectedCount,
            &teleportedCount,
            &destinationCenter))
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_test_spot\" success=false reason=\"apply_failed\"");
        SetStatusMessage(std::string("Teleport to ") + kTeleportDestinationLabel + " failed - apply path unavailable");
        return;
    }

    if (selectedCount <= 0)
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_test_spot\" success=false reason=\"no_selection\"");
        SetStatusMessage(std::string("No selected characters to teleport to ") + kTeleportDestinationLabel);
        return;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"teleport_selected_to_test_spot\" success="
           << (teleportedCount > 0 ? "true" : "false")
           << " selected_count=" << selectedCount
           << " teleported_count=" << teleportedCount
           << " destination_x=" << destinationCenter.x
           << " destination_y=" << destinationCenter.y
           << " destination_z=" << destinationCenter.z;
    if (teleportedCount <= 0)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (teleportedCount == selectedCount)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " selected character(s) to " << kTeleportDestinationLabel;
        SetStatusMessage(status.str());
    }
    else if (teleportedCount > 0)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " of " << selectedCount << " selected characters to "
               << kTeleportDestinationLabel;
        SetStatusMessage(status.str());
    }
    else
    {
        SetStatusMessage(std::string("Teleport to ") + kTeleportDestinationLabel + " requested - no selected characters moved");
    }

    UpdateTargetInspection(g_lastPlayerInterface);
}

void OnTeleportSelectedToCameraButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnTeleportSelectedToCameraButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void SetActivePanelTab(PanelTab tab)
{
    if (g_activePanelTab == tab)
    {
        return;
    }

    g_activePanelTab = tab;
    ApplyPanelLayout();
}

void OnHealthTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Health);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnTeleportTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Teleport);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnInventoryTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Inventory);

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

void ConfigureTextWidget(MyGUI::TextBox* widget)
{
    if (!widget)
    {
        return;
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
}

void ConfigureEditBoxWidget(MyGUI::EditBox* widget)
{
    if (!widget)
    {
        return;
    }

    MyGUI::Widget* clientWidget = widget->getClientWidget();
    if (clientWidget)
    {
        clientWidget->setAlign(MyGUI::Align::Stretch);
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
}

void ConfigureComboBoxWidget(MyGUI::ComboBox* widget)
{
    if (!widget)
    {
        return;
    }

    MyGUI::Widget* clientWidget = widget->getClientWidget();
    if (clientWidget)
    {
        clientWidget->setAlign(MyGUI::Align::Stretch);
    }

    widget->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
    widget->setComboModeDrop(true);
    widget->setMaxListLength(kInventoryItemDropdownMaxListLength);
}

void ConfigureListBoxWidget(MyGUI::ListBox* widget)
{
    if (!widget)
    {
        return;
    }

    widget->setScrollVisible(true);
    widget->setActivateOnClick(true);
}

bool HasAllPanelWidgets()
{
    return g_panel
        && g_headerFrame
        && g_headerTitleText
        && g_collapseButton
        && g_bodyFrame
        && g_targetSectionText
        && g_targetNameText
        && g_targetFactionText
        && g_targetAlignmentText
        && g_targetMembershipText
        && g_targetStateText
        && g_noTargetText
        && g_healthTabButton
        && g_teleportTabButton
        && g_inventoryTabButton
        && g_statesSectionText
        && g_fullRestoreButton
        && g_forceUnconsciousButton
        && g_forcePlayingDeadButton
        && g_limbDamageSectionText
        && g_damageLeftArmButton
        && g_damageRightArmButton
        && g_damageLeftLegButton
        && g_damageRightLegButton
        && g_teleportSectionText
        && g_teleportSelectedToCameraButton
        && g_inventorySectionText
        && g_moneyAmountLabelText
        && g_moneyAmountEdit
        && g_addMoneyButton
        && g_spawnFoodSectionText
        && g_itemCategoryLabelText
        && g_itemCategoryDropdown
        && g_itemSearchLabelText
        && g_itemSearchEdit
        && g_itemSearchResultsList
        && g_itemDropdownLabelText
        && g_itemDropdown
        && g_itemQuantityLabelText
        && g_itemQuantityEdit
        && g_spawnItemButton
        && g_dangerousSectionText
        && g_forceDyingButton
        && g_statusText;
}

void InitializePanelWidgets()
{
    if (!HasAllPanelWidgets())
    {
        return;
    }

    g_headerFrame->setCaption("");
    g_headerFrame->setEnabled(true);
    g_headerFrame->setNeedMouseFocus(true);
    g_headerFrame->setNeedKeyFocus(true);
    g_panel->setNeedMouseFocus(true);
    g_headerTitleText->setCaption("Emkejs Test Kit");
    g_headerTitleText->setNeedMouseFocus(false);
    ConfigureTextWidget(g_headerTitleText);

    g_bodyFrame->setCaption("");
    g_bodyFrame->setEnabled(false);
    g_bodyFrame->setNeedMouseFocus(true);

    ConfigureTextWidget(g_targetSectionText);
    ConfigureTextWidget(g_targetNameText);
    ConfigureTextWidget(g_targetFactionText);
    ConfigureTextWidget(g_targetAlignmentText);
    ConfigureTextWidget(g_targetMembershipText);
    ConfigureTextWidget(g_targetStateText);
    ConfigureTextWidget(g_noTargetText);
    ConfigureTextWidget(g_statesSectionText);
    ConfigureTextWidget(g_limbDamageSectionText);
    ConfigureTextWidget(g_teleportSectionText);
    ConfigureTextWidget(g_inventorySectionText);
    ConfigureTextWidget(g_moneyAmountLabelText);
    ConfigureTextWidget(g_spawnFoodSectionText);
    ConfigureTextWidget(g_itemCategoryLabelText);
    ConfigureTextWidget(g_itemSearchLabelText);
    ConfigureTextWidget(g_itemDropdownLabelText);
    ConfigureTextWidget(g_itemQuantityLabelText);
    ConfigureTextWidget(g_dangerousSectionText);
    ConfigureTextWidget(g_statusText);
    ConfigureEditBoxWidget(g_moneyAmountEdit);
    ConfigureEditBoxWidget(g_itemSearchEdit);
    ConfigureEditBoxWidget(g_itemQuantityEdit);
    ConfigureComboBoxWidget(g_itemCategoryDropdown);
    ConfigureComboBoxWidget(g_itemDropdown);
    ConfigureListBoxWidget(g_itemSearchResultsList);

    g_targetSectionText->setCaption("Target");
    g_targetNameText->setCaption("Name: Pending target inspection");
    g_targetFactionText->setCaption("Faction: Unknown");
    g_targetAlignmentText->setCaption("Alignment: Unknown");
    g_targetMembershipText->setCaption("Membership: Unknown");
    g_targetStateText->setCaption("State: Unknown");
    g_noTargetText->setCaption("No target - select a character");
    UpdatePanelTabButtonCaptions();
    g_statesSectionText->setCaption("States");
    g_limbDamageSectionText->setCaption("Limb Damage");
    g_teleportSectionText->setCaption("Teleport");
    g_inventorySectionText->setCaption("Inventory");
    g_moneyAmountLabelText->setCaption("Cats To Add");
    g_spawnFoodSectionText->setCaption("Spawn Items");
    g_itemCategoryLabelText->setCaption("Category");
    g_itemSearchLabelText->setCaption("Search");
    g_itemDropdownLabelText->setCaption("Selected Item");
    g_itemQuantityLabelText->setCaption("Quantity");
    g_dangerousSectionText->setCaption("Dangerous");

    g_fullRestoreButton->setCaption("Full Restore");
    g_forceUnconsciousButton->setCaption("Force Unconscious");
    g_forcePlayingDeadButton->setCaption("Force Playing Dead");
    g_damageLeftArmButton->setCaption("Damage Left Arm");
    g_damageRightArmButton->setCaption("Damage Right Arm");
    g_damageLeftLegButton->setCaption("Damage Left Leg");
    g_damageRightLegButton->setCaption("Damage Right Leg");
    g_teleportSelectedToCameraButton->setCaption(std::string("Teleport Selected To ") + kTeleportDestinationLabel);
    g_moneyAmountEdit->setEditStatic(false);
    g_moneyAmountEdit->setMaxTextLength(10);
    g_moneyAmountEdit->setOnlyText("1000");
    g_addMoneyButton->setCaption("Add Money");
    g_itemCategoryDropdown->removeAllItems();
    g_itemCategoryDropdown->addItem("All");
    g_itemCategoryDropdown->addItem("Food");
    g_itemCategoryDropdown->addItem("General");
    g_itemCategoryDropdown->addItem("Armor");
    g_itemCategoryDropdown->addItem("Weapons");
    g_itemCategoryDropdown->setIndexSelected(0);
    g_itemSearchEdit->setEditStatic(false);
    g_itemSearchEdit->setMaxTextLength(48);
    g_itemSearchEdit->setOnlyText("");
    g_itemSearchResultsList->removeAllItems();
    g_itemSearchResultsList->clearIndexSelected();
    g_itemQuantityEdit->setEditStatic(false);
    g_itemQuantityEdit->setMaxTextLength(10);
    g_itemQuantityEdit->setOnlyText("1");
    g_spawnItemButton->setCaption("Spawn Item");
    EnsureInventoryFoodItemOptionsLoaded();
    RefreshInventoryFoodItemDropdown();
    UpdateForceDyingButtonCaption();
    UpdateCollapseButtonCaption();
    RefreshStatusWidget();
    SetActionButtonsEnabled(false);
    SetSelectionActionButtonsEnabled(false);

    g_collapseButton->eventMouseButtonClick += MyGUI::newDelegate(&OnCollapseButtonClicked);
    g_healthTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnHealthTabButtonPressed);
    g_teleportTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnTeleportTabButtonPressed);
    g_inventoryTabButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnInventoryTabButtonPressed);
    g_fullRestoreButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnFullRestoreButtonPressed);
    g_forceUnconsciousButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceUnconsciousButtonPressed);
    g_forcePlayingDeadButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForcePlayingDeadButtonPressed);
    g_damageLeftArmButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageLeftArmButtonPressed);
    g_damageRightArmButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageRightArmButtonPressed);
    g_damageLeftLegButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageLeftLegButtonPressed);
    g_damageRightLegButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnDamageRightLegButtonPressed);
    g_teleportSelectedToCameraButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnTeleportSelectedToCameraButtonPressed);
    g_addMoneyButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnAddMoneyButtonPressed);
    g_itemCategoryDropdown->eventComboChangePosition += MyGUI::newDelegate(&OnInventoryCategoryChanged);
    g_itemSearchEdit->eventEditTextChange += MyGUI::newDelegate(&OnInventoryItemSearchTextChanged);
    g_itemSearchResultsList->eventListMouseItemActivate += MyGUI::newDelegate(&OnInventorySearchResultsActivated);
    g_spawnItemButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnSpawnItemButtonPressed);
    g_forceDyingButton->eventMouseButtonPressed += MyGUI::newDelegate(&OnForceDyingButtonPressed);
    g_headerFrame->eventMouseButtonPressed += MyGUI::newDelegate(&OnHeaderMousePressed);
    g_headerFrame->eventMouseDrag += MyGUI::newDelegate(&OnHeaderMouseDrag);
    g_headerFrame->eventMouseMove += MyGUI::newDelegate(&OnHeaderMouseMove);
    g_headerFrame->eventMouseButtonReleased += MyGUI::newDelegate(&OnHeaderMouseReleased);

    TargetSnapshot snapshot;
    ResetTargetSnapshot(&snapshot);
    ApplyTargetSnapshotToUi(snapshot);
}

void CreatePanelWidgets()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (!gui)
    {
        return;
    }

    const MyGUI::IntCoord panelCoord = ResolvePanelCoord();

    g_panel = gui->createWidget<MyGUI::Widget>(
        "PanelEmpty",
        panelCoord,
        MyGUI::Align::Default,
        "Main");
    if (!g_panel)
    {
        g_panel = gui->createWidget<MyGUI::Widget>(
            "PanelEmpty",
            panelCoord,
            MyGUI::Align::Default,
            "Overlapped");
    }

    if (!g_panel)
    {
        if (!g_loggedPanelCreateFailure)
        {
            LogErrorLine("failed to create panel root");
            g_loggedPanelCreateFailure = true;
        }
        return;
    }

    g_headerFrame = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, 0, kPanelWidth, 38),
        MyGUI::Align::Default);
    g_headerTitleText = g_headerFrame->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(12, 8, kPanelWidth - 116, 22),
        MyGUI::Align::Default);
    g_collapseButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(kPanelWidth - 96, 4, 84, 30),
        MyGUI::Align::Default);
    g_bodyFrame = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(0, 38, kPanelWidth, kPanelExpandedHeight - 38),
        MyGUI::Align::Default);
    g_targetSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 52, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_targetNameText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 74, 156, 18),
        MyGUI::Align::Default);
    g_targetFactionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(184, 74, 156, 18),
        MyGUI::Align::Default);
    g_targetAlignmentText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 96, 156, 18),
        MyGUI::Align::Default);
    g_targetMembershipText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(184, 96, 156, 18),
        MyGUI::Align::Default);
    g_targetStateText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 118, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_noTargetText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 140, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_healthTabButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 170, 106, 28),
        MyGUI::Align::Default);
    g_teleportTabButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(127, 170, 106, 28),
        MyGUI::Align::Default);
    g_inventoryTabButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(234, 170, 106, 28),
        MyGUI::Align::Default);
    g_statesSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 208, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_fullRestoreButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 230, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_forceUnconsciousButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 264, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_forcePlayingDeadButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 298, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_limbDamageSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 332, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_damageLeftArmButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 354, 156, 28),
        MyGUI::Align::Default);
    g_damageRightArmButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 354, 156, 28),
        MyGUI::Align::Default);
    g_damageLeftLegButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 388, 156, 28),
        MyGUI::Align::Default);
    g_damageRightLegButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 388, 156, 28),
        MyGUI::Align::Default);
    g_teleportSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 208, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_teleportSelectedToCameraButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 230, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_inventorySectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 208, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_moneyAmountLabelText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 230, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_moneyAmountEdit = g_panel->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        MyGUI::IntCoord(20, 252, 156, 28),
        MyGUI::Align::Default);
    g_addMoneyButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 252, 156, 28),
        MyGUI::Align::Default);
    g_spawnFoodSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 296, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_itemCategoryLabelText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 318, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemCategoryDropdown = g_panel->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        MyGUI::IntCoord(20, 340, kPanelWidth - 40, 30),
        MyGUI::Align::Default);
    g_itemSearchLabelText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 376, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemSearchEdit = g_panel->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        MyGUI::IntCoord(20, 398, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_itemSearchResultsList = g_panel->createWidget<MyGUI::ListBox>(
        "Kenshi_ListBox",
        MyGUI::IntCoord(20, 432, kPanelWidth - 40, 84),
        MyGUI::Align::Default);
    g_itemDropdownLabelText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 522, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemDropdown = g_panel->createWidget<MyGUI::ComboBox>(
        "Kenshi_ComboBox",
        MyGUI::IntCoord(20, 544, kPanelWidth - 40, 30),
        MyGUI::Align::Default);
    g_itemQuantityLabelText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 580, kPanelWidth - 40, 18),
        MyGUI::Align::Default);
    g_itemQuantityEdit = g_panel->createWidget<MyGUI::EditBox>(
        "Kenshi_EditBox",
        MyGUI::IntCoord(20, 602, 156, 28),
        MyGUI::Align::Default);
    g_spawnItemButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(184, 602, 156, 28),
        MyGUI::Align::Default);
    g_dangerousSectionText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(14, 428, kPanelWidth - 28, 18),
        MyGUI::Align::Default);
    g_forceDyingButton = g_panel->createWidget<MyGUI::Button>(
        "Kenshi_Button1",
        MyGUI::IntCoord(20, 450, kPanelWidth - 40, 28),
        MyGUI::Align::Default);
    g_statusText = g_panel->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(20, 664, kPanelWidth - 40, 18),
        MyGUI::Align::Default);

    if (!HasAllPanelWidgets())
    {
        DestroyPanel();
        if (!g_loggedPanelCreateFailure)
        {
            LogErrorLine("failed to create panel widgets");
            g_loggedPanelCreateFailure = true;
        }
        return;
    }

    InitializePanelWidgets();
    ApplyPanelLayout();
    g_loggedPanelCreateFailure = false;
    LogInfoLine(std::string("event=testkit_panel_created visible=") + (!g_panelHidden ? "true" : "false"));
}

void EnsurePanel(PlayerInterface* thisptr)
{
    g_lastPlayerInterface = thisptr;

    if (!g_panel)
    {
        CreatePanelWidgets();
    }

    TickPanelDrag();
    TickForceDyingArmTimeout();
    UpdateCollapseButtonCaption();
    UpdateForceDyingButtonCaption();
    ApplyPanelLayout();

    if (!g_panelHidden && !g_panelCollapsed)
    {
        UpdateTargetInspection(thisptr);
    }
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
    ResetInventoryFoodItemOptions();
    ResetTargetSnapshot(&g_lastTargetSnapshot);
    g_hasLastTargetSnapshot = false;
}

void PlayerInterface_updateUT_hook(PlayerInterface* thisptr)
{
    PlayerInterface_updateUT_orig(thisptr);

    TickModHubAttachRetry();
    TickPanelToggleHotkey();
    EnsurePanel(thisptr);
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
        LogInfoLine("plugin disabled by config");
        return;
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
