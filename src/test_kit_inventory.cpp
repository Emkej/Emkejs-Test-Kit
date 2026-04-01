#include "test_kit_inventory.h"

#include <core/Functions.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/InputHandler.h>
#include <kenshi/Item.h>
#include <kenshi/RootObjectFactory.h>
#include <mygui/MyGUI_InputManager.h>
#include <ois/OISKeyboard.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

namespace test_kit
{
const int kInventoryItemDropdownMaxListLength = 224;

namespace
{
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

std::vector<InventorySpawnOption> g_inventoryFoodItemOptions;
std::vector<size_t> g_filteredInventoryFoodItemOptionIndexes;
bool g_inventoryFoodItemOptionsLoaded = false;
bool g_inventorySearchCtrlFPrevDown = false;
PendingInventorySearchShortcut g_pendingInventorySearchShortcut;
bool g_haveInventorySearchEditSnapshot = false;
InventorySearchSnapshot g_inventorySearchEditSnapshot;

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

void ResetPendingInventorySearchShortcut()
{
    g_pendingInventorySearchShortcut = PendingInventorySearchShortcut();
}

void ResetInventorySearchEditSnapshot()
{
    g_haveInventorySearchEditSnapshot = false;
    g_inventorySearchEditSnapshot = InventorySearchSnapshot();
}

void ResetInventoryFoodItemOptions()
{
    g_inventoryFoodItemOptions.clear();
    g_filteredInventoryFoodItemOptionIndexes.clear();
    g_inventoryFoodItemOptionsLoaded = false;
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

    std::stringstream line;
    line << "inventory search focused";
    if (reason)
    {
        line << " reason=\"" << reason << "\"";
    }
    LogDebugLine(line.str());
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
}

void ResetInventoryRuntimeState()
{
    g_inventorySearchCtrlFPrevDown = false;
    ResetInventoryWidgetInteractionState();
    ResetInventoryFoodItemOptions();
}

void ResetInventoryWidgetInteractionState()
{
    ResetPendingInventorySearchShortcut();
    ResetInventorySearchEditSnapshot();
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
}
