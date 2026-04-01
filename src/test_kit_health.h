#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
bool TryResolveStateSummary(
    Character* target,
    std::string* outLabel,
    bool* unconsciousOut,
    bool* playingDeadOut,
    bool* dyingOut,
    bool* deadOut);

void UpdateDangerousActionButtonCaptions();
void ClearDangerousActionArm(const char* reason, bool updateStatus);
void TickDangerousActionArmTimeout();

void OnFullRestoreButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnForceUnconsciousButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnDamageLeftArmButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnDamageRightArmButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnDamageLeftLegButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnDamageRightLegButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnForceDeadButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
void OnForceDyingButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
}
