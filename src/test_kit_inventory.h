#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
void ResetInventoryRuntimeState();
void ResetInventoryWidgetInteractionState();
void EnsureInventoryFoodItemOptionsLoaded();
void RefreshInventoryFoodItemDropdown();
void RefreshInventorySpawnButtonState();
void TickInventorySearchFocusHotkey();
void OnAddMoneyButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnInventoryCategoryChanged(MyGUI::ComboBox*, size_t);
void OnInventoryItemSearchTextChanged(MyGUI::EditBox*);
void OnInventoryItemSearchFocusChanged(MyGUI::Widget*, MyGUI::Widget*);
void OnInventoryItemSearchKeyPressed(MyGUI::Widget*, MyGUI::KeyCode, MyGUI::Char);
void OnInventoryItemSearchKeyReleased(MyGUI::Widget*, MyGUI::KeyCode);
void OnInventorySearchResultsSelectionChanged(MyGUI::ListBox*, size_t);
void OnSpawnItemButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
}
