#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
void RefreshSaveLocationInputUi();
void RefreshSavedLocationsListWidget();
void UpdateSavedLocationsCollapseButtonCaption();
void RefreshSavedLocationActionButtons(PlayerInterface* player);
void ClearPendingDownedTeleportRestores();
void TickPendingDownedTeleportRestores();
void OnSaveLocationNameTextChanged(MyGUI::EditBox*);
void OnSaveLocationNameAccepted(MyGUI::EditBox*);
void OnSaveSelectedLocationButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationsCollapseButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationSearchTextChanged(MyGUI::EditBox*);
void OnSavedLocationsListSelectionChanged(MyGUI::ListBox*, size_t);
void OnSavedLocationTeleportButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationPinButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationRenameButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnSavedLocationDeleteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
}
