#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
void RefreshStatsUi(PlayerInterface* player);
void RefreshStatsList();
void UpdateStatsSectionButtonCaptions();
void OnStatsTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsAllSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCommonSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCoreSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsUtilitySectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCombatSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsWeaponsSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsLaborSectionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsSearchTextChanged(MyGUI::EditBox*);
void OnStatsResultsSelectionChanged(MyGUI::ListBox*, size_t);
void OnStatsInputTextChanged(MyGUI::EditBox*);
void OnStatsApplyToAllButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsCopyButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsPasteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsSetButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsAddButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnStatsSubtractButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
}
