#pragma once
// ============================================================================
// NVS Store — Persistent configuration via Preferences.h
// ============================================================================

#include <Arduino.h>

#include "config.h"

struct WledConfig {
    String ssids[5];
    String passwords[5];
    uint8_t numWifiNetworks;
    String wledAddresses[MAX_WLED_DEVICES]; // mDNS name or IP for up to 5 devices
    uint8_t numWleds;                       // Number of bonded WLEDs
    bool   apMode;                          // true = Ad-Hoc (connect to WLED AP)
    bool   autoMapPresets = true;           // If true, automatically fetch 4 lowest presets from WLED
    int    presetIds[4] = {1, 2, 3, 4};     // Dynamically populated or manually mapped preset IDs
    String presetLabels[4];                 // Custom user-defined labels for keys
    uint8_t ledActiveBri;                   // Active key brightness (0-255)
    uint8_t ledInactiveBri;                 // Inactive key brightness (0-255)
    uint8_t ledFeedbackStyle;               // 0 = Solid, 1 = Smooth Fade, 2 = Flash
    uint32_t otaSnoozeUntil;                // UNIX timestamp for 48h snooze
};

extern WledConfig cfg;

// Load saved configuration. Returns false if nothing is stored.
bool loadConfig(WledConfig& cfg);

// Persist configuration to NVS.
void saveConfig(const WledConfig& cfg);

// Wipe all stored configuration (OOBE reset).
void clearConfig();
