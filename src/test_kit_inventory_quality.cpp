#include "test_kit_inventory_quality.h"

#include "test_kit_inventory.h"

#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/RootObjectFactory.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace test_kit
{
namespace
{
std::vector<InventorySpawnQualityOption> g_inventorySpawnQualityOptions;
std::string g_inventoryLastSelectedQualityLabelNormalized;
bool g_inventoryQualitySyncInProgress = false;

bool IsInventoryWeaponDataType(const GameData* itemData)
{
    return itemData && (itemData->type == WEAPON || itemData->type == CROSSBOW);
}

bool IsInventoryArmourDataType(const GameData* itemData)
{
    return itemData && itemData->type == ARMOUR;
}

bool IsInventoryQualitySelectableDataType(const GameData* itemData)
{
    return IsInventoryWeaponDataType(itemData) || IsInventoryArmourDataType(itemData);
}

std::string BuildInventoryQualityLabel(const GameData* data, const char* fallbackPrefix)
{
    if (!data)
    {
        return fallbackPrefix ? fallbackPrefix : "Default";
    }

    const std::string name = TrimAscii(data->name);
    if (!name.empty())
    {
        return name;
    }

    const std::string stringId = TrimAscii(data->stringID);
    if (!stringId.empty())
    {
        return stringId;
    }

    std::stringstream fallback;
    fallback << (fallbackPrefix ? fallbackPrefix : "Data") << " " << data->id;
    return fallback.str();
}

std::string NormalizeInventoryQualityLabel(const std::string& value)
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

GameData* GetInventorySpawnFirstListData(const GameData* itemData, const char* listName, itemType type)
{
    if (!itemData || !listName || !ou || !itemData->listExistsAndNotEmpty(listName))
    {
        return 0;
    }

    const std::string& sid = itemData->getFromList(listName, 0);
    if (sid.empty())
    {
        return 0;
    }

    return ou->gamedata.getData(sid, type);
}

GameData* GetInventorySpawnWeaponModelData(const GameData* itemData, const GameData* weaponManufacturerData)
{
    GameData* weaponGradeData =
        GetInventorySpawnFirstListData(weaponManufacturerData, "weapon models", MATERIAL_SPECS_WEAPON);
    if (weaponGradeData)
    {
        return weaponGradeData;
    }

    weaponGradeData = GetInventorySpawnFirstListData(itemData, "weapon models", MATERIAL_SPECS_WEAPON);
    if (weaponGradeData)
    {
        return weaponGradeData;
    }

    static GameData* defaultWeaponGradeData = 0;
    if (!defaultWeaponGradeData && ou)
    {
        defaultWeaponGradeData = ou->gamedata.getData("913-gamedata.base", MATERIAL_SPECS_WEAPON);
    }

    return defaultWeaponGradeData;
}

int GetInventorySpawnWeaponLevel(const GameData* weaponManufacturerData, const GameData* weaponModelData)
{
    if (weaponManufacturerData && weaponModelData)
    {
        const Ogre::vector<GameDataReference>::type* weaponModels =
            weaponManufacturerData->getReferenceListIfExists("weapon models");
        if (weaponModels)
        {
            for (Ogre::vector<GameDataReference>::type::const_iterator iter = weaponModels->begin();
                 iter != weaponModels->end();
                 ++iter)
            {
                if (iter->sid == weaponModelData->stringID)
                {
                    const int level = iter->values.value[0];
                    if (level > 0)
                    {
                        return level;
                    }
                }
            }
        }
    }

    return 40;
}

GameData* GetInventorySpawnWeaponManufacturerData(const GameData* itemData)
{
    if (!itemData || !ou)
    {
        return 0;
    }

    GameData* manufacturerData =
        GetInventorySpawnFirstListData(itemData, "weapon manufacturers", WEAPON_MANUFACTURER);
    if (manufacturerData)
    {
        return manufacturerData;
    }

    lektor<GameData*> referencingManufacturers;
    ou->gamedata.findAllDataThatReferencesThis(
        referencingManufacturers,
        const_cast<GameData*>(itemData),
        WEAPON_MANUFACTURER,
        "weapon types");
    if (referencingManufacturers.size() > 0 && referencingManufacturers[0])
    {
        return referencingManufacturers[0];
    }

    static GameData* defaultManufacturerData = 0;
    if (!defaultManufacturerData && ou)
    {
        defaultManufacturerData = ou->gamedata.getData("1057-gamedata.base", WEAPON_MANUFACTURER);
    }

    return defaultManufacturerData;
}

bool AreSameWeaponQualityTuple(
    GameData* leftManufacturer,
    GameData* leftModel,
    int leftLevel,
    GameData* rightManufacturer,
    GameData* rightModel,
    int rightLevel)
{
    return leftManufacturer == rightManufacturer
        && leftModel == rightModel
        && leftLevel == rightLevel;
}

void PopulateInventoryQualityDropdownDefaultState()
{
    g_inventorySpawnQualityOptions.clear();

    if (!g_itemQualityDropdown)
    {
        return;
    }

    g_inventoryQualitySyncInProgress = true;
    g_itemQualityDropdown->removeAllItems();
    g_itemQualityDropdown->addItem("Default");
    g_itemQualityDropdown->setIndexSelected(0);
    g_itemQualityDropdown->setEnabled(false);
    g_inventoryQualitySyncInProgress = false;
}

bool TryBuildDefaultWeaponQualitySelection(
    GameData* itemData,
    InventorySpawnWeaponQualitySelection* outSelection)
{
    if (!itemData || !outSelection)
    {
        return false;
    }

    outSelection->manufacturerData = GetInventorySpawnWeaponManufacturerData(itemData);
    outSelection->modelData = GetInventorySpawnWeaponModelData(itemData, outSelection->manufacturerData);
    outSelection->weaponLevel = GetInventorySpawnWeaponLevel(outSelection->manufacturerData, outSelection->modelData);
    outSelection->label = "Default";
    outSelection->usesDefaultBehavior = true;
    return true;
}

typedef boost::unordered::unordered_map<
    std::string,
    Ogre::vector<GameDataReference>::type,
    boost::hash<std::string>,
    std::equal_to<std::string>,
    Ogre::STLAllocator<
        std::pair<std::string const, Ogre::vector<GameDataReference>::type>,
        Ogre::GeneralAllocPolicy> >
    GameDataReferenceLists;

bool TryGetInventorySpawnArmourReferenceLevel(
    GameData* itemData,
    GameData* materialData,
    const std::string& listName,
    int* outLevel)
{
    if (outLevel)
    {
        *outLevel = 0;
    }

    if (!itemData || !materialData || !outLevel || !ou || !ou->theFactory)
    {
        return false;
    }

    const TripleInt values = ou->theFactory->getValsFromDataInList(itemData, materialData, listName);
    if (values.value[0] > 0)
    {
        *outLevel = values.value[0];
    }

    return true;
}

bool TryBuildDefaultArmourQualitySelection(
    GameData* itemData,
    InventorySpawnWeaponQualitySelection* outSelection)
{
    if (!itemData || !outSelection)
    {
        return false;
    }

    outSelection->manufacturerData = 0;
    outSelection->modelData = 0;
    outSelection->weaponLevel = 0;
    outSelection->label = "Default";
    outSelection->usesDefaultBehavior = true;

    if (!ou || !ou->theFactory)
    {
        return true;
    }

    for (GameDataReferenceLists::const_iterator iter = itemData->objectReferences.begin();
         iter != itemData->objectReferences.end();
         ++iter)
    {
        GameData* materialData = ou->theFactory->chooseDataFromList(
            itemData,
            iter->first,
            MATERIAL_SPECS_CLOTHING,
            0);
        if (!materialData)
        {
            continue;
        }

        int armourLevel = 0;
        TryGetInventorySpawnArmourReferenceLevel(itemData, materialData, iter->first, &armourLevel);

        outSelection->modelData = materialData;
        outSelection->weaponLevel = armourLevel;
        outSelection->label = BuildInventoryQualityLabel(materialData, "Quality");
        return true;
    }

    return true;
}

bool TryBuildDefaultInventoryQualitySelection(
    GameData* itemData,
    InventorySpawnWeaponQualitySelection* outSelection)
{
    if (IsInventoryWeaponDataType(itemData))
    {
        return TryBuildDefaultWeaponQualitySelection(itemData, outSelection);
    }

    if (IsInventoryArmourDataType(itemData))
    {
        return TryBuildDefaultArmourQualitySelection(itemData, outSelection);
    }

    if (!outSelection)
    {
        return false;
    }

    *outSelection = InventorySpawnWeaponQualitySelection();
    outSelection->label = "Default";
    return true;
}

bool HasManufacturerAlready(
    const std::vector<GameData*>& manufacturers,
    GameData* manufacturerData)
{
    for (size_t index = 0; index < manufacturers.size(); ++index)
    {
        if (manufacturers[index] == manufacturerData)
        {
            return true;
        }
    }

    return false;
}

void AddCandidateManufacturer(std::vector<GameData*>* manufacturers, GameData* manufacturerData)
{
    if (!manufacturers || !manufacturerData || HasManufacturerAlready(*manufacturers, manufacturerData))
    {
        return;
    }

    manufacturers->push_back(manufacturerData);
}

void CollectCandidateManufacturers(GameData* itemData, std::vector<GameData*>* outManufacturers)
{
    if (!itemData || !outManufacturers || !ou)
    {
        return;
    }

    if (itemData->listExistsAndNotEmpty("weapon manufacturers"))
    {
        const int listSize = itemData->getListSize("weapon manufacturers");
        for (int index = 0; index < listSize; ++index)
        {
            const std::string& sid = itemData->getFromList("weapon manufacturers", index);
            if (sid.empty())
            {
                continue;
            }

            AddCandidateManufacturer(outManufacturers, ou->gamedata.getData(sid, WEAPON_MANUFACTURER));
        }
    }

    lektor<GameData*> referencingManufacturers;
    ou->gamedata.findAllDataThatReferencesThis(
        referencingManufacturers,
        itemData,
        WEAPON_MANUFACTURER,
        "weapon types");
    for (lektor<GameData*>::const_iterator it = referencingManufacturers.begin();
         it != referencingManufacturers.end();
         ++it)
    {
        AddCandidateManufacturer(outManufacturers, *it);
    }
}

bool HasMatchingExplicitQualityOption(
    const std::vector<InventorySpawnQualityOption>& options,
    GameData* manufacturerData,
    GameData* modelData,
    int weaponLevel)
{
    for (size_t index = 0; index < options.size(); ++index)
    {
        const InventorySpawnQualityOption& option = options[index];
        if (AreSameWeaponQualityTuple(
                option.manufacturerData,
                option.modelData,
                option.weaponLevel,
                manufacturerData,
                modelData,
                weaponLevel))
        {
            return true;
        }
    }

    return false;
}

int GetInventorySpawnQualityOptionLevel(const GameDataReference& reference, int fallbackLevel)
{
    return reference.values.value[0] > 0 ? reference.values.value[0] : fallbackLevel;
}

void AppendManufacturerWeaponQualityOptions(
    GameData* manufacturerData,
    std::vector<InventorySpawnQualityOption>* outOptions)
{
    if (!manufacturerData || !outOptions)
    {
        return;
    }

    const Ogre::vector<GameDataReference>::type* weaponModels =
        manufacturerData->getReferenceListIfExists("weapon models");
    if (!weaponModels || !ou)
    {
        return;
    }

    for (Ogre::vector<GameDataReference>::type::const_iterator iter = weaponModels->begin();
         iter != weaponModels->end();
         ++iter)
    {
        if (iter->sid.empty())
        {
            continue;
        }

        GameData* modelData = ou->gamedata.getData(iter->sid, MATERIAL_SPECS_WEAPON);
        if (!modelData)
        {
            continue;
        }

        const int weaponLevel =
            GetInventorySpawnQualityOptionLevel(*iter, GetInventorySpawnWeaponLevel(manufacturerData, modelData));
        if (HasMatchingExplicitQualityOption(*outOptions, manufacturerData, modelData, weaponLevel))
        {
            continue;
        }

        InventorySpawnQualityOption option;
        option.label = BuildInventoryQualityLabel(modelData, "Quality");
        option.normalizedLabel = NormalizeInventoryQualityLabel(option.label);
        option.manufacturerData = manufacturerData;
        option.modelData = modelData;
        option.weaponLevel = weaponLevel;
        option.isDefault = false;
        outOptions->push_back(option);
    }
}

void AppendArmourQualityOptions(
    GameData* itemData,
    std::vector<InventorySpawnQualityOption>* outOptions)
{
    if (!itemData || !outOptions || !ou)
    {
        return;
    }

    for (GameDataReferenceLists::const_iterator listIter = itemData->objectReferences.begin();
         listIter != itemData->objectReferences.end();
         ++listIter)
    {
        const Ogre::vector<GameDataReference>::type& refs = listIter->second;
        for (Ogre::vector<GameDataReference>::type::const_iterator refIter = refs.begin();
             refIter != refs.end();
             ++refIter)
        {
            if (refIter->sid.empty())
            {
                continue;
            }

            GameData* materialData = ou->gamedata.getData(refIter->sid, MATERIAL_SPECS_CLOTHING);
            if (!materialData)
            {
                continue;
            }

            int armourLevel = 0;
            TryGetInventorySpawnArmourReferenceLevel(itemData, materialData, listIter->first, &armourLevel);
            armourLevel = GetInventorySpawnQualityOptionLevel(*refIter, armourLevel);
            if (HasMatchingExplicitQualityOption(*outOptions, 0, materialData, armourLevel))
            {
                continue;
            }

            InventorySpawnQualityOption option;
            option.label = BuildInventoryQualityLabel(materialData, "Quality");
            option.normalizedLabel = NormalizeInventoryQualityLabel(option.label);
            option.manufacturerData = 0;
            option.modelData = materialData;
            option.weaponLevel = armourLevel;
            option.isDefault = false;
            outOptions->push_back(option);
        }
    }
}

std::string BuildInventoryQualityDisambiguator(const InventorySpawnQualityOption& option)
{
    if (option.manufacturerData)
    {
        return BuildInventoryQualityLabel(option.manufacturerData, "Manufacturer");
    }

    if (option.weaponLevel > 0)
    {
        std::stringstream label;
        label << "Level " << option.weaponLevel;
        return label.str();
    }

    if (option.modelData)
    {
        const std::string sid = TrimAscii(option.modelData->stringID);
        if (!sid.empty())
        {
            return sid;
        }
    }

    return "";
}

void DisambiguateDuplicateQualityLabels(std::vector<InventorySpawnQualityOption>* options)
{
    if (!options)
    {
        return;
    }

    for (size_t leftIndex = 0; leftIndex < options->size(); ++leftIndex)
    {
        InventorySpawnQualityOption& left = (*options)[leftIndex];
        bool hasDuplicate = false;
        for (size_t rightIndex = 0; rightIndex < options->size(); ++rightIndex)
        {
            if (leftIndex == rightIndex)
            {
                continue;
            }

            const InventorySpawnQualityOption& right = (*options)[rightIndex];
            if (left.normalizedLabel == right.normalizedLabel)
            {
                hasDuplicate = true;
                break;
            }
        }

        if (!hasDuplicate)
        {
            continue;
        }

        const std::string disambiguator = BuildInventoryQualityDisambiguator(left);
        if (disambiguator.empty())
        {
            continue;
        }

        left.label += " (" + disambiguator + ")";
    }
}

size_t FindPreferredQualityOptionIndex(
    const std::vector<InventorySpawnQualityOption>& options,
    const InventorySpawnWeaponQualitySelection& defaultSelection)
{
    if (!g_inventoryLastSelectedQualityLabelNormalized.empty())
    {
        for (size_t index = 0; index < options.size(); ++index)
        {
            if (options[index].normalizedLabel == g_inventoryLastSelectedQualityLabelNormalized)
            {
                return index;
            }
        }
    }

    for (size_t index = 0; index < options.size(); ++index)
    {
        const InventorySpawnQualityOption& option = options[index];
        if (AreSameWeaponQualityTuple(
                option.manufacturerData,
                option.modelData,
                option.weaponLevel,
                defaultSelection.manufacturerData,
                defaultSelection.modelData,
                defaultSelection.weaponLevel))
        {
            return index;
        }
    }

    return options.empty() ? MyGUI::ITEM_NONE : 0u;
}
}

void ResetInventoryQualityRuntimeState()
{
    g_inventoryLastSelectedQualityLabelNormalized.clear();
    ResetInventoryQualityWidgetState();
}

void ResetInventoryQualityWidgetState()
{
    PopulateInventoryQualityDropdownDefaultState();
}

void RefreshInventoryQualityOptions()
{
    if (!g_itemQualityDropdown)
    {
        g_inventorySpawnQualityOptions.clear();
        return;
    }

    GameData* itemData = 0;
    if (!TryResolveSelectedInventoryItem(&itemData, 0) || !IsInventoryQualitySelectableDataType(itemData))
    {
        PopulateInventoryQualityDropdownDefaultState();
        return;
    }

    InventorySpawnWeaponQualitySelection defaultSelection;
    if (!TryBuildDefaultInventoryQualitySelection(itemData, &defaultSelection))
    {
        PopulateInventoryQualityDropdownDefaultState();
        return;
    }

    std::vector<InventorySpawnQualityOption> nextOptions;
    if (IsInventoryWeaponDataType(itemData))
    {
        std::vector<GameData*> candidateManufacturers;
        CollectCandidateManufacturers(itemData, &candidateManufacturers);
        for (size_t index = 0; index < candidateManufacturers.size(); ++index)
        {
            AppendManufacturerWeaponQualityOptions(candidateManufacturers[index], &nextOptions);
        }
    }
    else if (IsInventoryArmourDataType(itemData))
    {
        AppendArmourQualityOptions(itemData, &nextOptions);
    }

    DisambiguateDuplicateQualityLabels(&nextOptions);
    std::sort(
        nextOptions.begin(),
        nextOptions.end(),
        [](const InventorySpawnQualityOption& left, const InventorySpawnQualityOption& right) -> bool
        {
            if (left.weaponLevel != right.weaponLevel)
            {
                return left.weaponLevel < right.weaponLevel;
            }

            return left.label < right.label;
        });

    if (!HasMatchingExplicitQualityOption(
            nextOptions,
            defaultSelection.manufacturerData,
            defaultSelection.modelData,
            defaultSelection.weaponLevel))
    {
        InventorySpawnQualityOption defaultOption;
        defaultOption.label = "Default";
        defaultOption.normalizedLabel = NormalizeInventoryQualityLabel(defaultOption.label);
        defaultOption.manufacturerData = defaultSelection.manufacturerData;
        defaultOption.modelData = defaultSelection.modelData;
        defaultOption.weaponLevel = defaultSelection.weaponLevel;
        defaultOption.isDefault = true;
        nextOptions.insert(nextOptions.begin(), defaultOption);
    }

    if (nextOptions.empty())
    {
        InventorySpawnQualityOption defaultOption;
        defaultOption.label = "Default";
        defaultOption.normalizedLabel = NormalizeInventoryQualityLabel(defaultOption.label);
        defaultOption.manufacturerData = defaultSelection.manufacturerData;
        defaultOption.modelData = defaultSelection.modelData;
        defaultOption.weaponLevel = defaultSelection.weaponLevel;
        defaultOption.isDefault = true;
        nextOptions.push_back(defaultOption);
    }

    g_inventorySpawnQualityOptions.swap(nextOptions);

    const size_t preferredIndex =
        FindPreferredQualityOptionIndex(g_inventorySpawnQualityOptions, defaultSelection);
    const bool enableDropdown = g_inventorySpawnQualityOptions.size() > 1u;

    g_inventoryQualitySyncInProgress = true;
    g_itemQualityDropdown->removeAllItems();
    for (size_t index = 0; index < g_inventorySpawnQualityOptions.size(); ++index)
    {
        g_itemQualityDropdown->addItem(g_inventorySpawnQualityOptions[index].label);
    }
    g_itemQualityDropdown->setEnabled(enableDropdown);
    if (preferredIndex != MyGUI::ITEM_NONE && preferredIndex < g_inventorySpawnQualityOptions.size())
    {
        g_itemQualityDropdown->setIndexSelected(preferredIndex);
    }
    else
    {
        g_itemQualityDropdown->clearIndexSelected();
    }
    g_inventoryQualitySyncInProgress = false;
}

void OnInventoryQualityChanged(MyGUI::ComboBox*, size_t)
{
    if (g_inventoryQualitySyncInProgress || !g_itemQualityDropdown)
    {
        return;
    }

    const size_t selectedIndex = g_itemQualityDropdown->getIndexSelected();
    if (selectedIndex >= g_inventorySpawnQualityOptions.size())
    {
        g_inventoryLastSelectedQualityLabelNormalized.clear();
        return;
    }

    const InventorySpawnQualityOption& option = g_inventorySpawnQualityOptions[selectedIndex];
    if (option.isDefault)
    {
        g_inventoryLastSelectedQualityLabelNormalized.clear();
        return;
    }

    g_inventoryLastSelectedQualityLabelNormalized = option.normalizedLabel;
}

bool TryResolveSelectedInventoryWeaponQuality(
    GameData* itemData,
    InventorySpawnWeaponQualitySelection* outSelection)
{
    if (!outSelection)
    {
        return false;
    }

    *outSelection = InventorySpawnWeaponQualitySelection();
    outSelection->label = "Default";

    if (!IsInventoryQualitySelectableDataType(itemData))
    {
        return true;
    }

    InventorySpawnWeaponQualitySelection defaultSelection;
    if (!TryBuildDefaultInventoryQualitySelection(itemData, &defaultSelection))
    {
        return true;
    }

    *outSelection = defaultSelection;

    if (!g_itemQualityDropdown)
    {
        return true;
    }

    const size_t selectedIndex = g_itemQualityDropdown->getIndexSelected();
    if (selectedIndex >= g_inventorySpawnQualityOptions.size())
    {
        return true;
    }

    const InventorySpawnQualityOption& option = g_inventorySpawnQualityOptions[selectedIndex];
    outSelection->label = option.label;
    outSelection->manufacturerData = option.manufacturerData;
    outSelection->modelData = option.modelData;
    outSelection->weaponLevel = option.weaponLevel;
    outSelection->usesDefaultBehavior = AreSameWeaponQualityTuple(
        option.manufacturerData,
        option.modelData,
        option.weaponLevel,
        defaultSelection.manufacturerData,
        defaultSelection.modelData,
        defaultSelection.weaponLevel);
    return true;
}
}
