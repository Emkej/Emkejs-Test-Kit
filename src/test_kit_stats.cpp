#include "test_kit_stats.h"

#include <mygui/MyGUI_InputManager.h>

#include <sstream>

namespace test_kit
{
namespace
{
const int kStatsSafeValueMin = 0;
const int kStatsSafeValueMax = 100;

const StatsRegistryEntry kStatsRegistry[] = {
    { STAT_STRENGTH, "Strength", StatsGroup_Core, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_TOUGHNESS, "Toughness", StatsGroup_Core, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_DEXTERITY, "Dexterity", StatsGroup_Core, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_PERCEPTION, "Perception", StatsGroup_Core, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_ATHLETICS, "Athletics", StatsGroup_MovementUtility, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_MELEE_ATTACK, "Melee Attack", StatsGroup_Combat, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_MELEE_DEFENCE, "Melee Defense", StatsGroup_Combat, true, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_STEALTH, "Stealth", StatsGroup_MovementUtility, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_LOCKPICKING, "Lockpicking", StatsGroup_MovementUtility, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_THIEVING, "Thievery", StatsGroup_MovementUtility, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_ASSASSINATION, "Assassination", StatsGroup_MovementUtility, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_SWIMMING, "Swimming", StatsGroup_MovementUtility, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_MARTIALARTS, "Martial Arts", StatsGroup_Combat, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_DODGE, "Dodge", StatsGroup_Combat, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_TURRETS, "Turrets", StatsGroup_Combat, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_FRIENDLY_FIRE, "Precision Shooting", StatsGroup_Combat, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_SABRES, "Sabres", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_KATANAS, "Katanas", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_HACKERS, "Hackers", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_HEAVYWEAPONS, "Heavy Weapons", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_BLUNT, "Blunt", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_POLEARMS, "Polearms", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_CROSSBOWS, "Crossbows", StatsGroup_WeaponSkills, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_LABOURING, "Labouring", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_MEDIC, "Field Medic", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_SMITHING_WEAPON, "Weapon Smithing", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_SMITHING_ARMOUR, "Armour Smithing", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_SMITHING_BOW, "Crossbow Smithing", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_FARMING, "Farming", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_COOKING, "Cooking", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_ROBOTICS, "Robotics", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_SCIENCE, "Science", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax },
    { STAT_ENGINEERING, "Engineering", StatsGroup_Labor, false, kStatsSafeValueMin, kStatsSafeValueMax }
};

size_t GetStatsRegistryCount()
{
    return sizeof(kStatsRegistry) / sizeof(kStatsRegistry[0]);
}

const StatsRegistryEntry* FindStatsRegistryEntry(StatsEnumerated stat)
{
    for (size_t index = 0; index < GetStatsRegistryCount(); ++index)
    {
        if (kStatsRegistry[index].stat == stat)
        {
            return &kStatsRegistry[index];
        }
    }

    return 0;
}

int ClampStatsValue(const StatsRegistryEntry& entry, int value)
{
    return ClampIntValue(value, entry.safeMinValue, entry.safeMaxValue);
}

const char* GetStatsGroupLabel(StatsGroup group)
{
    switch (group)
    {
    case StatsGroup_Core:
        return "Core";
    case StatsGroup_MovementUtility:
        return "Movement / Utility";
    case StatsGroup_Combat:
        return "Combat";
    case StatsGroup_WeaponSkills:
        return "Weapon Skills";
    case StatsGroup_Labor:
        return "Labor";
    default:
        break;
    }

    return "Unknown";
}

const char* GetStatsSectionFilterLabel(StatsSectionFilter filter)
{
    switch (filter)
    {
    case StatsSectionFilter_All:
        return "All";
    case StatsSectionFilter_CommonTest:
        return "Common";
    case StatsSectionFilter_Core:
        return "Core";
    case StatsSectionFilter_MovementUtility:
        return "Movement / Utility";
    case StatsSectionFilter_Combat:
        return "Combat";
    case StatsSectionFilter_WeaponSkills:
        return "Weapon Skills";
    case StatsSectionFilter_Labor:
        return "Labor";
    default:
        break;
    }

    return "Unknown";
}

const char* GetStatsSectionButtonLabel(StatsSectionFilter filter)
{
    switch (filter)
    {
    case StatsSectionFilter_All:
        return "All";
    case StatsSectionFilter_CommonTest:
        return "Common";
    case StatsSectionFilter_Core:
        return "Core";
    case StatsSectionFilter_MovementUtility:
        return "Utility";
    case StatsSectionFilter_Combat:
        return "Combat";
    case StatsSectionFilter_WeaponSkills:
        return "Weapons";
    case StatsSectionFilter_Labor:
        return "Labor";
    default:
        break;
    }

    return "Unknown";
}

bool DoesStatsEntryMatchSectionFilter(const StatsRegistryEntry& entry, StatsSectionFilter filter)
{
    switch (filter)
    {
    case StatsSectionFilter_All:
        return true;
    case StatsSectionFilter_CommonTest:
        return entry.commonTestStat;
    case StatsSectionFilter_Core:
        return entry.group == StatsGroup_Core;
    case StatsSectionFilter_MovementUtility:
        return entry.group == StatsGroup_MovementUtility;
    case StatsSectionFilter_Combat:
        return entry.group == StatsGroup_Combat;
    case StatsSectionFilter_WeaponSkills:
        return entry.group == StatsGroup_WeaponSkills;
    case StatsSectionFilter_Labor:
        return entry.group == StatsGroup_Labor;
    default:
        break;
    }

    return false;
}

void UpdateStatsSectionButtonCaption(MyGUI::Button* button, StatsSectionFilter filter)
{
    if (!button)
    {
        return;
    }

    const char* label = GetStatsSectionButtonLabel(filter);
    if (g_activeStatsSectionFilter == filter)
    {
        button->setCaption(std::string("[") + label + "]");
        return;
    }

    button->setCaption(label);
}

int RoundStatValueToInt(float value)
{
    if (value >= 0.0f)
    {
        return static_cast<int>(value + 0.5f);
    }

    return static_cast<int>(value - 0.5f);
}

std::string BuildStatsListLabel(const StatsRegistryEntry& entry)
{
    std::stringstream label;
    label << entry.label;
    const char* groupLabel = GetStatsGroupLabel(entry.group);
    if (entry.commonTestStat)
    {
        label << " [Common, " << groupLabel << "]";
    }
    else if (groupLabel && groupLabel[0] != '\0')
    {
        label << " [" << groupLabel << "]";
    }

    return label.str();
}

std::string BuildStatsSectionedListLabel(const StatsRegistryEntry& entry, bool includeGroupingHint)
{
    if (!includeGroupingHint)
    {
        return entry.label;
    }

    return BuildStatsListLabel(entry);
}

std::string BuildStatsSummaryLabel(const StatsRegistryEntry& entry)
{
    std::stringstream label;
    label << entry.label;
    const char* groupLabel = GetStatsGroupLabel(entry.group);
    if (entry.commonTestStat)
    {
        label << " (Common / " << groupLabel << ")";
    }
    else if (groupLabel && groupLabel[0] != '\0')
    {
        label << " (" << groupLabel << ")";
    }

    return label.str();
}

bool DoesStatsEntryMatchSearch(const StatsRegistryEntry& entry, const std::string& searchUpper)
{
    if (searchUpper.empty())
    {
        return true;
    }

    std::stringstream searchable;
    searchable << entry.label;
    const char* groupLabel = GetStatsGroupLabel(entry.group);
    if (groupLabel && groupLabel[0] != '\0')
    {
        searchable << ' ' << groupLabel;
    }
    if (entry.commonTestStat)
    {
        searchable << " Common";
    }

    return ToUpperAscii(searchable.str()).find(searchUpper) != std::string::npos;
}

std::string BuildStatsSectionAllListLabel(StatsSectionFilter section)
{
    switch (section)
    {
    case StatsSectionFilter_CommonTest:
        return "All Common test stats";
    case StatsSectionFilter_Core:
        return "All Core stats";
    case StatsSectionFilter_MovementUtility:
        return "All Movement / Utility stats";
    case StatsSectionFilter_Combat:
        return "All Combat stats";
    case StatsSectionFilter_WeaponSkills:
        return "All Weapon Skills stats";
    case StatsSectionFilter_Labor:
        return "All Labor stats";
    case StatsSectionFilter_All:
    default:
        break;
    }

    return "All stats";
}

bool IsStatsSectionBatchSelectionActive(StatsSectionFilter* sectionOut)
{
    if (sectionOut)
    {
        *sectionOut = StatsSectionFilter_All;
    }

    if (!g_statsResultsList)
    {
        return false;
    }

    const size_t selectedIndex = g_statsResultsList->getIndexSelected();
    if (selectedIndex >= g_filteredStatsRegistryIndexes.size())
    {
        return false;
    }

    if (g_filteredStatsRegistryIndexes[selectedIndex] != -1)
    {
        return false;
    }

    if (sectionOut)
    {
        *sectionOut = g_activeStatsSectionFilter;
    }
    return true;
}

size_t CountStatsForSectionFilter(StatsSectionFilter section)
{
    size_t count = 0u;
    for (size_t index = 0; index < GetStatsRegistryCount(); ++index)
    {
        if (DoesStatsEntryMatchSectionFilter(kStatsRegistry[index], section))
        {
            ++count;
        }
    }

    return count;
}

void CollectStatsEntriesForSectionFilter(StatsSectionFilter section, std::vector<const StatsRegistryEntry*>* outEntries)
{
    if (!outEntries)
    {
        return;
    }

    outEntries->clear();
    for (size_t index = 0; index < GetStatsRegistryCount(); ++index)
    {
        if (DoesStatsEntryMatchSectionFilter(kStatsRegistry[index], section))
        {
            outEntries->push_back(&kStatsRegistry[index]);
        }
    }
}

bool TryResolveSelectedStatsEntry(const StatsRegistryEntry** entryOut)
{
    if (entryOut)
    {
        *entryOut = 0;
    }

    StatsEnumerated selectedStat = g_selectedStatsStat;
    if (g_statsResultsList)
    {
        const size_t selectedIndex = g_statsResultsList->getIndexSelected();
        if (selectedIndex < g_filteredStatsRegistryIndexes.size())
        {
            const int registryIndex = g_filteredStatsRegistryIndexes[selectedIndex];
            if (registryIndex >= 0 && static_cast<size_t>(registryIndex) < GetStatsRegistryCount())
            {
                selectedStat = kStatsRegistry[registryIndex].stat;
            }
            else
            {
                selectedStat = STAT_NONE;
            }
        }
    }

    const StatsRegistryEntry* entry = FindStatsRegistryEntry(selectedStat);
    if (!entry)
    {
        return false;
    }

    if (entryOut)
    {
        *entryOut = entry;
    }
    return true;
}

bool TryReadCharacterStatValue(Character* character, StatsEnumerated stat, int* outValue)
{
    if (!character || !character->stats || !outValue)
    {
        return false;
    }

    __try
    {
        *outValue = RoundStatValueToInt(character->stats->getStat(stat, true));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool HasStatsClipboardData()
{
    return !g_statsClipboardEntries.empty();
}

std::string BuildStatsClipboardSummaryText()
{
    if (!HasStatsClipboardData())
    {
        return "Clipboard: Empty";
    }

    std::stringstream summary;
    summary << "Clipboard: " << g_statsClipboardEntries.size() << " stats";
    if (!g_statsClipboardSourceName.empty())
    {
        summary << " from " << g_statsClipboardSourceName;
    }
    return summary.str();
}

bool TryCopyStatsBlockFromCharacter(
    Character* character,
    std::string* sourceNameOut,
    int* copiedCountOut,
    int* attemptedCountOut)
{
    if (sourceNameOut)
    {
        sourceNameOut->clear();
    }
    if (copiedCountOut)
    {
        *copiedCountOut = 0;
    }
    if (attemptedCountOut)
    {
        *attemptedCountOut = 0;
    }

    if (!character)
    {
        return false;
    }

    std::vector<StatsClipboardEntry> copiedEntries;
    copiedEntries.reserve(GetStatsRegistryCount());

    for (size_t index = 0; index < GetStatsRegistryCount(); ++index)
    {
        const StatsRegistryEntry& entry = kStatsRegistry[index];
        int value = 0;
        if (attemptedCountOut)
        {
            ++(*attemptedCountOut);
        }

        if (!TryReadCharacterStatValue(character, entry.stat, &value))
        {
            continue;
        }

        StatsClipboardEntry clipboardEntry;
        clipboardEntry.stat = entry.stat;
        clipboardEntry.value = value;
        copiedEntries.push_back(clipboardEntry);
    }

    if (copiedEntries.empty())
    {
        return false;
    }

    g_statsClipboardEntries.swap(copiedEntries);
    g_statsClipboardSourceName = SafeCharacterName(character);
    if (g_statsClipboardSourceName.empty())
    {
        g_statsClipboardSourceName = "selected character";
    }

    if (sourceNameOut)
    {
        *sourceNameOut = g_statsClipboardSourceName;
    }
    if (copiedCountOut)
    {
        *copiedCountOut = static_cast<int>(g_statsClipboardEntries.size());
    }

    return true;
}

const char* StatsEditOperationToActionId(StatsEditOperation operation)
{
    switch (operation)
    {
    case StatsEditOperation_Add:
        return "add_stat";
    case StatsEditOperation_Subtract:
        return "subtract_stat";
    case StatsEditOperation_Set:
    default:
        return "set_stat";
    }
}

const char* StatsSectionEditOperationToActionId(StatsEditOperation operation)
{
    switch (operation)
    {
    case StatsEditOperation_Add:
        return "add_stat_section";
    case StatsEditOperation_Subtract:
        return "subtract_stat_section";
    case StatsEditOperation_Set:
    default:
        return "set_stat_section";
    }
}

const char* StatsEditOperationToStatusVerb(StatsEditOperation operation)
{
    switch (operation)
    {
    case StatsEditOperation_Add:
        return "Added";
    case StatsEditOperation_Subtract:
        return "Subtracted";
    case StatsEditOperation_Set:
    default:
        return "Set";
    }
}

const char* StatsEditOperationToStatusPreposition(StatsEditOperation operation)
{
    return operation == StatsEditOperation_Subtract ? "from" : "to";
}

int ResolveStatsEditResultValue(
    const StatsRegistryEntry& entry,
    StatsEditOperation operation,
    int currentValue,
    int inputValue)
{
    switch (operation)
    {
    case StatsEditOperation_Add:
        return ClampStatsValue(entry, currentValue + inputValue);
    case StatsEditOperation_Subtract:
        return ClampStatsValue(entry, currentValue - inputValue);
    case StatsEditOperation_Set:
    default:
        return ClampStatsValue(entry, inputValue);
    }
}

bool TryApplyStatsEditToCharacter(
    Character* character,
    const StatsRegistryEntry& entry,
    StatsEditOperation operation,
    int inputValue,
    int* beforeValueOut,
    int* afterValueOut,
    bool* clampedOut)
{
    if (!character || !character->stats)
    {
        return false;
    }

    if (beforeValueOut)
    {
        *beforeValueOut = 0;
    }
    if (afterValueOut)
    {
        *afterValueOut = 0;
    }
    if (clampedOut)
    {
        *clampedOut = false;
    }

    __try
    {
        CharStats* stats = character->stats;
        const int beforeValue = RoundStatValueToInt(stats->getStat(entry.stat, true));
        const int rawTargetValue =
            operation == StatsEditOperation_Set
                ? inputValue
                : (operation == StatsEditOperation_Add ? beforeValue + inputValue : beforeValue - inputValue);
        const int afterValue = ResolveStatsEditResultValue(entry, operation, beforeValue, inputValue);
        float& statRef = stats->getStatRef(entry.stat);
        statRef = static_cast<float>(afterValue);

        if (beforeValueOut)
        {
            *beforeValueOut = beforeValue;
        }
        if (afterValueOut)
        {
            *afterValueOut = afterValue;
        }
        if (clampedOut)
        {
            *clampedOut = rawTargetValue != afterValue;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TryApplyStatsClipboardToCharacter(
    Character* character,
    int* appliedCountOut,
    bool* anyClampedOut)
{
    if (appliedCountOut)
    {
        *appliedCountOut = 0;
    }
    if (anyClampedOut)
    {
        *anyClampedOut = false;
    }

    if (!character || g_statsClipboardEntries.empty())
    {
        return false;
    }

    int appliedCount = 0;
    bool anyClamped = false;

    for (size_t index = 0; index < g_statsClipboardEntries.size(); ++index)
    {
        const StatsClipboardEntry& clipboardEntry = g_statsClipboardEntries[index];
        const StatsRegistryEntry* entry = FindStatsRegistryEntry(clipboardEntry.stat);
        if (!entry)
        {
            continue;
        }

        int beforeValue = 0;
        int afterValue = 0;
        bool clamped = false;
        if (!TryApplyStatsEditToCharacter(
                character,
                *entry,
                StatsEditOperation_Set,
                clipboardEntry.value,
                &beforeValue,
                &afterValue,
                &clamped))
        {
            continue;
        }

        ++appliedCount;
        if (clamped)
        {
            anyClamped = true;
        }
    }

    if (appliedCountOut)
    {
        *appliedCountOut = appliedCount;
    }
    if (anyClampedOut)
    {
        *anyClampedOut = anyClamped;
    }

    return appliedCount > 0;
}

bool AddUniqueStatsTarget(std::vector<Character*>* targets, Character* character)
{
    if (!targets || !character)
    {
        return false;
    }

    for (size_t index = 0; index < targets->size(); ++index)
    {
        if ((*targets)[index] == character)
        {
            return false;
        }
    }

    targets->push_back(character);
    return true;
}

bool TryCollectStatsTargets(PlayerInterface* player, bool applyToAllSelected, std::vector<Character*>* outTargets)
{
    if (!outTargets)
    {
        return false;
    }

    outTargets->clear();
    if (!player)
    {
        return false;
    }

    if (!applyToAllSelected)
    {
        Character* primary = TryGetPrimarySelectedCharacter(player);
        if (primary)
        {
            outTargets->push_back(primary);
        }
        return !outTargets->empty();
    }

    __try
    {
        const ogre_unordered_set<hand>::type& selectedCharacters = player->selectedCharacters;
        ogre_unordered_set<hand>::type::const_iterator it = selectedCharacters.begin();
        for (; it != selectedCharacters.end(); ++it)
        {
            AddUniqueStatsTarget(outTargets, it->getCharacter());
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        outTargets->clear();
    }

    AddUniqueStatsTarget(outTargets, TryGetPrimarySelectedCharacter(player));
    return !outTargets->empty();
}

void RefreshStatsActionButtons(PlayerInterface* player)
{
    const int selectedCount = GetSelectedCharacterCount(player);
    if (selectedCount <= 1)
    {
        g_statsApplyToAllSelected = false;
    }

    if (g_statsApplyToAllButton)
    {
        g_statsApplyToAllButton->setCaption(
            std::string("Apply To All Selected: ") + (g_statsApplyToAllSelected ? "On" : "Off"));
        g_statsApplyToAllButton->setEnabled(selectedCount > 1);
    }

    const bool hasPrimarySelectedCharacter = HasPrimarySelectedCharacter(player);
    const bool hasTargets = g_statsApplyToAllSelected ? (selectedCount > 0) : hasPrimarySelectedCharacter;
    const bool hasClipboardData = HasStatsClipboardData();

    const StatsRegistryEntry* entry = 0;
    const bool hasSelectedStat = TryResolveSelectedStatsEntry(&entry) && entry != 0;
    const bool hasSectionBatchTarget = IsStatsSectionBatchSelectionActive(0);
    int inputValue = 0;
    const bool hasValidInput =
        g_statsInputEdit && TryParseNonNegativeInt(TrimAscii(g_statsInputEdit->getOnlyText().asUTF8()), &inputValue);
    const bool setEnabled = hasTargets && (hasSelectedStat || hasSectionBatchTarget) && hasValidInput;
    const bool deltaEnabled = setEnabled && inputValue > 0;

    if (g_statsSetButton)
    {
        g_statsSetButton->setEnabled(setEnabled);
    }
    if (g_statsAddButton)
    {
        g_statsAddButton->setEnabled(deltaEnabled);
    }
    if (g_statsSubtractButton)
    {
        g_statsSubtractButton->setEnabled(deltaEnabled);
    }
    if (g_statsCopyButton)
    {
        g_statsCopyButton->setEnabled(hasPrimarySelectedCharacter);
    }
    if (g_statsPasteButton)
    {
        g_statsPasteButton->setEnabled(selectedCount > 0 && hasClipboardData);
    }
}
} // namespace

void UpdateStatsSectionButtonCaptions()
{
    UpdateStatsSectionButtonCaption(g_statsAllSectionButton, StatsSectionFilter_All);
    UpdateStatsSectionButtonCaption(g_statsCommonSectionButton, StatsSectionFilter_CommonTest);
    UpdateStatsSectionButtonCaption(g_statsCoreSectionButton, StatsSectionFilter_Core);
    UpdateStatsSectionButtonCaption(g_statsUtilitySectionButton, StatsSectionFilter_MovementUtility);
    UpdateStatsSectionButtonCaption(g_statsCombatSectionButton, StatsSectionFilter_Combat);
    UpdateStatsSectionButtonCaption(g_statsWeaponsSectionButton, StatsSectionFilter_WeaponSkills);
    UpdateStatsSectionButtonCaption(g_statsLaborSectionButton, StatsSectionFilter_Labor);
}

void RefreshStatsUi(PlayerInterface* player)
{
    const int selectedCount = GetSelectedCharacterCount(player);
    const bool hasPrimarySelectedCharacter = HasPrimarySelectedCharacter(player);

    if (g_statsClipboardText)
    {
        g_statsClipboardText->setCaption(BuildStatsClipboardSummaryText());
    }

    if (g_statsScopeText)
    {
        if (!hasPrimarySelectedCharacter && selectedCount <= 0)
        {
            g_statsScopeText->setCaption("Scope: No selected character");
        }
        else if (g_statsApplyToAllSelected && selectedCount > 1)
        {
            std::stringstream caption;
            caption << "Scope: All selected characters (" << selectedCount << ")";
            g_statsScopeText->setCaption(caption.str());
        }
        else
        {
            g_statsScopeText->setCaption("Scope: Current selected character");
        }
    }

    StatsSectionFilter batchSection = StatsSectionFilter_All;
    if (IsStatsSectionBatchSelectionActive(&batchSection))
    {
        if (g_statsSelectedSummaryText)
        {
            g_statsSelectedSummaryText->setCaption(
                std::string("Selected: ") + BuildStatsSectionAllListLabel(batchSection) + " (section batch)");
        }
        if (g_statsCurrentValueText)
        {
            std::stringstream caption;
            caption << "Current: Applies to " << CountStatsForSectionFilter(batchSection)
                    << " stats per selected character";
            if (g_statsApplyToAllSelected && selectedCount > 1)
            {
                caption << " | " << selectedCount << " selected";
            }
            g_statsCurrentValueText->setCaption(caption.str());
        }
        if (g_statsInputLabelText)
        {
            g_statsInputLabelText->setCaption("Value / Delta (0-100, section batch)");
        }
        if (g_statsPreviewText)
        {
            int inputValue = 0;
            if (!g_statsInputEdit
                || !TryParseNonNegativeInt(TrimAscii(g_statsInputEdit->getOnlyText().asUTF8()), &inputValue))
            {
                g_statsPreviewText->setCaption("Preview: Enter a non-negative value");
            }
            else
            {
                std::stringstream preview;
                preview << "Preview: Set " << BuildStatsSectionAllListLabel(batchSection)
                        << " to " << ClampIntValue(inputValue, kStatsSafeValueMin, kStatsSafeValueMax)
                        << " | Add " << inputValue << " to each"
                        << " | Subtract " << inputValue << " from each";
                g_statsPreviewText->setCaption(preview.str());
            }
        }
        RefreshStatsActionButtons(player);
        return;
    }

    const StatsRegistryEntry* entry = 0;
    if (!TryResolveSelectedStatsEntry(&entry) || !entry)
    {
        if (g_statsSelectedSummaryText)
        {
            g_statsSelectedSummaryText->setCaption("Selected: None");
        }
        if (g_statsCurrentValueText)
        {
            g_statsCurrentValueText->setCaption("Current: No stat selected");
        }
        if (g_statsInputLabelText)
        {
            g_statsInputLabelText->setCaption("Value / Delta");
        }
        if (g_statsPreviewText)
        {
            g_statsPreviewText->setCaption("Preview: Select a stat");
        }
        RefreshStatsActionButtons(player);
        return;
    }

    if (g_statsSelectedSummaryText)
    {
        g_statsSelectedSummaryText->setCaption("Selected: " + BuildStatsSummaryLabel(*entry));
    }

    if (g_statsInputLabelText)
    {
        std::stringstream caption;
        caption << "Value / Delta (" << entry->safeMinValue << "-" << entry->safeMaxValue << ")";
        g_statsInputLabelText->setCaption(caption.str());
    }

    int currentValue = 0;
    const bool hasCurrentValue =
        hasPrimarySelectedCharacter && TryReadCharacterStatValue(TryGetPrimarySelectedCharacter(player), entry->stat, &currentValue);
    if (g_statsCurrentValueText)
    {
        if (!hasCurrentValue)
        {
            g_statsCurrentValueText->setCaption("Current: No selected character");
        }
        else if (g_statsApplyToAllSelected && selectedCount > 1)
        {
            std::stringstream caption;
            caption << "Current (primary): " << currentValue << " | " << selectedCount << " selected";
            g_statsCurrentValueText->setCaption(caption.str());
        }
        else
        {
            std::stringstream caption;
            caption << "Current: " << currentValue;
            g_statsCurrentValueText->setCaption(caption.str());
        }
    }

    if (g_statsPreviewText)
    {
        int inputValue = 0;
        if (!hasCurrentValue)
        {
            g_statsPreviewText->setCaption("Preview: Select a character");
        }
        else if (!g_statsInputEdit
            || !TryParseNonNegativeInt(TrimAscii(g_statsInputEdit->getOnlyText().asUTF8()), &inputValue))
        {
            g_statsPreviewText->setCaption("Preview: Enter a non-negative value");
        }
        else
        {
            const int setValue = ClampStatsValue(*entry, inputValue);
            const int addValue = ClampStatsValue(*entry, currentValue + inputValue);
            const int subtractValue = ClampStatsValue(*entry, currentValue - inputValue);

            std::stringstream preview;
            if (g_statsApplyToAllSelected && selectedCount > 1)
            {
                preview << "Preview (primary): ";
            }
            else
            {
                preview << "Preview: ";
            }
            preview << "from " << currentValue
                    << " -> Set " << setValue
                    << " | + " << addValue
                    << " | - " << subtractValue;
            g_statsPreviewText->setCaption(preview.str());
        }
    }

    RefreshStatsActionButtons(player);
}

void RefreshStatsList()
{
    UpdateStatsSectionButtonCaptions();

    if (!g_statsResultsList)
    {
        g_filteredStatsRegistryIndexes.clear();
        if (g_statsResultCountText)
        {
            g_statsResultCountText->setCaption("0 stats");
        }
        return;
    }

    const StatsEnumerated previouslySelectedStat = g_selectedStatsStat;

    g_statsResultsList->removeAllItems();
    g_filteredStatsRegistryIndexes.clear();

    std::string searchUpper;
    if (g_statsSearchEdit)
    {
        searchUpper = ToUpperAscii(TrimAscii(g_statsSearchEdit->getOnlyText().asUTF8()));
    }
    const bool showSectionBatchEntry = searchUpper.empty();
    const bool includeGroupingHint =
        g_activeStatsSectionFilter == StatsSectionFilter_All
        || g_activeStatsSectionFilter == StatsSectionFilter_CommonTest;
    size_t matchingStatCount = 0u;

    if (showSectionBatchEntry)
    {
        g_filteredStatsRegistryIndexes.push_back(-1);
        g_statsResultsList->addItem(BuildStatsSectionAllListLabel(g_activeStatsSectionFilter));
    }

    for (size_t index = 0; index < GetStatsRegistryCount(); ++index)
    {
        const StatsRegistryEntry& entry = kStatsRegistry[index];
        if (!DoesStatsEntryMatchSectionFilter(entry, g_activeStatsSectionFilter))
        {
            continue;
        }
        if (!DoesStatsEntryMatchSearch(entry, searchUpper))
        {
            continue;
        }

        g_filteredStatsRegistryIndexes.push_back(static_cast<int>(index));
        g_statsResultsList->addItem(BuildStatsSectionedListLabel(entry, includeGroupingHint));
        ++matchingStatCount;
    }

    if (g_statsResultCountText)
    {
        std::stringstream caption;
        caption << GetStatsSectionFilterLabel(g_activeStatsSectionFilter)
                << ": " << matchingStatCount << " stats";
        g_statsResultCountText->setCaption(caption.str());
    }

    if (matchingStatCount == 0u)
    {
        g_statsResultsList->addItem("No matching stats");
        g_statsResultsList->clearIndexSelected();
        g_statsResultsList->beginToItemFirst();
        g_selectedStatsStat = STAT_NONE;
        RefreshStatsUi(g_lastPlayerInterface);
        return;
    }

    size_t nextSelectedIndex = showSectionBatchEntry ? 1u : 0u;
    if (nextSelectedIndex >= g_filteredStatsRegistryIndexes.size())
    {
        nextSelectedIndex = 0u;
    }
    for (size_t filteredIndex = 0; filteredIndex < g_filteredStatsRegistryIndexes.size(); ++filteredIndex)
    {
        const int registryIndex = g_filteredStatsRegistryIndexes[filteredIndex];
        if (registryIndex >= 0
            && static_cast<size_t>(registryIndex) < GetStatsRegistryCount()
            && kStatsRegistry[registryIndex].stat == previouslySelectedStat)
        {
            nextSelectedIndex = filteredIndex;
            break;
        }
    }

    const int selectedRegistryIndex = g_filteredStatsRegistryIndexes[nextSelectedIndex];
    g_selectedStatsStat =
        (selectedRegistryIndex >= 0 && static_cast<size_t>(selectedRegistryIndex) < GetStatsRegistryCount())
            ? kStatsRegistry[selectedRegistryIndex].stat
            : STAT_NONE;
    g_statsResultsList->setIndexSelected(nextSelectedIndex);
    g_statsResultsList->beginToItemAt(nextSelectedIndex);
    RefreshStatsUi(g_lastPlayerInterface);
}

void ApplyStatsEditOperation(StatsEditOperation operation)
{
    StatsSectionFilter batchSection = StatsSectionFilter_All;
    const bool sectionBatch = IsStatsSectionBatchSelectionActive(&batchSection);
    const char* actionId = sectionBatch
        ? StatsSectionEditOperationToActionId(operation)
        : StatsEditOperationToActionId(operation);
    LogActionRequested(actionId);

    if (!g_lastPlayerInterface)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_player_interface\"";
        LogInfoLine(result.str());
        SetStatusMessage("Stats edit failed - player interface unavailable");
        return;
    }

    const StatsRegistryEntry* entry = 0;
    if (!sectionBatch && (!TryResolveSelectedStatsEntry(&entry) || !entry))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_stat_selected\"";
        LogInfoLine(result.str());
        SetStatusMessage("Stats edit failed - select a stat");
        return;
    }
    const std::string selectionLabel = sectionBatch
        ? BuildStatsSectionAllListLabel(batchSection)
        : (entry ? entry->label : "stat");

    if (!g_statsInputEdit)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"missing_input_widget\""
               << " target=\"" << SanitizeLogValue(selectionLabel) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(selectionLabel + " edit failed - input unavailable");
        return;
    }

    const std::string inputText = TrimAscii(g_statsInputEdit->getOnlyText().asUTF8());
    int inputValue = 0;
    if (!TryParseNonNegativeInt(inputText, &inputValue)
        || ((operation == StatsEditOperation_Add || operation == StatsEditOperation_Subtract) && inputValue <= 0))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"invalid_value\""
               << " target=\"" << SanitizeLogValue(selectionLabel) << "\""
               << " value_text=\"" << SanitizeLogValue(inputText) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(
            selectionLabel
            + (operation == StatsEditOperation_Set
                    ? " edit failed - enter a value from 0 to 100"
                    : " edit failed - enter a positive value"));
        return;
    }

    g_statsInputEdit->setOnlyText(inputText);

    std::vector<Character*> targets;
    if (!TryCollectStatsTargets(g_lastPlayerInterface, g_statsApplyToAllSelected, &targets) || targets.empty())
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_selected_character\""
               << " target=\"" << SanitizeLogValue(selectionLabel) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Stats edit failed - select a character");
        return;
    }

    const Character* primaryTarget = TryGetPrimarySelectedCharacter(g_lastPlayerInterface);
    const std::string primaryName = primaryTarget ? SafeCharacterName(const_cast<Character*>(primaryTarget)) : "";

    if (sectionBatch)
    {
        std::vector<const StatsRegistryEntry*> sectionEntries;
        CollectStatsEntriesForSectionFilter(batchSection, &sectionEntries);
        if (sectionEntries.empty())
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"empty_section\""
                   << " section=\"" << SanitizeLogValue(GetStatsSectionFilterLabel(batchSection)) << "\"";
            LogInfoLine(result.str());
            SetStatusMessage(selectionLabel + " edit failed - section is empty");
            return;
        }

        int appliedCharacterCount = 0;
        int appliedStatEditCount = 0;
        bool anyClamped = false;

        for (size_t targetIndex = 0; targetIndex < targets.size(); ++targetIndex)
        {
            int appliedToCharacter = 0;
            for (size_t entryIndex = 0; entryIndex < sectionEntries.size(); ++entryIndex)
            {
                int beforeValue = 0;
                int afterValue = 0;
                bool clamped = false;
                if (!TryApplyStatsEditToCharacter(
                        targets[targetIndex],
                        *sectionEntries[entryIndex],
                        operation,
                        inputValue,
                        &beforeValue,
                        &afterValue,
                        &clamped))
                {
                    continue;
                }

                ++appliedStatEditCount;
                ++appliedToCharacter;
                if (clamped)
                {
                    anyClamped = true;
                }
            }

            if (appliedToCharacter > 0)
            {
                ++appliedCharacterCount;
            }
        }

        const bool success = appliedStatEditCount > 0;
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success="
               << (success ? "true" : "false")
               << " section=\"" << SanitizeLogValue(GetStatsSectionFilterLabel(batchSection)) << "\""
               << " requested_value=" << inputValue
               << " apply_to_all_selected=" << (g_statsApplyToAllSelected ? "true" : "false")
               << " target_count=" << targets.size()
               << " applied_character_count=" << appliedCharacterCount
               << " applied_stat_edit_count=" << appliedStatEditCount
               << " section_stat_count=" << sectionEntries.size()
               << " clamped=" << (anyClamped ? "true" : "false");
        if (!success)
        {
            result << " reason=\"apply_failed\"";
        }
        LogInfoLine(result.str());

        RefreshStatsUi(g_lastPlayerInterface);

        if (!success)
        {
            SetStatusMessage(selectionLabel + " edit failed - stat path unavailable");
            return;
        }

        std::stringstream status;
        if (operation == StatsEditOperation_Set)
        {
            const int setValue = ClampIntValue(inputValue, kStatsSafeValueMin, kStatsSafeValueMax);
            if (appliedCharacterCount == 1 && !primaryName.empty())
            {
                status << "Set " << selectionLabel << " to " << setValue << " for " << primaryName;
            }
            else if (appliedCharacterCount == static_cast<int>(targets.size()))
            {
                status << "Set " << selectionLabel << " for " << appliedCharacterCount << " selected character(s)";
            }
            else
            {
                status << "Set " << selectionLabel << " for " << appliedCharacterCount << " of " << targets.size()
                       << " selected character(s)";
            }
        }
        else
        {
            status << StatsEditOperationToStatusVerb(operation) << ' ' << inputValue << ' ';
            status << StatsEditOperationToStatusPreposition(operation) << " each of " << selectionLabel << ' ';
            if (appliedCharacterCount == 1 && !primaryName.empty())
            {
                status << primaryName;
            }
            else if (appliedCharacterCount == static_cast<int>(targets.size()))
            {
                status << appliedCharacterCount << " selected character(s)";
            }
            else
            {
                status << appliedCharacterCount << " of " << targets.size() << " selected character(s)";
            }
        }

        if (anyClamped)
        {
            status << " (clamped)";
        }
        SetStatusMessage(status.str());
        return;
    }

    int appliedCount = 0;
    int primaryBeforeValue = 0;
    int primaryAfterValue = 0;
    bool primaryObserved = false;
    bool anyClamped = false;

    for (size_t index = 0; index < targets.size(); ++index)
    {
        int beforeValue = 0;
        int afterValue = 0;
        bool clamped = false;
        if (!TryApplyStatsEditToCharacter(
                targets[index],
                *entry,
                operation,
                inputValue,
                &beforeValue,
                &afterValue,
                &clamped))
        {
            continue;
        }

        ++appliedCount;
        if (clamped)
        {
            anyClamped = true;
        }

        if (!primaryObserved && primaryTarget && targets[index] == primaryTarget)
        {
            primaryBeforeValue = beforeValue;
            primaryAfterValue = afterValue;
            primaryObserved = true;
        }
    }

    const bool success = appliedCount > 0;
    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId << "\" success="
           << (success ? "true" : "false")
           << " stat=\"" << SanitizeLogValue(selectionLabel) << "\""
           << " requested_value=" << inputValue
           << " apply_to_all_selected=" << (g_statsApplyToAllSelected ? "true" : "false")
           << " target_count=" << targets.size()
           << " applied_count=" << appliedCount
           << " clamped=" << (anyClamped ? "true" : "false");
    if (primaryObserved)
    {
        result << " primary_before=" << primaryBeforeValue
               << " primary_after=" << primaryAfterValue;
    }
    if (!success)
    {
        result << " reason=\"apply_failed\"";
    }
    LogInfoLine(result.str());

    RefreshStatsUi(g_lastPlayerInterface);

    if (!success)
    {
        SetStatusMessage(selectionLabel + " edit failed - stat path unavailable");
        return;
    }

    std::stringstream status;
    if (operation == StatsEditOperation_Set)
    {
        if (appliedCount == 1 && !primaryName.empty())
        {
            status << "Set " << selectionLabel << " to " << (primaryObserved ? primaryAfterValue : inputValue)
                   << " for " << primaryName;
        }
        else if (appliedCount == static_cast<int>(targets.size()))
        {
            status << "Set " << selectionLabel << " for " << appliedCount << " selected character(s)";
        }
        else
        {
            status << "Set " << selectionLabel << " for " << appliedCount << " of " << targets.size()
                   << " selected character(s)";
        }
    }
    else
    {
        status << StatsEditOperationToStatusVerb(operation) << ' ' << inputValue << ' ' << selectionLabel << ' ';
        status << StatsEditOperationToStatusPreposition(operation) << ' ';
        if (appliedCount == 1 && !primaryName.empty())
        {
            status << primaryName;
        }
        else if (appliedCount == static_cast<int>(targets.size()))
        {
            status << appliedCount << " selected character(s)";
        }
        else
        {
            status << appliedCount << " of " << targets.size() << " selected character(s)";
        }
    }

    if (anyClamped)
    {
        status << " (clamped)";
    }
    SetStatusMessage(status.str());
}

void OnStatsSearchTextChanged(MyGUI::EditBox*)
{
    RefreshStatsList();
}

void OnStatsResultsSelectionChanged(MyGUI::ListBox*, size_t index)
{
    if (index >= g_filteredStatsRegistryIndexes.size())
    {
        g_selectedStatsStat = STAT_NONE;
    }
    else
    {
        const int registryIndex = g_filteredStatsRegistryIndexes[index];
        g_selectedStatsStat =
            (registryIndex >= 0 && static_cast<size_t>(registryIndex) < GetStatsRegistryCount())
                ? kStatsRegistry[registryIndex].stat
                : STAT_NONE;
    }

    RefreshStatsUi(g_lastPlayerInterface);
}

void OnStatsInputTextChanged(MyGUI::EditBox*)
{
    RefreshStatsUi(g_lastPlayerInterface);
}

void OnStatsApplyToAllButtonClicked(MyGUI::Widget*)
{
    if (GetSelectedCharacterCount(g_lastPlayerInterface) <= 1)
    {
        g_statsApplyToAllSelected = false;
    }
    else
    {
        g_statsApplyToAllSelected = !g_statsApplyToAllSelected;
    }

    RefreshStatsUi(g_lastPlayerInterface);
}

void OnStatsApplyToAllButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnStatsApplyToAllButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void CopyCurrentStatsBlock()
{
    const char* actionId = "copy_stats_block";
    LogActionRequested(actionId);

    if (!g_lastPlayerInterface)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_player_interface\"";
        LogInfoLine(result.str());
        SetStatusMessage("Copy Stats failed - player interface unavailable");
        return;
    }

    Character* sourceCharacter = TryGetPrimarySelectedCharacter(g_lastPlayerInterface);
    if (!sourceCharacter)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_selected_character\"";
        LogInfoLine(result.str());
        SetStatusMessage("Copy Stats failed - select a character");
        return;
    }

    std::string sourceName;
    int copiedCount = 0;
    int attemptedCount = 0;
    if (!TryCopyStatsBlockFromCharacter(sourceCharacter, &sourceName, &copiedCount, &attemptedCount))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"read_failed\""
               << " source=\"" << SanitizeLogValue(SafeCharacterName(sourceCharacter)) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Copy Stats failed - stat read path unavailable");
        RefreshStatsUi(g_lastPlayerInterface);
        return;
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId << "\" success=true"
           << " source=\"" << SanitizeLogValue(sourceName) << "\""
           << " copied_count=" << copiedCount
           << " attempted_count=" << attemptedCount;
    if (copiedCount < attemptedCount)
    {
        result << " partial=true";
    }
    LogInfoLine(result.str());

    RefreshStatsUi(g_lastPlayerInterface);

    std::stringstream status;
    if (copiedCount >= attemptedCount)
    {
        status << "Copied " << copiedCount << " stats from " << sourceName;
    }
    else
    {
        status << "Copied " << copiedCount << " of " << attemptedCount << " stats from " << sourceName;
    }
    SetStatusMessage(status.str());
}

void PasteCopiedStatsBlock()
{
    const char* actionId = "paste_stats_block";
    LogActionRequested(actionId);

    if (!g_lastPlayerInterface)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_player_interface\"";
        LogInfoLine(result.str());
        SetStatusMessage("Paste Stats failed - player interface unavailable");
        return;
    }

    if (!HasStatsClipboardData())
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"clipboard_empty\"";
        LogInfoLine(result.str());
        SetStatusMessage("Paste Stats failed - copy stats first");
        RefreshStatsUi(g_lastPlayerInterface);
        return;
    }

    const int selectedCount = GetSelectedCharacterCount(g_lastPlayerInterface);
    const bool applyToSelectedTargets = selectedCount > 1;

    std::vector<Character*> targets;
    if (!TryCollectStatsTargets(g_lastPlayerInterface, applyToSelectedTargets, &targets) || targets.empty())
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"no_selected_character\"";
        LogInfoLine(result.str());
        SetStatusMessage("Paste Stats failed - select one or more characters");
        RefreshStatsUi(g_lastPlayerInterface);
        return;
    }

    const Character* primaryTarget = TryGetPrimarySelectedCharacter(g_lastPlayerInterface);
    const std::string primaryName = primaryTarget ? SafeCharacterName(const_cast<Character*>(primaryTarget)) : "";

    int appliedCharacterCount = 0;
    int appliedStatEditCount = 0;
    bool anyClamped = false;

    for (size_t index = 0; index < targets.size(); ++index)
    {
        int appliedToCharacter = 0;
        bool characterClamped = false;
        if (!TryApplyStatsClipboardToCharacter(targets[index], &appliedToCharacter, &characterClamped))
        {
            continue;
        }

        ++appliedCharacterCount;
        appliedStatEditCount += appliedToCharacter;
        if (characterClamped)
        {
            anyClamped = true;
        }
    }

    const bool success = appliedStatEditCount > 0;
    std::stringstream result;
    result << "event=testkit_action_result action=\"" << actionId << "\" success="
           << (success ? "true" : "false")
           << " source=\"" << SanitizeLogValue(g_statsClipboardSourceName) << "\""
           << " copied_stat_count=" << g_statsClipboardEntries.size()
           << " apply_to_all_selected=" << (applyToSelectedTargets ? "true" : "false")
           << " target_count=" << targets.size()
           << " applied_character_count=" << appliedCharacterCount
           << " applied_stat_edit_count=" << appliedStatEditCount
           << " clamped=" << (anyClamped ? "true" : "false");
    if (!success)
    {
        result << " reason=\"apply_failed\"";
    }
    LogInfoLine(result.str());

    RefreshStatsUi(g_lastPlayerInterface);

    if (!success)
    {
        SetStatusMessage("Paste Stats failed - stat apply path unavailable");
        return;
    }

    std::stringstream status;
    if (appliedCharacterCount == 1 && !primaryName.empty())
    {
        status << "Pasted " << appliedStatEditCount << " copied stats";
        if (!g_statsClipboardSourceName.empty())
        {
            status << " from " << g_statsClipboardSourceName;
        }
        status << " to " << primaryName;
    }
    else if (appliedCharacterCount == static_cast<int>(targets.size()))
    {
        status << "Pasted copied stats";
        if (!g_statsClipboardSourceName.empty())
        {
            status << " from " << g_statsClipboardSourceName;
        }
        status << " to " << appliedCharacterCount << " selected character(s)";
    }
    else
    {
        status << "Pasted copied stats";
        if (!g_statsClipboardSourceName.empty())
        {
            status << " from " << g_statsClipboardSourceName;
        }
        status << " to " << appliedCharacterCount << " of " << targets.size() << " selected character(s)";
    }

    if (anyClamped)
    {
        status << " (clamped)";
    }
    SetStatusMessage(status.str());
}

void SetActiveStatsSectionFilter(StatsSectionFilter section)
{
    g_activeStatsSectionFilter = section;
    RefreshStatsList();
    RefreshStatsUi(g_lastPlayerInterface);
}

void HandleStatsSectionButtonPressed(MyGUI::MouseButton id, StatsSectionFilter section)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActiveStatsSectionFilter(section);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnStatsAllSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_All);
}

void OnStatsCommonSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_CommonTest);
}

void OnStatsCoreSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_Core);
}

void OnStatsUtilitySectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_MovementUtility);
}

void OnStatsCombatSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_Combat);
}

void OnStatsWeaponsSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_WeaponSkills);
}

void OnStatsLaborSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    HandleStatsSectionButtonPressed(id, StatsSectionFilter_Labor);
}

void OnStatsCopyButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    CopyCurrentStatsBlock();

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnStatsPasteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    PasteCopiedStatsBlock();

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnStatsSetButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    ApplyStatsEditOperation(StatsEditOperation_Set);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnStatsAddButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    ApplyStatsEditOperation(StatsEditOperation_Add);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnStatsSubtractButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    ApplyStatsEditOperation(StatsEditOperation_Subtract);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnStatsTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    RefreshStatsList();
    RefreshStatsUi(g_lastPlayerInterface);
    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Stats);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}
}
