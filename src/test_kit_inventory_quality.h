#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
struct InventorySpawnQualityOption
{
    InventorySpawnQualityOption()
        : manufacturerData(0)
        , modelData(0)
        , weaponLevel(0)
        , isDefault(false)
    {
    }

    std::string label;
    std::string normalizedLabel;
    GameData* manufacturerData;
    GameData* modelData;
    int weaponLevel;
    bool isDefault;
};

struct InventorySpawnWeaponQualitySelection
{
    InventorySpawnWeaponQualitySelection()
        : manufacturerData(0)
        , modelData(0)
        , weaponLevel(0)
        , usesDefaultBehavior(true)
    {
    }

    std::string label;
    GameData* manufacturerData;
    GameData* modelData;
    int weaponLevel;
    bool usesDefaultBehavior;
};

void ResetInventoryQualityRuntimeState();
void ResetInventoryQualityWidgetState();
void RefreshInventoryQualityOptions();
void OnInventoryQualityChanged(MyGUI::ComboBox*, size_t);
bool TryResolveSelectedInventoryWeaponQuality(
    GameData* itemData,
    InventorySpawnWeaponQualitySelection* outSelection);
}
