#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
void ApplyPanelLayout();
void CreatePanelWidgets();
void DestroyPanel();
void EnsurePanel(PlayerInterface* thisptr);
void TogglePanelHidden(const char* source);
void TogglePanelCollapsed(const char* source);
void TickPanelToggleHotkey();
}
