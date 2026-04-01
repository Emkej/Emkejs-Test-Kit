#pragma once

#include "test_kit_internal.h"

namespace test_kit
{
void PersistCollapsedStateSetting();
void SortSavedLocationsForDisplay(std::vector<SavedLocation>* locations);
void NormalizePanelHeightSettings();
void NormalizePanelVisualSettings();
bool TryPersistSavedLocationsConfig(const std::vector<SavedLocation>& locations, std::string* outError);
void LoadConfig();
void EnsureModHubClientConfigured();
void StartModHubClient();
void TickModHubAttachRetry();
}
