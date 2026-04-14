#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
void ResetConstructionUiState();
void RefreshConstructionUi(PlayerInterface* player);
void OnConstructionTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnFinishSelectedConstructionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
}
