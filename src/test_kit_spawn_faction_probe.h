#pragma once

#include "test_kit_spawn.h"

namespace test_kit
{
bool ShouldUseSpawnTemplateNaturalFactionProbe();
void LogSpawnTemplateFactionProbe(GameData* templateData, const std::string& templateName, Character* target);
void LogSpawnTemplateFactionProbeComparison(
    GameData* templateData,
    const std::string& templateName,
    Character* spawnedCharacter,
    const char* phase);
}
