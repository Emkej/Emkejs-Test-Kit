#include "test_kit_teleport.h"

#include "test_kit_config.h"
#include "test_kit_panel.h"

#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/MedicalSystem.h>
#include <mygui/MyGUI_InputManager.h>

#include <cmath>
#include <sstream>

namespace test_kit
{
std::vector<size_t> g_filteredSavedLocationIndexes;
std::string g_savedLocationRenameId;
std::string g_savedLocationSearchText;
std::string g_selectedSavedLocationId;
bool g_savedLocationsCollapsed = false;

void RefreshSavedLocationActionButtons(PlayerInterface* player);

namespace
{
const int kDownedTeleportRestoreMinDelayTicks = 5;
const int kDownedTeleportRestoreMaxDelayTicks = 20;
const float kTeleportVectorCompareEpsilon = 0.001f;

struct DownedTeleportState
{
    DownedTeleportState()
        : active(false)
        , proneState(PS_NORMAL)
        , playerWantsMeToGetUp(false)
        , crippled(false)
        , unconscious(false)
        , sub50KO(false)
        , bloodlossTrauma(false)
        , knockoutTimer(0.0f)
    {
    }

    bool active;
    ProneState proneState;
    bool playerWantsMeToGetUp;
    bool crippled;
    bool unconscious;
    bool sub50KO;
    bool bloodlossTrauma;
    float knockoutTimer;
};

struct PendingDownedTeleportRestore
{
    PendingDownedTeleportRestore()
        : character(0)
        , destination(0.0f, 0.0f, 0.0f)
        , ageTicks(0)
        , active(false)
    {
    }

    Character* character;
    std::string actionId;
    std::string targetName;
    DownedTeleportState state;
    Ogre::Vector3 destination;
    int ageTicks;
    bool active;
};

std::vector<PendingDownedTeleportRestore> g_pendingDownedTeleportRestores;

unsigned long long GetCurrentUtcTimestamp()
{
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);

    ULARGE_INTEGER value;
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    return value.QuadPart;
}

bool AreFloatsNearlyEqual(float left, float right)
{
    return std::fabs(left - right) <= kTeleportVectorCompareEpsilon;
}

bool AreVectorsNearlyEqual(const Ogre::Vector3& left, const Ogre::Vector3& right)
{
    return AreFloatsNearlyEqual(left.x, right.x)
        && AreFloatsNearlyEqual(left.y, right.y)
        && AreFloatsNearlyEqual(left.z, right.z);
}

const char* GetProneStateLabel(ProneState proneState)
{
    switch (proneState)
    {
    case PS_NORMAL:
        return "normal";
    case PS_KO:
        return "ko";
    case PS_PLAYING_DEAD:
        return "playing_dead";
    case PS_CRIPPLED:
        return "crippled";
    default:
        return "unknown";
    }
}

void AppendDownedStateLogFields(
    std::stringstream& line,
    const char* prefix,
    const DownedTeleportState& state)
{
    const std::string fieldPrefix = prefix ? prefix : "state";
    line << " " << fieldPrefix << "_active=" << (state.active ? "true" : "false")
         << " " << fieldPrefix << "_prone=\"" << GetProneStateLabel(state.proneState) << "\""
         << " " << fieldPrefix << "_wants_get_up=" << (state.playerWantsMeToGetUp ? "true" : "false")
         << " " << fieldPrefix << "_crippled=" << (state.crippled ? "true" : "false")
         << " " << fieldPrefix << "_unconscious=" << (state.unconscious ? "true" : "false")
         << " " << fieldPrefix << "_sub50ko=" << (state.sub50KO ? "true" : "false")
         << " " << fieldPrefix << "_bloodloss=" << (state.bloodlossTrauma ? "true" : "false")
         << " " << fieldPrefix << "_knockout_timer=" << state.knockoutTimer;
}

bool TryGetDownedTeleportState(Character* character, DownedTeleportState* outState)
{
    if (outState)
    {
        *outState = DownedTeleportState();
    }

    if (!character || !outState)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = character->getMedical();
        if (!medical)
        {
            return false;
        }

        outState->proneState = character->_NV_getProneState();
        outState->playerWantsMeToGetUp = character->playerWantsMeToGetUp;
        outState->crippled = medical->crippled;
        outState->unconscious = medical->unconcious;
        outState->sub50KO = medical->sub50KO;
        outState->bloodlossTrauma = medical->bloodlossTrauma;
        outState->knockoutTimer = medical->knockoutTimer;
        outState->active =
            outState->proneState == PS_KO
            || outState->proneState == PS_PLAYING_DEAD
            || outState->proneState == PS_CRIPPLED
            || outState->unconscious
            || outState->sub50KO
            || outState->crippled
            || outState->bloodlossTrauma
            || outState->knockoutTimer > 0.0f;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool DoesSnapshotRawPositionMatchDestination(
    const CharacterPositionSnapshot& snapshot,
    const Ogre::Vector3& destination)
{
    return snapshot.hasRawPosition && AreVectorsNearlyEqual(snapshot.rawPosition, destination);
}

bool DoesSnapshotBodyPositionMatchDestination(
    const CharacterPositionSnapshot& snapshot,
    const Ogre::Vector3& destination)
{
    if (snapshot.hasPosition && AreVectorsNearlyEqual(snapshot.position, destination))
    {
        return true;
    }

    return snapshot.hasRawEntityPosition && AreVectorsNearlyEqual(snapshot.rawEntityPosition, destination);
}

bool ShouldRetryExactTeleportWithMovement(
    const CharacterPositionSnapshot& beforeSnapshot,
    const CharacterPositionSnapshot& afterSnapshot)
{
    if (!beforeSnapshot.hasPosition || !afterSnapshot.hasPosition)
    {
        return false;
    }

    if (!AreVectorsNearlyEqual(beforeSnapshot.position, afterSnapshot.position))
    {
        return false;
    }

    if (beforeSnapshot.hasRawEntityPosition && afterSnapshot.hasRawEntityPosition)
    {
        return AreVectorsNearlyEqual(beforeSnapshot.rawEntityPosition, afterSnapshot.rawEntityPosition);
    }

    return true;
}

bool TryRetryExactTeleportWithMovement(
    Character* character,
    const Ogre::Vector3& destination,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    __try
    {
        character->teleportVisuallyOnly(destination, character->getOrientation());
        character->teleportFromAnimation();
        character->setTerrainHeightPosition(destination.y);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }

    return true;
}

bool TryRetryExactTeleportWithRelocation(
    Character* character,
    const Ogre::Vector3& moveBy,
    const Ogre::Vector3& destination,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    __try
    {
        character->relocationTeleport(moveBy);
        character->setTerrainHeightPosition(destination.y);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }

    return true;
}

bool TryTemporarilyClearDownedCharacterForTeleport(
    Character* character,
    const DownedTeleportState& downedState)
{
    if (!character || !downedState.active)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = character->getMedical();
        if (!medical)
        {
            return false;
        }

        character->playerWantsMeToGetUp = true;
        if (downedState.proneState != PS_NORMAL)
        {
            character->_NV_setProneState(PS_NORMAL);
        }

        medical->crippled = false;
        medical->unconcious = false;
        medical->sub50KO = false;
        medical->bloodlossTrauma = false;
        medical->knockoutTimer = 0.0f;
        medical->validateHealthValues();
        medical->_reassessRagdollPartsAssumingWeJustClearedTheEntireRagdoll();
        medical->reassessCollapseMode(false, false);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryRestoreDownedCharacterAfterTeleport(
    Character* character,
    const DownedTeleportState& downedState)
{
    if (!character || !downedState.active)
    {
        return false;
    }

    __try
    {
        MedicalSystem* medical = character->getMedical();
        if (!medical)
        {
            return false;
        }

        character->playerWantsMeToGetUp = downedState.playerWantsMeToGetUp;
        medical->crippled = downedState.crippled;
        medical->unconcious = downedState.unconscious;
        medical->sub50KO = downedState.sub50KO;
        medical->bloodlossTrauma = downedState.bloodlossTrauma;
        medical->knockoutTimer = downedState.knockoutTimer;
        character->_NV_setProneState(downedState.proneState);
        medical->validateHealthValues();
        medical->_reassessRagdollPartsAssumingWeJustClearedTheEntireRagdoll();
        medical->reassessCollapseMode(false, downedState.bloodlossTrauma);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

bool TryRetryExactTeleportWithDelayedDownedRestore(
    Character* character,
    const Ogre::Vector3& destination,
    const DownedTeleportState& downedState,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    __try
    {
        if (!TryTemporarilyClearDownedCharacterForTeleport(character, downedState))
        {
            return false;
        }

        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        TryRestoreDownedCharacterAfterTeleport(character, downedState);
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }
    return true;
}

bool TryAdvancePendingDownedTeleport(
    Character* character,
    const Ogre::Vector3& destination,
    const DownedTeleportState& downedState,
    bool* clearAppliedOut,
    bool* teleportCalledOut,
    bool* relocationSyncCalledOut,
    CharacterPositionSnapshot* afterSnapshotOut)
{
    if (clearAppliedOut)
    {
        *clearAppliedOut = false;
    }
    if (teleportCalledOut)
    {
        *teleportCalledOut = false;
    }
    if (relocationSyncCalledOut)
    {
        *relocationSyncCalledOut = false;
    }
    if (afterSnapshotOut)
    {
        *afterSnapshotOut = CharacterPositionSnapshot();
    }

    if (!character)
    {
        return false;
    }

    CharacterPositionSnapshot beforeSnapshot;
    const bool hasBeforeSnapshot = TryGetCharacterPositionSnapshot(character, &beforeSnapshot);
    if (hasBeforeSnapshot && DoesSnapshotBodyPositionMatchDestination(beforeSnapshot, destination))
    {
        if (afterSnapshotOut)
        {
            *afterSnapshotOut = beforeSnapshot;
        }
        return true;
    }

    __try
    {
        if (!TryTemporarilyClearDownedCharacterForTeleport(character, downedState))
        {
            return false;
        }
        if (clearAppliedOut)
        {
            *clearAppliedOut = true;
        }

        character->teleport(destination, character->getOrientation());
        if (teleportCalledOut)
        {
            *teleportCalledOut = true;
        }

        character->relocationTeleport(Ogre::Vector3(0.0f, 0.0f, 0.0f));
        if (relocationSyncCalledOut)
        {
            *relocationSyncCalledOut = true;
        }

        character->setTerrainHeightPosition(destination.y);
        character->setRagdollNavmeshSafePos();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (afterSnapshotOut)
    {
        TryGetCharacterPositionSnapshot(character, afterSnapshotOut);
    }

    return true;
}

void LogDownedTeleportFallbackSelectionInvestigation(
    const char* actionId,
    Character* character,
    const Ogre::Vector3& destination,
    bool observedDowned,
    bool useDownedRagdollSync,
    const CharacterPositionSnapshot& beforeSnapshot,
    const CharacterPositionSnapshot& afterSnapshot,
    const DownedTeleportState& downedState)
{
    if (!g_developerMode || !observedDowned)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][teleport_downed] action=\""
         << SanitizeLogValue(actionId ? actionId : "")
         << "\" phase=\"fallback_selected\""
         << " target_name=\"" << SanitizeLogValue(SafeCharacterName(character)) << "\""
         << " sync_mode=\"" << (useDownedRagdollSync ? "delayed_restore" : "relocation_move") << "\""
         << " destination_x=" << destination.x
         << " destination_y=" << destination.y
         << " destination_z=" << destination.z
         << " before_body_at_destination=" << (DoesSnapshotBodyPositionMatchDestination(beforeSnapshot, destination) ? "true" : "false")
         << " before_raw_at_destination=" << (DoesSnapshotRawPositionMatchDestination(beforeSnapshot, destination) ? "true" : "false")
         << " after_body_at_destination=" << (DoesSnapshotBodyPositionMatchDestination(afterSnapshot, destination) ? "true" : "false")
         << " after_raw_at_destination=" << (DoesSnapshotRawPositionMatchDestination(afterSnapshot, destination) ? "true" : "false");
    AppendDownedStateLogFields(line, "detected", downedState);
    LogInfoLine(line.str());
}

void LogSavedLocationPositionInvestigation(
    const char* actionId,
    const std::string& locationName,
    const std::string& targetName,
    const CharacterPositionSnapshot& snapshot,
    const Ogre::Vector3& selectedPosition,
    const char* selectedSource)
{
    if (!g_developerMode)
    {
        return;
    }

    std::stringstream line;
    line << "[investigate][saved_location] action=\"" << actionId << "\""
         << " location_name=\"" << SanitizeLogValue(locationName) << "\""
         << " target_name=\"" << SanitizeLogValue(targetName) << "\""
         << " selected_source=\"" << SanitizeLogValue(selectedSource ? selectedSource : "unknown") << "\""
         << " selected_x=" << selectedPosition.x
         << " selected_y=" << selectedPosition.y
         << " selected_z=" << selectedPosition.z;
    if (snapshot.hasPosition)
    {
        line << " position_x=" << snapshot.position.x
             << " position_y=" << snapshot.position.y
             << " position_z=" << snapshot.position.z;
    }
    if (snapshot.hasRawEntityPosition)
    {
        line << " raw_entity_x=" << snapshot.rawEntityPosition.x
             << " raw_entity_y=" << snapshot.rawEntityPosition.y
             << " raw_entity_z=" << snapshot.rawEntityPosition.z;
    }
    if (snapshot.hasRawPosition)
    {
        line << " raw_x=" << snapshot.rawPosition.x
             << " raw_y=" << snapshot.rawPosition.y
             << " raw_z=" << snapshot.rawPosition.z;
    }
    if (snapshot.hasTerrainHeight)
    {
        line << " terrain_height=" << snapshot.terrainHeight;
    }
    LogInfoLine(line.str());
}

void FocusCameraOnTeleportedSelection(PlayerInterface* player, const Ogre::Vector3& destination)
{
    if (!player)
    {
        return;
    }

    __try
    {
        if (player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter() != 0)
        {
            player->focusCameraSelectedCharacter();
            return;
        }

        player->focusCamera(destination);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void StopTeleportedSelectionMovement(PlayerInterface* player)
{
    if (!player)
    {
        return;
    }

    __try
    {
        player->stopCharactersMovement();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void LogTeleportInvestigation(
    const char* actionId,
    const std::string& locationName,
    const Ogre::Vector3& requestedDestination,
    const Ogre::Vector3& resolvedDestination,
    bool validSpawnFound)
{
    if (!g_developerMode)
    {
        return;
    }

    const bool destinationAdjusted =
        requestedDestination.x != resolvedDestination.x
        || requestedDestination.y != resolvedDestination.y
        || requestedDestination.z != resolvedDestination.z;

    std::stringstream line;
    line << "[investigate][teleport] action=\"" << actionId << "\""
         << " location_name=\"" << SanitizeLogValue(locationName) << "\""
         << " requested_x=" << requestedDestination.x
         << " requested_y=" << requestedDestination.y
         << " requested_z=" << requestedDestination.z
         << " resolved_x=" << resolvedDestination.x
         << " resolved_y=" << resolvedDestination.y
         << " resolved_z=" << resolvedDestination.z
         << " valid_spawn_found=" << (validSpawnFound ? "true" : "false")
         << " destination_adjusted=" << (destinationAdjusted ? "true" : "false")
         << " delta_x=" << (resolvedDestination.x - requestedDestination.x)
         << " delta_y=" << (resolvedDestination.y - requestedDestination.y)
         << " delta_z=" << (resolvedDestination.z - requestedDestination.z);
    LogInfoLine(line.str());
}

std::string BuildSavedLocationDisplayName(const SavedLocation& location)
{
    if (location.pinned)
    {
        return std::string("[Pinned] ") + location.name;
    }

    return location.name;
}

std::string BuildSavedLocationListEntry(size_t displayIndex, const SavedLocation& location)
{
    std::stringstream entry;
    entry << (displayIndex + 1u) << ". " << BuildSavedLocationDisplayName(location);
    return entry.str();
}

bool DoesSavedLocationMatchSearch(const SavedLocation& location, const std::string& searchUpper)
{
    if (searchUpper.empty())
    {
        return true;
    }

    return ToUpperAscii(location.name).find(searchUpper) != std::string::npos;
}

void ClearSavedLocationRenameState(bool clearInputText)
{
    g_savedLocationRenameId.clear();
    RefreshSaveLocationInputUi();

    if (clearInputText && g_saveLocationNameEdit)
    {
        g_saveLocationNameEdit->setOnlyText("");
    }
}

void BeginSavedLocationRename(const SavedLocation& location)
{
    g_savedLocationRenameId = location.id;
    RefreshSaveLocationInputUi();

    if (g_saveLocationNameEdit)
    {
        g_saveLocationNameEdit->setOnlyText(location.name);
    }
}

void RefreshTeleportSelectionControls(PlayerInterface* player)
{
    if (g_saveSelectedLocationButton)
    {
        g_saveSelectedLocationButton->setEnabled(!g_savedLocationRenameId.empty() || HasPrimarySelectedCharacter(player));
    }

    RefreshSavedLocationActionButtons(player);
}

void SchedulePendingDownedTeleportRestore(
    const char* actionId,
    Character* character,
    const DownedTeleportState& downedState,
    const Ogre::Vector3& destination)
{
    if (!character || !downedState.active)
    {
        return;
    }

    for (std::size_t index = 0; index < g_pendingDownedTeleportRestores.size();)
    {
        if (g_pendingDownedTeleportRestores[index].character == character)
        {
            g_pendingDownedTeleportRestores.erase(g_pendingDownedTeleportRestores.begin() + index);
            continue;
        }

        ++index;
    }

    PendingDownedTeleportRestore pending;
    pending.character = character;
    pending.actionId = actionId ? actionId : "";
    pending.targetName = SafeCharacterName(character);
    pending.state = downedState;
    pending.destination = destination;
    pending.active = true;
    g_pendingDownedTeleportRestores.push_back(pending);
}

std::string NormalizeSavedLocationName(const std::string& name)
{
    return ToUpperAscii(TrimAscii(name));
}

bool DoesSavedLocationNameExist(const std::vector<SavedLocation>& locations, const std::string& candidateName)
{
    const std::string normalizedCandidate = NormalizeSavedLocationName(candidateName);
    if (normalizedCandidate.empty())
    {
        return false;
    }

    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (NormalizeSavedLocationName(locations[index].name) == normalizedCandidate)
        {
            return true;
        }
    }

    return false;
}

bool DoesSavedLocationNameExistExcludingId(
    const std::vector<SavedLocation>& locations,
    const std::string& candidateName,
    const std::string& excludedLocationId)
{
    const std::string normalizedCandidate = NormalizeSavedLocationName(candidateName);
    if (normalizedCandidate.empty())
    {
        return false;
    }

    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (locations[index].id == excludedLocationId)
        {
            continue;
        }

        if (NormalizeSavedLocationName(locations[index].name) == normalizedCandidate)
        {
            return true;
        }
    }

    return false;
}

bool DoesSavedLocationIdExist(const std::vector<SavedLocation>& locations, const std::string& candidateId)
{
    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (locations[index].id == candidateId)
        {
            return true;
        }
    }

    return false;
}

std::string BuildNextSavedLocationId(const std::vector<SavedLocation>& locations)
{
    unsigned int nextNumber = static_cast<unsigned int>(locations.size() + 1u);
    std::string candidateId;
    do
    {
        std::stringstream stream;
        stream << "saved_location_" << nextNumber;
        candidateId = stream.str();
        ++nextNumber;
    }
    while (DoesSavedLocationIdExist(locations, candidateId));

    return candidateId;
}

size_t FindSavedLocationIndexById(const std::vector<SavedLocation>& locations, const std::string& locationId)
{
    for (size_t index = 0; index < locations.size(); ++index)
    {
        if (locations[index].id == locationId)
        {
            return index;
        }
    }

    return locations.size();
}

bool TryGetSelectedSavedLocation(size_t* indexOut, SavedLocation* locationOut)
{
    if (g_selectedSavedLocationId.empty())
    {
        return false;
    }

    const size_t selectedIndex = FindSavedLocationIndexById(g_savedLocations, g_selectedSavedLocationId);
    if (selectedIndex >= g_savedLocations.size())
    {
        return false;
    }

    if (indexOut)
    {
        *indexOut = selectedIndex;
    }
    if (locationOut)
    {
        *locationOut = g_savedLocations[selectedIndex];
    }

    return true;
}

bool TryTeleportSelectedCharactersToCamera(
    PlayerInterface* player,
    const char* actionId,
    const Ogre::Vector3& destinationCenter,
    bool useSpawnValidation,
    int* selectedCountOut,
    int* teleportedCountOut,
    Ogre::Vector3* requestedDestinationOut,
    Ogre::Vector3* resolvedDestinationOut,
    bool* validSpawnFoundOut)
{
    if (selectedCountOut)
    {
        *selectedCountOut = 0;
    }
    if (teleportedCountOut)
    {
        *teleportedCountOut = 0;
    }
    if (requestedDestinationOut)
    {
        *requestedDestinationOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }
    if (resolvedDestinationOut)
    {
        *resolvedDestinationOut = Ogre::Vector3(0.0f, 0.0f, 0.0f);
    }
    if (validSpawnFoundOut)
    {
        *validSpawnFoundOut = false;
    }

    if (!player || (useSpawnValidation && !ou))
    {
        return false;
    }

    __try
    {
        const ogre_unordered_set<hand>::type& selectedCharacters = player->selectedCharacters;
        int selectedCharacterCount = 0;

        ogre_unordered_set<hand>::type::const_iterator countIt = selectedCharacters.begin();
        for (; countIt != selectedCharacters.end(); ++countIt)
        {
            Character* character = countIt->getCharacter();
            if (character)
            {
                ++selectedCharacterCount;
            }
        }

        const bool useSelectedCharacterFallback =
            selectedCharacterCount <= 0 && player->selectedCharacter.isValid() && player->selectedCharacter.getCharacter() != 0;
        if (useSelectedCharacterFallback)
        {
            selectedCharacterCount = 1;
        }

        if (selectedCountOut)
        {
            *selectedCountOut = selectedCharacterCount;
        }

        if (requestedDestinationOut)
        {
            *requestedDestinationOut = destinationCenter;
        }

        Ogre::Vector3 resolvedDestination = destinationCenter;
        bool validSpawnFound = false;
        if (!useSpawnValidation)
        {
            if (ou)
            {
                ou->fixNaNPosition(resolvedDestination);
            }
        }
        else if (ou)
        {
            Ogre::Vector3 validatedDestination = resolvedDestination;
            if (ou->findValidSpawnPos(validatedDestination, resolvedDestination))
            {
                resolvedDestination = validatedDestination;
                validSpawnFound = true;
            }
        }
        if (resolvedDestinationOut)
        {
            *resolvedDestinationOut = resolvedDestination;
        }
        if (validSpawnFoundOut)
        {
            *validSpawnFoundOut = validSpawnFound;
        }

        int teleportedCount = 0;

        ogre_unordered_set<hand>::type::const_iterator teleportIt = selectedCharacters.begin();
        for (; teleportIt != selectedCharacters.end(); ++teleportIt)
        {
            Character* character = teleportIt->getCharacter();
            if (!character)
            {
                continue;
            }

            Ogre::Vector3 referencePosition(0.0f, 0.0f, 0.0f);
            if (!TryGetCharacterTeleportReferencePosition(character, useSpawnValidation, &referencePosition, 0))
            {
                referencePosition = character->getPosition();
            }

            CharacterPositionSnapshot beforeSnapshot;
            const bool capturedBeforeSnapshot =
                !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &beforeSnapshot);

            const bool useAbsoluteTeleportInput = !useSpawnValidation;
            const Ogre::Vector3 moveBy = resolvedDestination - referencePosition;
            const Ogre::Vector3 teleportInput =
                useAbsoluteTeleportInput ? resolvedDestination : moveBy;
            character->teleport(teleportInput, character->getOrientation());
            character->setRagdollNavmeshSafePos();

            CharacterPositionSnapshot afterSnapshot;
            const bool capturedAfterSnapshot =
                !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &afterSnapshot);
            if (!useSpawnValidation
                && capturedBeforeSnapshot
                && capturedAfterSnapshot
                && ShouldRetryExactTeleportWithMovement(beforeSnapshot, afterSnapshot))
            {
                DownedTeleportState downedState;
                const bool observedDowned = TryGetDownedTeleportState(character, &downedState) && downedState.active;
                CharacterPositionSnapshot fallbackSnapshot;
                const bool useDownedRagdollSync = observedDowned
                    && DoesSnapshotRawPositionMatchDestination(afterSnapshot, resolvedDestination);
                LogDownedTeleportFallbackSelectionInvestigation(
                    actionId,
                    character,
                    resolvedDestination,
                    observedDowned,
                    useDownedRagdollSync,
                    beforeSnapshot,
                    afterSnapshot,
                    downedState);
                const bool fallbackApplied = observedDowned
                    ? (useDownedRagdollSync
                        ? TryRetryExactTeleportWithDelayedDownedRestore(character, resolvedDestination, downedState, &fallbackSnapshot)
                        : TryRetryExactTeleportWithRelocation(character, moveBy, resolvedDestination, &fallbackSnapshot))
                    : TryRetryExactTeleportWithMovement(character, resolvedDestination, &fallbackSnapshot);
                if (fallbackApplied && useDownedRagdollSync)
                {
                    SchedulePendingDownedTeleportRestore(actionId, character, downedState, resolvedDestination);
                }
            }
            ++teleportedCount;
        }

        if (useSelectedCharacterFallback)
        {
            Character* character = player->selectedCharacter.getCharacter();
            if (character)
            {
                Ogre::Vector3 referencePosition(0.0f, 0.0f, 0.0f);
                if (!TryGetCharacterTeleportReferencePosition(character, useSpawnValidation, &referencePosition, 0))
                {
                    referencePosition = character->getPosition();
                }

                CharacterPositionSnapshot beforeSnapshot;
                const bool capturedBeforeSnapshot =
                    !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &beforeSnapshot);

                const bool useAbsoluteTeleportInput = !useSpawnValidation;
                const Ogre::Vector3 moveBy = resolvedDestination - referencePosition;
                const Ogre::Vector3 teleportInput =
                    useAbsoluteTeleportInput ? resolvedDestination : moveBy;
                character->teleport(teleportInput, character->getOrientation());
                character->setRagdollNavmeshSafePos();

                CharacterPositionSnapshot afterSnapshot;
                const bool capturedAfterSnapshot =
                    !useSpawnValidation && TryGetCharacterPositionSnapshot(character, &afterSnapshot);
                if (!useSpawnValidation
                    && capturedBeforeSnapshot
                    && capturedAfterSnapshot
                    && ShouldRetryExactTeleportWithMovement(beforeSnapshot, afterSnapshot))
                {
                    DownedTeleportState downedState;
                    const bool observedDowned = TryGetDownedTeleportState(character, &downedState) && downedState.active;
                    CharacterPositionSnapshot fallbackSnapshot;
                    const bool useDownedRagdollSync = observedDowned
                        && DoesSnapshotRawPositionMatchDestination(afterSnapshot, resolvedDestination);
                    LogDownedTeleportFallbackSelectionInvestigation(
                        actionId,
                        character,
                        resolvedDestination,
                        observedDowned,
                        useDownedRagdollSync,
                        beforeSnapshot,
                        afterSnapshot,
                        downedState);
                    const bool fallbackApplied = observedDowned
                        ? (useDownedRagdollSync
                            ? TryRetryExactTeleportWithDelayedDownedRestore(character, resolvedDestination, downedState, &fallbackSnapshot)
                            : TryRetryExactTeleportWithRelocation(character, moveBy, resolvedDestination, &fallbackSnapshot))
                        : TryRetryExactTeleportWithMovement(character, resolvedDestination, &fallbackSnapshot);
                    if (fallbackApplied && useDownedRagdollSync)
                    {
                        SchedulePendingDownedTeleportRestore(actionId, character, downedState, resolvedDestination);
                    }
                }
                ++teleportedCount;
            }
        }

        if (teleportedCountOut)
        {
            *teleportedCountOut = teleportedCount;
        }

        if (teleportedCount > 0)
        {
            StopTeleportedSelectionMovement(player);
            FocusCameraOnTeleportedSelection(player, resolvedDestination);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    return true;
}

void OnSaveSelectedLocationButtonClicked(MyGUI::Widget*)
{
    const bool renameMode = !g_savedLocationRenameId.empty();
    const char* actionId = renameMode ? "rename_saved_location" : "save_selected_location";
    LogActionRequested(actionId);

    if (!g_saveLocationNameEdit)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"missing_input_widget\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(renameMode ? "Rename Location" : "Save Selected Location") + " failed - name input unavailable");
        return;
    }

    const std::string locationName = TrimAscii(g_saveLocationNameEdit->getOnlyText().asUTF8());
    if (locationName.empty())
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"" << actionId << "\" success=false reason=\"empty_name\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(renameMode ? "Rename Location" : "Save Selected Location") + " failed - enter a location name");
        return;
    }

    g_saveLocationNameEdit->setOnlyText(locationName);

    if (renameMode)
    {
        const size_t locationIndex = FindSavedLocationIndexById(g_savedLocations, g_savedLocationRenameId);
        if (locationIndex >= g_savedLocations.size())
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"rename_saved_location\" success=false reason=\"missing_location\""
                   << " location_id=\"" << SanitizeLogValue(g_savedLocationRenameId) << "\"";
            LogInfoLine(result.str());
            ClearSavedLocationRenameState(true);
            RefreshSavedLocationsListWidget();
            RefreshTeleportSelectionControls(g_lastPlayerInterface);
            SetStatusMessage("Rename Location failed - saved location no longer exists");
            return;
        }

        if (DoesSavedLocationNameExistExcludingId(g_savedLocations, locationName, g_savedLocationRenameId))
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"rename_saved_location\" success=false reason=\"duplicate_name\""
                   << " location_id=\"" << SanitizeLogValue(g_savedLocationRenameId) << "\""
                   << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
            LogInfoLine(result.str());
            SetStatusMessage("Rename Location failed - name already exists");
            return;
        }

        std::vector<SavedLocation> updatedLocations = g_savedLocations;
        const std::string previousName = updatedLocations[locationIndex].name;
        updatedLocations[locationIndex].name = locationName;
        SortSavedLocationsForDisplay(&updatedLocations);

        std::string persistError;
        if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
        {
            std::stringstream result;
            result << "event=testkit_action_result action=\"rename_saved_location\" success=false reason=\""
                   << persistError << "\""
                   << " location_id=\"" << SanitizeLogValue(g_savedLocationRenameId) << "\""
                   << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
            LogInfoLine(result.str());
            SetStatusMessage("Rename Location failed - could not persist config");
            return;
        }

        const std::string renamedLocationId = g_savedLocationRenameId;
        g_savedLocations.swap(updatedLocations);
        ClearSavedLocationRenameState(true);
        g_selectedSavedLocationId = renamedLocationId;
        g_savedLocationSearchText.clear();
        if (g_savedLocationSearchEdit)
        {
            g_savedLocationSearchEdit->setOnlyText("");
        }
        RefreshSavedLocationsListWidget();
        RefreshTeleportSelectionControls(g_lastPlayerInterface);

        std::stringstream result;
        result << "event=testkit_action_result action=\"rename_saved_location\" success=true"
               << " location_id=\"" << SanitizeLogValue(renamedLocationId) << "\""
               << " old_name=\"" << SanitizeLogValue(previousName) << "\""
               << " new_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());

        SetStatusMessage(std::string("Renamed location ") + previousName + " to " + locationName);
        return;
    }

    if (DoesSavedLocationNameExist(g_savedLocations, locationName))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\"duplicate_name\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - name already exists");
        return;
    }

    if (!g_lastPlayerInterface)
    {
        LogInfoLine("event=testkit_action_result action=\"save_selected_location\" success=false reason=\"no_player_interface\"");
        SetStatusMessage("Save Selected Location failed - player interface unavailable");
        return;
    }

    Character* selectedCharacter = TryGetPrimarySelectedCharacter(g_lastPlayerInterface);
    if (!selectedCharacter)
    {
        LogInfoLine("event=testkit_action_result action=\"save_selected_location\" success=false reason=\"no_selected_character\"");
        SetStatusMessage("No selected character to save location from");
        return;
    }

    CharacterPositionSnapshot positionSnapshot;
    if (!TryGetCharacterPositionSnapshot(selectedCharacter, &positionSnapshot))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\"position_read_failed\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - selected character position unavailable");
        return;
    }

    Ogre::Vector3 selectedPosition(0.0f, 0.0f, 0.0f);
    const char* selectedSource = "unavailable";
    if (!TryGetCharacterTeleportReferencePosition(selectedCharacter, false, &selectedPosition, &selectedSource))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\"position_read_failed\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - selected character position unavailable");
        return;
    }
    const std::string targetName = SafeCharacterName(selectedCharacter);
    LogSavedLocationPositionInvestigation(
        "save_selected_location",
        locationName,
        targetName,
        positionSnapshot,
        selectedPosition,
        selectedSource);

    std::vector<SavedLocation> updatedLocations = g_savedLocations;
    SavedLocation location;
    location.id = BuildNextSavedLocationId(updatedLocations);
    location.name = locationName;
    location.position = selectedPosition;
    updatedLocations.push_back(location);
    SortSavedLocationsForDisplay(&updatedLocations);

    std::string persistError;
    if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"save_selected_location\" success=false reason=\""
               << persistError << "\""
               << " location_name=\"" << SanitizeLogValue(locationName) << "\""
               << " target_name=\"" << SanitizeLogValue(targetName) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Save Selected Location failed - could not persist config");
        return;
    }

    g_savedLocations.swap(updatedLocations);
    ClearSavedLocationRenameState(true);
    g_selectedSavedLocationId = location.id;
    g_savedLocationSearchText.clear();
    if (g_savedLocationSearchEdit)
    {
        g_savedLocationSearchEdit->setOnlyText("");
    }
    RefreshSavedLocationsListWidget();
    RefreshTeleportSelectionControls(g_lastPlayerInterface);

    std::stringstream result;
    result << "event=testkit_action_result action=\"save_selected_location\" success=true"
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\""
           << " target_name=\"" << SanitizeLogValue(targetName) << "\""
           << " x=" << location.position.x
           << " y=" << location.position.y
           << " z=" << location.position.z
           << " saved_count=" << g_savedLocations.size();
    LogInfoLine(result.str());

    SetStatusMessage(std::string("Saved location ") + location.name + " from " + targetName);
}

void OnSavedLocationsCollapseButtonClicked(MyGUI::Widget*)
{
    g_savedLocationsCollapsed = !g_savedLocationsCollapsed;
    UpdateSavedLocationsCollapseButtonCaption();
    ApplyPanelLayout();
}

void OnSavedLocationTeleportButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "teleport_selected_to_saved_location";
    LogActionRequested(actionId);

    SavedLocation location;
    size_t locationIndex = 0u;
    if (!TryGetSelectedSavedLocation(&locationIndex, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Teleport failed - no saved location selected");
        return;
    }

    if (!g_lastPlayerInterface)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"no_player_interface\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string("Teleport to ") + location.name + " failed - player interface unavailable");
        return;
    }

    int selectedCount = 0;
    int teleportedCount = 0;
    Ogre::Vector3 requestedDestination(0.0f, 0.0f, 0.0f);
    Ogre::Vector3 resolvedDestination(0.0f, 0.0f, 0.0f);
    bool validSpawnFound = false;
    if (!TryTeleportSelectedCharactersToCamera(
            g_lastPlayerInterface,
            "teleport_selected_to_saved_location",
            location.position,
            false,
            &selectedCount,
            &teleportedCount,
            &requestedDestination,
            &resolvedDestination,
            &validSpawnFound))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"apply_failed\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\""
               << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string("Teleport to ") + location.name + " failed - apply path unavailable");
        return;
    }

    if (selectedCount <= 0)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success=false reason=\"no_selection\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\""
               << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string("No selected characters to teleport to ") + location.name);
        return;
    }

    const bool destinationAdjusted =
        requestedDestination.x != resolvedDestination.x
        || requestedDestination.y != resolvedDestination.y
        || requestedDestination.z != resolvedDestination.z;
    LogTeleportInvestigation(
        "teleport_selected_to_saved_location",
        location.name,
        requestedDestination,
        resolvedDestination,
        validSpawnFound);

    if (teleportedCount > 0)
    {
        std::vector<SavedLocation> updatedLocations = g_savedLocations;
        if (locationIndex < updatedLocations.size())
        {
            updatedLocations[locationIndex].lastUsedUtc = GetCurrentUtcTimestamp();
            SortSavedLocationsForDisplay(&updatedLocations);

            std::string persistError;
            if (TryPersistSavedLocationsConfig(updatedLocations, &persistError))
            {
                g_savedLocations.swap(updatedLocations);
                RefreshSavedLocationsListWidget();
                RefreshTeleportSelectionControls(g_lastPlayerInterface);
            }
            else
            {
                std::stringstream persistLine;
                persistLine << "event=testkit_saved_location_recent_persist_failed location_id=\""
                            << SanitizeLogValue(location.id)
                            << "\" reason=\"" << persistError << "\"";
                LogWarnLine(persistLine.str());
            }
        }
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"teleport_selected_to_saved_location\" success="
           << (teleportedCount > 0 ? "true" : "false")
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\""
           << " selected_count=" << selectedCount
           << " teleported_count=" << teleportedCount
           << " requested_x=" << requestedDestination.x
           << " requested_y=" << requestedDestination.y
           << " requested_z=" << requestedDestination.z
           << " destination_x=" << resolvedDestination.x
           << " destination_y=" << resolvedDestination.y
           << " destination_z=" << resolvedDestination.z
           << " valid_spawn_found=" << (validSpawnFound ? "true" : "false")
           << " destination_adjusted=" << (destinationAdjusted ? "true" : "false");
    if (teleportedCount <= 0)
    {
        result << " reason=\"not_observed_after_apply\"";
    }
    LogInfoLine(result.str());

    if (teleportedCount == selectedCount)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " selected character(s) to " << location.name;
        SetStatusMessage(status.str());
    }
    else if (teleportedCount > 0)
    {
        std::stringstream status;
        status << "Teleported " << teleportedCount << " of " << selectedCount << " selected characters to "
               << location.name;
        SetStatusMessage(status.str());
    }
    else
    {
        SetStatusMessage(std::string("Teleport to ") + location.name + " requested - no selected characters moved");
    }

    UpdateTargetInspection(g_lastPlayerInterface);
}

void OnSavedLocationPinButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "toggle_saved_location_pin";
    LogActionRequested(actionId);

    SavedLocation location;
    size_t locationIndex = 0u;
    if (!TryGetSelectedSavedLocation(&locationIndex, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"toggle_saved_location_pin\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Pin failed - no saved location selected");
        return;
    }

    std::vector<SavedLocation> updatedLocations = g_savedLocations;
    const bool pinned = !updatedLocations[locationIndex].pinned;
    updatedLocations[locationIndex].pinned = pinned;
    SortSavedLocationsForDisplay(&updatedLocations);

    std::string persistError;
    if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"toggle_saved_location_pin\" success=false reason=\""
               << persistError << "\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage(std::string(location.pinned ? "Unpin" : "Pin") + " failed - could not persist config");
        return;
    }

    g_savedLocations.swap(updatedLocations);
    RefreshSavedLocationsListWidget();
    RefreshTeleportSelectionControls(g_lastPlayerInterface);

    std::stringstream result;
    result << "event=testkit_action_result action=\"toggle_saved_location_pin\" success=true"
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\""
           << " pinned=" << (pinned ? "true" : "false");
    LogInfoLine(result.str());

    SetStatusMessage(std::string(pinned ? "Pinned location " : "Unpinned location ") + location.name);
}

void OnSavedLocationRenameButtonClicked(MyGUI::Widget*)
{
    SavedLocation location;
    if (!TryGetSelectedSavedLocation(0, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"rename_saved_location\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Rename Location failed - no saved location selected");
        return;
    }

    if (g_savedLocationRenameId == location.id)
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"rename_saved_location\" success=true mode=\"cancel\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        ClearSavedLocationRenameState(true);
        RefreshSavedLocationsListWidget();
        RefreshTeleportSelectionControls(g_lastPlayerInterface);
        SetStatusMessage(std::string("Rename cancelled for ") + location.name);
        return;
    }

    BeginSavedLocationRename(location);
    RefreshSavedLocationsListWidget();
    RefreshTeleportSelectionControls(g_lastPlayerInterface);

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    if (inputManager && g_saveLocationNameEdit)
    {
        inputManager->setKeyFocusWidget(g_saveLocationNameEdit);
    }

    std::stringstream result;
    result << "event=testkit_action_result action=\"rename_saved_location\" success=true mode=\"begin\""
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
    LogInfoLine(result.str());
    SetStatusMessage(std::string("Rename location ") + location.name + " and click Save Rename");
}

void OnSavedLocationDeleteButtonClicked(MyGUI::Widget*)
{
    const char* actionId = "delete_saved_location";
    LogActionRequested(actionId);

    SavedLocation location;
    size_t locationIndex = 0u;
    if (!TryGetSelectedSavedLocation(&locationIndex, &location))
    {
        LogInfoLine("event=testkit_action_result action=\"delete_saved_location\" success=false reason=\"no_saved_location\"");
        SetStatusMessage("Delete Location failed - no saved location selected");
        return;
    }

    std::vector<SavedLocation> updatedLocations = g_savedLocations;
    updatedLocations.erase(updatedLocations.begin() + locationIndex);

    std::string persistError;
    if (!TryPersistSavedLocationsConfig(updatedLocations, &persistError))
    {
        std::stringstream result;
        result << "event=testkit_action_result action=\"delete_saved_location\" success=false reason=\""
               << persistError << "\""
               << " location_id=\"" << SanitizeLogValue(location.id) << "\"";
        LogInfoLine(result.str());
        SetStatusMessage("Delete Location failed - could not persist config");
        return;
    }

    const bool clearedRenameState = g_savedLocationRenameId == location.id;
    g_savedLocations.swap(updatedLocations);
    if (clearedRenameState)
    {
        ClearSavedLocationRenameState(true);
    }
    RefreshSavedLocationsListWidget();
    RefreshTeleportSelectionControls(g_lastPlayerInterface);

    std::stringstream result;
    result << "event=testkit_action_result action=\"delete_saved_location\" success=true"
           << " location_id=\"" << SanitizeLogValue(location.id) << "\""
           << " location_name=\"" << SanitizeLogValue(location.name) << "\"";
    LogInfoLine(result.str());

    SetStatusMessage(std::string("Deleted location ") + location.name);
}
} // namespace

void RefreshSaveLocationInputUi()
{
    if (g_saveLocationNameLabelText)
    {
        g_saveLocationNameLabelText->setCaption(
            g_savedLocationRenameId.empty()
                ? "Location Name (Enter to save)"
                : "Rename Location (Enter to confirm)");
    }

    if (g_saveSelectedLocationButton)
    {
        g_saveSelectedLocationButton->setCaption(g_savedLocationRenameId.empty() ? "Save Selected Location" : "Save Rename");
    }
}

void UpdateSavedLocationsCollapseButtonCaption()
{
    if (!g_savedLocationsCollapseButton)
    {
        return;
    }

    g_savedLocationsCollapseButton->setCaption(g_savedLocationsCollapsed ? "+" : "-");
}

void RefreshSavedLocationActionButtons(PlayerInterface* player)
{
    SavedLocation location;
    const bool hasSelectedLocation = TryGetSelectedSavedLocation(0, &location);
    const bool hasSelectedCharacters = GetSelectedCharacterCount(player) > 0;

    if (g_savedLocationTeleportButton)
    {
        g_savedLocationTeleportButton->setCaption("Teleport");
        g_savedLocationTeleportButton->setEnabled(hasSelectedLocation && hasSelectedCharacters);
    }

    if (g_savedLocationPinButton)
    {
        g_savedLocationPinButton->setCaption((hasSelectedLocation && location.pinned) ? "Unpin" : "Pin");
        g_savedLocationPinButton->setEnabled(hasSelectedLocation);
    }

    if (g_savedLocationRenameButton)
    {
        g_savedLocationRenameButton->setCaption(
            (hasSelectedLocation && g_savedLocationRenameId == location.id) ? "Cancel" : "Rename");
        g_savedLocationRenameButton->setEnabled(hasSelectedLocation);
    }

    if (g_savedLocationDeleteButton)
    {
        g_savedLocationDeleteButton->setCaption("Delete");
        g_savedLocationDeleteButton->setEnabled(hasSelectedLocation);
    }
}

void RefreshSavedLocationsListWidget()
{
    if (!g_savedLocationsListBox || !g_savedLocationsEmptyText)
    {
        return;
    }

    g_filteredSavedLocationIndexes.clear();
    g_savedLocationsListBox->removeAllItems();

    const std::string searchUpper = ToUpperAscii(TrimAscii(g_savedLocationSearchText));
    for (size_t index = 0; index < g_savedLocations.size(); ++index)
    {
        const SavedLocation& location = g_savedLocations[index];
        if (!DoesSavedLocationMatchSearch(location, searchUpper))
        {
            continue;
        }

        g_filteredSavedLocationIndexes.push_back(index);
        g_savedLocationsListBox->addItem(BuildSavedLocationListEntry(g_filteredSavedLocationIndexes.size() - 1u, location));
    }

    if (g_filteredSavedLocationIndexes.empty())
    {
        g_selectedSavedLocationId.clear();
        g_savedLocationsListBox->clearIndexSelected();
        g_savedLocationsListBox->setVisible(false);
        g_savedLocationsEmptyText->setCaption(g_savedLocations.empty() ? "No saved locations yet" : "No matching saved locations");
        g_savedLocationsEmptyText->setVisible(true);
        RefreshTeleportSelectionControls(g_lastPlayerInterface);
        return;
    }

    g_savedLocationsListBox->setVisible(true);
    g_savedLocationsEmptyText->setVisible(false);

    size_t selectedFilteredIndex = MyGUI::ITEM_NONE;
    if (!g_selectedSavedLocationId.empty())
    {
        for (size_t filteredIndex = 0; filteredIndex < g_filteredSavedLocationIndexes.size(); ++filteredIndex)
        {
            if (g_savedLocations[g_filteredSavedLocationIndexes[filteredIndex]].id == g_selectedSavedLocationId)
            {
                selectedFilteredIndex = filteredIndex;
                break;
            }
        }
    }

    if (selectedFilteredIndex == MyGUI::ITEM_NONE)
    {
        selectedFilteredIndex = 0u;
        g_selectedSavedLocationId = g_savedLocations[g_filteredSavedLocationIndexes[selectedFilteredIndex]].id;
    }

    g_savedLocationsListBox->setIndexSelected(selectedFilteredIndex);
    g_savedLocationsListBox->beginToItemAt(selectedFilteredIndex);
    RefreshTeleportSelectionControls(g_lastPlayerInterface);
}

void ClearPendingDownedTeleportRestores()
{
    g_pendingDownedTeleportRestores.clear();
}

void TickPendingDownedTeleportRestores()
{
    for (std::size_t index = 0; index < g_pendingDownedTeleportRestores.size();)
    {
        PendingDownedTeleportRestore& pending = g_pendingDownedTeleportRestores[index];
        if (!pending.active)
        {
            g_pendingDownedTeleportRestores.erase(g_pendingDownedTeleportRestores.begin() + index);
            continue;
        }

        ++pending.ageTicks;
        CharacterPositionSnapshot syncSnapshot;
        bool clearApplied = false;
        bool teleportCalled = false;
        bool relocationSyncCalled = false;
        const bool syncApplied = TryAdvancePendingDownedTeleport(
            pending.character,
            pending.destination,
            pending.state,
            &clearApplied,
            &teleportCalled,
            &relocationSyncCalled,
            &syncSnapshot);
        const bool bodyAtDestination =
            syncApplied && DoesSnapshotBodyPositionMatchDestination(syncSnapshot, pending.destination);

        if (pending.ageTicks < kDownedTeleportRestoreMinDelayTicks)
        {
            ++index;
            continue;
        }

        if (!bodyAtDestination && pending.ageTicks < kDownedTeleportRestoreMaxDelayTicks)
        {
            ++index;
            continue;
        }

        const bool restored = TryRestoreDownedCharacterAfterTeleport(pending.character, pending.state);
        DownedTeleportState restoredState;
        const bool hasRestoredState = TryGetDownedTeleportState(pending.character, &restoredState);
        CharacterPositionSnapshot restoredSnapshot;
        const bool hasRestoredSnapshot = TryGetCharacterPositionSnapshot(pending.character, &restoredSnapshot);
        if (g_developerMode && (!bodyAtDestination || !restored))
        {
            std::stringstream line;
            line << "[investigate][teleport_restore] action=\""
                 << SanitizeLogValue(pending.actionId)
                 << "\" phase=\"final\""
                 << " target_name=\"" << SanitizeLogValue(pending.targetName) << "\""
                 << " tick=" << pending.ageTicks
                 << " restore_reason=\"" << (bodyAtDestination ? "arrived" : "timeout") << "\""
                 << " sync_applied=" << (syncApplied ? "true" : "false")
                 << " clear_applied=" << (clearApplied ? "true" : "false")
                 << " teleport_called=" << (teleportCalled ? "true" : "false")
                 << " relocation_zero_called=" << (relocationSyncCalled ? "true" : "false")
                 << " body_at_destination=" << (bodyAtDestination ? "true" : "false")
                 << " restored=" << (restored ? "true" : "false")
                 << " restored_state_captured=" << (hasRestoredState ? "true" : "false")
                 << " restored_snapshot_captured=" << (hasRestoredSnapshot ? "true" : "false");
            if (hasRestoredState)
            {
                AppendDownedStateLogFields(line, "restored", restoredState);
            }
            if (hasRestoredSnapshot)
            {
                AppendCharacterSnapshotLogFields(line, "restored", restoredSnapshot);
            }
            LogInfoLine(line.str());
        }

        g_pendingDownedTeleportRestores.erase(g_pendingDownedTeleportRestores.begin() + index);
    }
}

void OnSaveLocationNameTextChanged(MyGUI::EditBox*)
{
    RefreshTeleportSelectionControls(g_lastPlayerInterface);
}

void OnSaveLocationNameAccepted(MyGUI::EditBox* sender)
{
    if (!sender)
    {
        return;
    }

    if (!g_saveSelectedLocationButton || !g_saveSelectedLocationButton->getEnabled())
    {
        return;
    }

    if (TrimAscii(sender->getOnlyText().asUTF8()).empty())
    {
        return;
    }

    OnSaveSelectedLocationButtonClicked(0);
}

void OnSaveSelectedLocationButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSaveSelectedLocationButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationsCollapseButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationsCollapseButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationSearchTextChanged(MyGUI::EditBox* sender)
{
    g_savedLocationSearchText = sender ? sender->getOnlyText().asUTF8() : "";
    RefreshSavedLocationsListWidget();
}

void OnSavedLocationsListSelectionChanged(MyGUI::ListBox*, size_t index)
{
    if (index >= g_filteredSavedLocationIndexes.size())
    {
        g_selectedSavedLocationId.clear();
        RefreshSavedLocationActionButtons(g_lastPlayerInterface);
        return;
    }

    const size_t locationIndex = g_filteredSavedLocationIndexes[index];
    if (locationIndex >= g_savedLocations.size())
    {
        g_selectedSavedLocationId.clear();
        RefreshSavedLocationActionButtons(g_lastPlayerInterface);
        return;
    }

    g_selectedSavedLocationId = g_savedLocations[locationIndex].id;
    RefreshSavedLocationActionButtons(g_lastPlayerInterface);
}

void OnSavedLocationTeleportButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationTeleportButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationPinButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationPinButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationRenameButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationRenameButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}

void OnSavedLocationDeleteButtonPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }

    MyGUI::InputManager* inputManager = MyGUI::InputManager::getInstancePtr();
    OnSavedLocationDeleteButtonClicked(0);

    if (inputManager)
    {
        inputManager->resetMouseCaptureWidget();
    }
}
}
