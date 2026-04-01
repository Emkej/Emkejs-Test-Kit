#pragma once

#include "test_kit_spawn.h"

namespace test_kit
{
bool TrySpawnTemplateNearTarget(
    GameData* templateData,
    const std::string& templateName,
    Character* target,
    SpawnTemplateRadiusPreset radiusPreset,
    SpawnTemplateSquadMode squadMode,
    SpawnTemplateAllegiance allegiance,
    SpawnCreatureAgePreset creatureAgePreset,
    int quantity,
    SpawnTemplateApplyResult* outResult);
}
