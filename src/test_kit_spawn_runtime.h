#pragma once

#include "test_kit_spawn.h"

namespace test_kit
{
bool TryGetSpawnTargetWithPlayerState(Character* character, bool* outIsWithPlayer);

bool TryGetSpawnTemplateFactionRestrictionMessage(
    Character* target,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    const SpawnTemplateFactionSelection& factionSelection,
    std::string* outMessage);

bool TrySpawnTemplateNearTarget(
    GameData* templateData,
    const std::string& templateName,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    const SpawnTemplateFactionSelection& factionSelection,
    SpawnCreatureAgePreset creatureAgePreset,
    int quantity,
    SpawnTemplateApplyResult* outResult);
}
