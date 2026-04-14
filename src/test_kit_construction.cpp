#include "test_kit_construction.h"

#include <kenshi/Building.h>
#include <kenshi/RootObject.h>
#include <mygui/MyGUI_InputManager.h>

#include <sstream>

namespace test_kit
{
namespace
{
struct ConstructionSelectionSnapshot
{
    ConstructionSelectionSnapshot()
        : hasSelection(false)
        , selectedObjectBase(0)
        , selectedBuilding(0)
        , constructionTarget(0)
        , buildState(0)
        , playerFaction(0)
        , ownerFaction(0)
        , directConstructionTarget(false)
        , constructionTargetResolvedFromDoor(false)
        , buildingHandleValid(false)
        , buildStateComplete(false)
    {
    }

    bool hasSelection;
    RootObjectBase* selectedObjectBase;
    Building* selectedBuilding;
    Building* constructionTarget;
    Building::ConstructionState* buildState;
    std::string selectedName;
    Faction* playerFaction;
    Faction* ownerFaction;
    bool directConstructionTarget;
    bool constructionTargetResolvedFromDoor;
    bool buildingHandleValid;
    bool buildStateComplete;
};

struct ConstructionValidationResult
{
    ConstructionValidationResult()
        : valid(false)
    {
    }

    bool valid;
    std::string message;
    const char* reason;
};

RootObjectBase* g_constructionStickySelection = 0;
std::string g_constructionStickySelectionName;
std::string g_constructionStickyStatusMessage;
bool g_constructionHasStickyStatus = false;
bool g_constructionActionLocked = false;

void SetConstructionSelectedLabel(const std::string& selectedName)
{
    if (g_constructionSelectedText)
    {
        g_constructionSelectedText->setCaption(std::string("Selected: ") + selectedName);
    }
}

void SetConstructionStatusLabel(const std::string& statusMessage)
{
    if (g_constructionStatusText)
    {
        g_constructionStatusText->setCaption(std::string("Status: ") + statusMessage);
    }
}

Building::ConstructionState* GetConstructionState(Building* building)
{
    if (!building)
    {
        return 0;
    }

    Building::ConstructionState* state = building->getBuildState();
    if (!state)
    {
        state = building->getBuildState_ActualNonShared();
    }
    return state;
}

Building* ResolveConstructionTarget(Building* selectedBuilding, bool* outResolvedFromDoor)
{
    if (outResolvedFromDoor)
    {
        *outResolvedFromDoor = false;
    }

    if (!selectedBuilding)
    {
        return 0;
    }

    if (!selectedBuilding->isDoor())
    {
        return selectedBuilding;
    }

    Building* parentBuilding = selectedBuilding->doorParentBuilding();
    if (!parentBuilding || parentBuilding == selectedBuilding)
    {
        return selectedBuilding;
    }

    Building::ConstructionState* selectedState = GetConstructionState(selectedBuilding);
    Building::ConstructionState* parentState = GetConstructionState(parentBuilding);
    if (selectedState && selectedState->isComplete && parentState && !parentState->isComplete)
    {
        if (outResolvedFromDoor)
        {
            *outResolvedFromDoor = true;
        }
        return parentBuilding;
    }

    return selectedBuilding;
}

bool TryGetConstructionSelectionSnapshot(PlayerInterface* player, ConstructionSelectionSnapshot* outSnapshot)
{
    if (!outSnapshot)
    {
        return false;
    }

    *outSnapshot = ConstructionSelectionSnapshot();
    if (!player)
    {
        return true;
    }

    const hand* selectedHand = 0;
    if (player->selectedObject.isValid())
    {
        selectedHand = &player->selectedObject;
    }
    else if (player->selectedCharacter.isValid())
    {
        selectedHand = &player->selectedCharacter;
    }

    if (!selectedHand)
    {
        return true;
    }

    outSnapshot->hasSelection = true;
    outSnapshot->selectedObjectBase = selectedHand->getRootObjectBase();
    outSnapshot->selectedBuilding = selectedHand->getBuilding();
    outSnapshot->playerFaction = player->getFaction();
    outSnapshot->constructionTarget = ResolveConstructionTarget(
        outSnapshot->selectedBuilding,
        &outSnapshot->constructionTargetResolvedFromDoor);

    if (outSnapshot->selectedObjectBase)
    {
        outSnapshot->selectedName = outSnapshot->selectedObjectBase->getName();
    }
    else if (outSnapshot->selectedBuilding)
    {
        outSnapshot->selectedName = outSnapshot->selectedBuilding->getName();
    }
    else
    {
        outSnapshot->selectedName = "Unknown";
    }

    if (outSnapshot->constructionTarget)
    {
        outSnapshot->ownerFaction = outSnapshot->constructionTarget->owner;
        outSnapshot->buildingHandleValid = outSnapshot->constructionTarget->getHandle().isValid();
        outSnapshot->directConstructionTarget = outSnapshot->selectedObjectBase
            == static_cast<RootObjectBase*>(outSnapshot->constructionTarget);

        outSnapshot->buildState = GetConstructionState(outSnapshot->constructionTarget);
        if (outSnapshot->buildState)
        {
            outSnapshot->buildStateComplete = outSnapshot->buildState->isComplete;
        }
    }

    return true;
}

ConstructionValidationResult ValidateConstructionSelection(const ConstructionSelectionSnapshot& snapshot)
{
    ConstructionValidationResult result;

    if (!snapshot.hasSelection)
    {
        result.message = "No selection";
        result.reason = "no_selection";
        return result;
    }

    if (!snapshot.constructionTarget)
    {
        result.message = "Selected target is not construction";
        result.reason = "not_construction";
        return result;
    }

    if (!snapshot.buildingHandleValid)
    {
        result.message = "Selected construction is not confirmed";
        result.reason = "not_confirmed";
        return result;
    }

    if (!snapshot.buildState)
    {
        result.message = "Selected target is not a supported construction target";
        result.reason = "unsupported_target";
        return result;
    }

    if (!snapshot.playerFaction)
    {
        result.message = "Selected target is not a supported construction target";
        result.reason = "unsupported_target";
        return result;
    }

    if (snapshot.ownerFaction != snapshot.playerFaction)
    {
        result.message = "Selected construction is not player-owned";
        result.reason = "not_player_owned";
        return result;
    }

    if (snapshot.buildStateComplete)
    {
        result.message = "Selected construction is already finished";
        result.reason = "already_finished";
        return result;
    }

    result.valid = true;
    result.message = "Ready";
    result.reason = "ready";
    return result;
}

bool TryCompleteConstructionTarget(Building* targetBuilding)
{
    if (!targetBuilding)
    {
        return false;
    }

    targetBuilding->setConstructionProgress(1.0f);
    targetBuilding->notifyConstructionComplete();
    return true;
}

bool TryReadConstructionFinishedState(Building* targetBuilding, bool* outFinished)
{
    if (outFinished)
    {
        *outFinished = false;
    }

    if (!targetBuilding)
    {
        return false;
    }

    Building::ConstructionState* state = targetBuilding->getBuildState();
    if (!state)
    {
        state = targetBuilding->getBuildState_ActualNonShared();
    }
    if (!state)
    {
        return false;
    }

    if (outFinished)
    {
        *outFinished = state->isComplete;
    }
    return true;
}

void ClearConstructionStickyStatus()
{
    g_constructionHasStickyStatus = false;
    g_constructionStickySelection = 0;
    g_constructionStickySelectionName.clear();
    g_constructionStickyStatusMessage.clear();
}

void SetConstructionStickyStatus(const ConstructionSelectionSnapshot& snapshot, const std::string& message)
{
    g_constructionHasStickyStatus = true;
    g_constructionStickySelection = snapshot.selectedObjectBase;
    g_constructionStickySelectionName = snapshot.selectedName;
    g_constructionStickyStatusMessage = message;
}

void RefreshConstructionButtonState(const ConstructionValidationResult& validation)
{
    if (!g_constructionFinishButton)
    {
        return;
    }

    g_constructionFinishButton->setEnabled(validation.valid && !g_constructionActionLocked);
}

void RefreshConstructionUiInternal(PlayerInterface* player)
{
    ConstructionSelectionSnapshot snapshot;
    if (!TryGetConstructionSelectionSnapshot(player, &snapshot))
    {
        return;
    }

    if (g_constructionHasStickyStatus
        && snapshot.hasSelection
        && snapshot.selectedObjectBase != g_constructionStickySelection)
    {
        ClearConstructionStickyStatus();
    }

    const ConstructionValidationResult validation = ValidateConstructionSelection(snapshot);
    const bool showStickyStatus = g_constructionHasStickyStatus
        && (!snapshot.hasSelection || snapshot.selectedObjectBase == g_constructionStickySelection);

    const std::string selectedName = snapshot.hasSelection
        ? snapshot.selectedName
        : (g_constructionHasStickyStatus ? g_constructionStickySelectionName : std::string("None"));
    SetConstructionSelectedLabel(selectedName.empty() ? "None" : selectedName);
    SetConstructionStatusLabel(showStickyStatus ? g_constructionStickyStatusMessage : validation.message);
    RefreshConstructionButtonState(validation);
}

void LogConstructionValidationFailure(const ConstructionSelectionSnapshot& snapshot, const ConstructionValidationResult& validation)
{
    std::stringstream line;
    line << "event=construction_finish_validation_failed reason=\"" << validation.reason << "\"";
    if (snapshot.hasSelection)
    {
        line << " target_name=\"" << SanitizeLogValue(snapshot.selectedName) << "\"";
    }
    LogInfoLine(line.str());
}

void LogConstructionSuccess(const std::string& targetName)
{
    std::stringstream line;
    line << "event=construction_finish_verified_success target_name=\"" << SanitizeLogValue(targetName) << "\"";
    LogInfoLine(line.str());
}

void LogConstructionVerifyInconclusive(const std::string& targetName, const char* reason)
{
    std::stringstream line;
    line << "event=construction_finish_verify_inconclusive target_name=\"" << SanitizeLogValue(targetName) << "\"";
    if (reason)
    {
        line << " reason=\"" << reason << "\"";
    }
    LogInfoLine(line.str());
}
} // namespace

void ResetConstructionUiState()
{
    ClearConstructionStickyStatus();
    g_constructionActionLocked = false;
}

void RefreshConstructionUi(PlayerInterface* player)
{
    RefreshConstructionUiInternal(player);
}

void OnConstructionTabButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    SetActivePanelTab(PanelTab_Construction);
    RefreshConstructionUi(g_lastPlayerInterface);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnFinishSelectedConstructionButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    g_constructionActionLocked = true;
    if (g_constructionFinishButton)
    {
        g_constructionFinishButton->setEnabled(false);
    }

    ConstructionSelectionSnapshot snapshot;
    if (!TryGetConstructionSelectionSnapshot(g_lastPlayerInterface, &snapshot))
    {
        g_constructionActionLocked = false;
        RefreshConstructionUi(g_lastPlayerInterface);
        if (inputManager)
        {
            inputManager->resetMouseCaptureWidget();
        }
        return;
    }

    const ConstructionValidationResult validation = ValidateConstructionSelection(snapshot);
    if (!validation.valid)
    {
        LogConstructionValidationFailure(snapshot, validation);
        SetConstructionStickyStatus(snapshot, validation.message);
        g_constructionActionLocked = false;
        RefreshConstructionUi(g_lastPlayerInterface);
        if (inputManager)
        {
            inputManager->resetMouseCaptureWidget();
        }
        return;
    }

    const RootObjectBase* expectedSelection = snapshot.selectedObjectBase;
    Building* targetBuilding = snapshot.constructionTarget;
    const std::string targetName = targetBuilding ? targetBuilding->getName() : snapshot.selectedName;

    {
        std::stringstream line;
        line << "event=construction_finish_execute target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(line.str());
    }

    if (!TryCompleteConstructionTarget(targetBuilding))
    {
        LogConstructionVerifyInconclusive(targetName, "apply_failed");
        SetConstructionStickyStatus(snapshot, "Failed: target is no longer valid");
        g_constructionActionLocked = false;
        RefreshConstructionUi(g_lastPlayerInterface);
        if (inputManager)
        {
            inputManager->resetMouseCaptureWidget();
        }
        return;
    }

    ConstructionSelectionSnapshot postActionSnapshot;
    const bool postActionSnapshotValid = TryGetConstructionSelectionSnapshot(g_lastPlayerInterface, &postActionSnapshot);
    bool verifiedFinished = false;
    const bool readbackAvailable = TryReadConstructionFinishedState(targetBuilding, &verifiedFinished);
    const bool sameSelectionStillLive = postActionSnapshotValid
        && postActionSnapshot.hasSelection
        && postActionSnapshot.selectedObjectBase == expectedSelection;

    if (readbackAvailable && verifiedFinished)
    {
        LogConstructionSuccess(targetName);
        SetConstructionStickyStatus(snapshot, std::string("Finished: ") + targetName);
    }
    else if (!sameSelectionStillLive)
    {
        LogConstructionVerifyInconclusive(targetName, "selection_changed");
        SetConstructionStickyStatus(snapshot, "Finished action could not be verified");
    }
    else
    {
        LogConstructionVerifyInconclusive(targetName, "not_finished");
        SetConstructionStickyStatus(snapshot, "Finished action could not be verified");
    }

    g_constructionActionLocked = false;
    RefreshConstructionUi(g_lastPlayerInterface);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}
} // namespace test_kit
