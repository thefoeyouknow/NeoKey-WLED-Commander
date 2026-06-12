#pragma once
// ============================================================================
// WLED Client — HTTP interface to WLED JSON API
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Fetch the lowest 4 presets dynamically and retrieve their primary colors.
// presetIdsOut[] receives the discovered preset IDs.
// colorsOut[] receives packed 0xRRGGBB values.
// Returns true on success.
bool wledFetchPresetColors(IPAddress ip, int presetIdsOut[PRESET_COUNT],
                           uint32_t colorsOut[PRESET_COUNT],
                           String presetLabelsOut[PRESET_COUNT],
                           bool autoMap);

// Activate a WLED preset by ID across multiple devices.
// Returns true if at least one HTTP request succeeded.
bool wledActivatePreset(IPAddress ips[], int numIps, int presetId);

// Poll the currently active preset from WLED.
// Returns the preset ID, or -1 on failure / no preset active.
int wledPollActivePreset(IPAddress ip);
