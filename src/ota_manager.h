#pragma once
// ============================================================================
// OTA Manager — Handles GitHub version checking and firmware downloading
// ============================================================================

#include <Arduino.h>

// Checks GitHub for a new version.
// Returns true if an update is available (and sets outUrl to the binary URL).
bool otaCheckForUpdates(String& outUrl);

// Downloads the firmware from the given URL and flashes it.
// Will restart the ESP32 on success. Returns false on failure.
bool otaPerformUpdate(const String& url);
