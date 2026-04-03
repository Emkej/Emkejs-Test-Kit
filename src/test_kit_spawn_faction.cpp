#include "test_kit_spawn.h"

#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>

#include <algorithm>

namespace test_kit
{
namespace
{
struct SpawnFactionOption
{
    std::string displayLabel;
    std::string name;
    std::string stringId;
    std::string searchTextUpper;
    Faction* faction;
};

std::vector<SpawnFactionOption> g_spawnFactionOptions;
std::vector<size_t> g_filteredSpawnFactionOptionIndexes;
bool g_spawnFactionOptionsLoaded = false;

GameData* TryGetSpawnFactionDataSafe(Faction* faction)
{
    if (!faction)
    {
        return 0;
    }

    __try
    {
        return faction->getData();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

std::string NormalizeSpawnFactionValue(const std::string& value)
{
    return ToUpperAscii(TrimAscii(value));
}

bool HasSpawnFactionOptionForFaction(Faction* faction)
{
    for (size_t index = 0; index < g_spawnFactionOptions.size(); ++index)
    {
        if (g_spawnFactionOptions[index].faction == faction)
        {
            return true;
        }
    }

    return false;
}

std::string BuildSpawnFactionOptionLabel(const std::string& name, const std::string& stringId)
{
    if (!name.empty() && !stringId.empty())
    {
        return name + " (" + stringId + ")";
    }

    if (!name.empty())
    {
        return name;
    }

    return stringId;
}

void AddSpawnFactionOption(Faction* faction)
{
    if (!faction || HasSpawnFactionOptionForFaction(faction))
    {
        return;
    }

    GameData* factionData = TryGetSpawnFactionDataSafe(faction);
    std::string name = TrimAscii(SafeFactionName(faction));
    std::string stringId;
    if (factionData)
    {
        const std::string factionDataName = TrimAscii(factionData->name);
        if (!factionDataName.empty())
        {
            name = factionDataName;
        }
        stringId = TrimAscii(factionData->stringID);
    }

    if (name.empty() && stringId.empty())
    {
        return;
    }

    SpawnFactionOption option;
    option.displayLabel = BuildSpawnFactionOptionLabel(name, stringId);
    option.name = name;
    option.stringId = stringId;
    option.searchTextUpper = NormalizeSpawnFactionValue(option.displayLabel);
    if (!option.name.empty())
    {
        option.searchTextUpper += " " + NormalizeSpawnFactionValue(option.name);
    }
    if (!option.stringId.empty())
    {
        option.searchTextUpper += " " + NormalizeSpawnFactionValue(option.stringId);
    }
    option.faction = faction;
    g_spawnFactionOptions.push_back(option);
}

void EnsureSpawnFactionOptionsLoaded()
{
    if (g_spawnFactionOptionsLoaded || !ou || !ou->initialized || !ou->factionMgr)
    {
        return;
    }

    const lektor<Faction*>* factions = ou->factionMgr->getAllFactions();
    if (!factions)
    {
        return;
    }

    for (lektor<Faction*>::const_iterator it = factions->begin(); it != factions->end(); ++it)
    {
        AddSpawnFactionOption(*it);
    }

    std::sort(
        g_spawnFactionOptions.begin(),
        g_spawnFactionOptions.end(),
        [](const SpawnFactionOption& left, const SpawnFactionOption& right) -> bool
        {
            return NormalizeSpawnFactionValue(left.displayLabel) < NormalizeSpawnFactionValue(right.displayLabel);
        });
    g_spawnFactionOptionsLoaded = true;
}

size_t FindExactSpawnFactionOptionIndex(const std::string& query)
{
    const std::string normalizedQuery = NormalizeSpawnFactionValue(query);
    if (normalizedQuery.empty())
    {
        return static_cast<size_t>(-1);
    }

    for (size_t index = 0; index < g_spawnFactionOptions.size(); ++index)
    {
        const SpawnFactionOption& option = g_spawnFactionOptions[index];
        if (NormalizeSpawnFactionValue(option.stringId) == normalizedQuery
            || NormalizeSpawnFactionValue(option.name) == normalizedQuery
            || NormalizeSpawnFactionValue(option.displayLabel) == normalizedQuery)
        {
            return index;
        }
    }

    return static_cast<size_t>(-1);
}

size_t GetSelectedSpawnFactionOptionIndex()
{
    if (!g_spawnCustomFactionResultsList)
    {
        return static_cast<size_t>(-1);
    }

    const size_t selectedFilteredIndex = g_spawnCustomFactionResultsList->getIndexSelected();
    if (selectedFilteredIndex >= g_filteredSpawnFactionOptionIndexes.size())
    {
        return static_cast<size_t>(-1);
    }

    return g_filteredSpawnFactionOptionIndexes[selectedFilteredIndex];
}

void RefreshSpawnCustomFactionResultsList()
{
    if (!g_spawnCustomFactionSearchEdit || !g_spawnCustomFactionResultsList)
    {
        return;
    }

    EnsureSpawnFactionOptionsLoaded();
    const size_t previousSelectedOptionIndex = GetSelectedSpawnFactionOptionIndex();
    const std::string searchUpper =
        NormalizeSpawnFactionValue(g_spawnCustomFactionSearchEdit->getOnlyText().asUTF8());

    g_filteredSpawnFactionOptionIndexes.clear();
    g_spawnCustomFactionResultsList->removeAllItems();

    for (size_t index = 0; index < g_spawnFactionOptions.size(); ++index)
    {
        const SpawnFactionOption& option = g_spawnFactionOptions[index];
        if (searchUpper.empty() || option.searchTextUpper.find(searchUpper) != std::string::npos)
        {
            g_filteredSpawnFactionOptionIndexes.push_back(index);
            g_spawnCustomFactionResultsList->addItem(option.displayLabel);
        }
    }

    if (g_filteredSpawnFactionOptionIndexes.empty())
    {
        g_spawnCustomFactionResultsList->clearIndexSelected();
        g_spawnCustomFactionResultsList->beginToItemFirst();
        return;
    }

    size_t nextSelectedFilteredIndex = static_cast<size_t>(-1);
    g_spawnCustomFactionResultsList->clearIndexSelected();
    if (previousSelectedOptionIndex != static_cast<size_t>(-1))
    {
        for (size_t filteredIndex = 0; filteredIndex < g_filteredSpawnFactionOptionIndexes.size(); ++filteredIndex)
        {
            if (g_filteredSpawnFactionOptionIndexes[filteredIndex] == previousSelectedOptionIndex)
            {
                nextSelectedFilteredIndex = filteredIndex;
                break;
            }
        }
    }

    if (nextSelectedFilteredIndex == static_cast<size_t>(-1))
    {
        const size_t exactIndex =
            FindExactSpawnFactionOptionIndex(g_spawnCustomFactionSearchEdit->getOnlyText().asUTF8());
        for (size_t filteredIndex = 0; filteredIndex < g_filteredSpawnFactionOptionIndexes.size(); ++filteredIndex)
        {
            if (g_filteredSpawnFactionOptionIndexes[filteredIndex] == exactIndex)
            {
                nextSelectedFilteredIndex = filteredIndex;
                break;
            }
        }
    }

    if (nextSelectedFilteredIndex == static_cast<size_t>(-1)
        && g_filteredSpawnFactionOptionIndexes.size() == 1u)
    {
        nextSelectedFilteredIndex = 0u;
    }

    if (nextSelectedFilteredIndex != static_cast<size_t>(-1))
    {
        g_spawnCustomFactionResultsList->setIndexSelected(nextSelectedFilteredIndex);
        g_spawnCustomFactionResultsList->beginToItemSelected();
    }
    else
    {
        g_spawnCustomFactionResultsList->clearIndexSelected();
        g_spawnCustomFactionResultsList->beginToItemFirst();
    }
}
}

const char* SpawnTemplateFactionModeToLabel(SpawnTemplateFactionMode factionMode)
{
    switch (factionMode)
    {
    case SpawnTemplateFactionMode_Custom:
        return "Custom";
    case SpawnTemplateFactionMode_None:
    default:
        return "None";
    }
}

SpawnTemplateFactionMode GetSelectedSpawnTemplateFactionMode()
{
    if (!g_spawnFactionDropdown)
    {
        return SpawnTemplateFactionMode_None;
    }

    switch (g_spawnFactionDropdown->getIndexSelected())
    {
    case 1:
        return SpawnTemplateFactionMode_Custom;
    case 0:
    default:
        return SpawnTemplateFactionMode_None;
    }
}

bool ShouldShowSpawnCustomFactionControls()
{
    return GetSelectedSpawnTemplateFactionMode() == SpawnTemplateFactionMode_Custom;
}

bool TryResolveSelectedSpawnTemplateFactionSelection(
    SpawnTemplateFactionSelection* outSelection,
    std::string* outErrorMessage)
{
    if (outSelection)
    {
        *outSelection = SpawnTemplateFactionSelection();
    }
    if (outErrorMessage)
    {
        outErrorMessage->clear();
    }

    SpawnTemplateFactionSelection selection;
    selection.mode = GetSelectedSpawnTemplateFactionMode();
    if (selection.mode != SpawnTemplateFactionMode_Custom)
    {
        if (outSelection)
        {
            *outSelection = selection;
        }
        return true;
    }

    if (!g_spawnCustomFactionSearchEdit
        || !g_spawnCustomFactionResultsList
        || !ou
        || !ou->initialized
        || !ou->factionMgr)
    {
        if (outErrorMessage)
        {
            *outErrorMessage = "Faction controls unavailable";
        }
        return false;
    }

    RefreshSpawnCustomFactionResultsList();
    selection.customFactionQuery = TrimAscii(g_spawnCustomFactionSearchEdit->getOnlyText().asUTF8());
    if (selection.customFactionQuery.empty())
    {
        if (outErrorMessage)
        {
            *outErrorMessage = "Select a faction";
        }
        return false;
    }

    const size_t selectedOptionIndex = GetSelectedSpawnFactionOptionIndex();
    if (selectedOptionIndex == static_cast<size_t>(-1) || selectedOptionIndex >= g_spawnFactionOptions.size())
    {
        if (outErrorMessage)
        {
            *outErrorMessage = "Select a faction";
        }
        return false;
    }

    const SpawnFactionOption& option = g_spawnFactionOptions[selectedOptionIndex];
    selection.customFaction = option.faction;
    selection.customFactionLabel = option.name.empty() ? option.displayLabel : option.name;
    if (outSelection)
    {
        *outSelection = selection;
    }
    return true;
}

std::string DescribeSpawnTemplateFactionSelection(const SpawnTemplateFactionSelection& selection)
{
    if (selection.mode != SpawnTemplateFactionMode_Custom)
    {
        return "None";
    }

    if (!selection.customFactionLabel.empty())
    {
        return selection.customFactionLabel;
    }

    if (!selection.customFactionQuery.empty())
    {
        return selection.customFactionQuery;
    }

    return "Custom";
}

void RefreshSpawnFactionControlState()
{
    const bool showCustomFaction = ShouldShowSpawnCustomFactionControls();
    const bool customFactionWidgetsVisible =
        showCustomFaction && g_activePanelTab == PanelTab_Spawn && !g_panelHidden;

    if (g_spawnFactionLabelText)
    {
        g_spawnFactionLabelText->setEnabled(true);
    }
    if (g_spawnFactionDropdown)
    {
        g_spawnFactionDropdown->setEnabled(true);
    }
    if (g_spawnCustomFactionLabelText)
    {
        g_spawnCustomFactionLabelText->setEnabled(showCustomFaction);
        g_spawnCustomFactionLabelText->setVisible(customFactionWidgetsVisible);
    }
    if (g_spawnCustomFactionSearchEdit)
    {
        g_spawnCustomFactionSearchEdit->setEnabled(showCustomFaction);
        g_spawnCustomFactionSearchEdit->setVisible(customFactionWidgetsVisible);
    }
    if (g_spawnCustomFactionResultsList)
    {
        g_spawnCustomFactionResultsList->setEnabled(showCustomFaction);
        g_spawnCustomFactionResultsList->setVisible(customFactionWidgetsVisible);
    }

    if (showCustomFaction)
    {
        RefreshSpawnCustomFactionResultsList();
    }
}

void OnSpawnFactionModeChanged(MyGUI::ComboBox*, size_t)
{
    RefreshSpawnFactionControlState();
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void OnSpawnCustomFactionTextChanged(MyGUI::EditBox*)
{
    RefreshSpawnCustomFactionResultsList();
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}

void OnSpawnCustomFactionResultsSelectionChanged(MyGUI::ListBox*, size_t)
{
    RefreshSpawnButtonState();
    RefreshSpawnPreviewText();
}
}
