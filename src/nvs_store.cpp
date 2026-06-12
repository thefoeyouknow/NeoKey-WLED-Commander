#include "nvs_store.h"
#include "config.h"
#include <Preferences.h>

// ============================================================================
// NVS Store Implementation
// ============================================================================

static Preferences prefs;

bool loadConfig(WledConfig& cfg) {
    prefs.begin(NVS_NAMESPACE, true);  // read-only

    cfg.numWifiNetworks = prefs.getUChar("numWifi", 0);
    
    // Migration: If no new networks saved, check for legacy 'ssid' key
    if (cfg.numWifiNetworks == 0) {
        String legacySsid = prefs.getString("ssid", "");
        if (!legacySsid.isEmpty()) {
            cfg.ssids[0] = legacySsid;
            cfg.passwords[0] = prefs.getString("pass", "");
            cfg.numWifiNetworks = 1;
        } else {
            prefs.end();
            return false;  // No config stored
        }
    } else {
        // Load the networks
        for (int i = 0; i < cfg.numWifiNetworks; i++) {
            char keyS[8], keyP[8];
            snprintf(keyS, sizeof(keyS), "ssid%d", i);
            snprintf(keyP, sizeof(keyP), "pass%d", i);
            cfg.ssids[i] = prefs.getString(keyS, "");
            cfg.passwords[i] = prefs.getString(keyP, "");
        }
    }

    cfg.apMode      = prefs.getBool("apMode", false);
    
    cfg.numWleds = prefs.getUChar("numWleds", 0);
    if (cfg.numWleds > MAX_WLED_DEVICES) cfg.numWleds = MAX_WLED_DEVICES;

    for (int i = 0; i < cfg.numWleds; i++) {
        char key[16];
        snprintf(key, sizeof(key), "wledAddr%d", i);
        cfg.wledAddresses[i] = prefs.getString(key, "");
    }

    cfg.autoMapPresets = prefs.getBool("autoMap", true);
    
    int defaults[] = DEFAULT_PRESET_IDS;
    for (int i = 0; i < PRESET_COUNT; i++) {
        char key[8], lblKey[8];
        snprintf(key, sizeof(key), "pid%d", i);
        snprintf(lblKey, sizeof(lblKey), "lbl%d", i);
        cfg.presetIds[i] = prefs.getInt(key, defaults[i]);
        cfg.presetLabels[i] = prefs.getString(lblKey, "");
    }

    cfg.ledActiveBri   = prefs.getUChar("actBri",  LED_ACTIVE_BRIGHTNESS);
    cfg.ledInactiveBri = prefs.getUChar("inaBri",  LED_INACTIVE_BRIGHTNESS);
    cfg.ledFeedbackStyle = prefs.getUChar("fbStyle", 1);
    cfg.otaSnoozeUntil = prefs.getUInt("snooze", 0);

    prefs.end();

    Serial.printf("[NVS] Loaded config: numWifi=%d, numWleds=%d, AP=%d, actBri=%d, inaBri=%d\n",
                  cfg.numWifiNetworks, cfg.numWleds, cfg.apMode,
                  cfg.ledActiveBri, cfg.ledInactiveBri);
    return true;
}

void saveConfig(const WledConfig& cfg) {
    prefs.begin(NVS_NAMESPACE, false);  // read-write

    prefs.putUChar("numWifi",   cfg.numWifiNetworks);
    for (int i = 0; i < 5; i++) {
        char keyS[8], keyP[8];
        snprintf(keyS, sizeof(keyS), "ssid%d", i);
        snprintf(keyP, sizeof(keyP), "pass%d", i);
        if (i < cfg.numWifiNetworks) {
            prefs.putString(keyS, cfg.ssids[i]);
            prefs.putString(keyP, cfg.passwords[i]);
        } else {
            prefs.remove(keyS);
            prefs.remove(keyP);
        }
    }
    prefs.putBool("apMode",     cfg.apMode);
    prefs.putUChar("numWleds",  cfg.numWleds);

    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "wledAddr%d", i);
        if (i < cfg.numWleds) {
            prefs.putString(key, cfg.wledAddresses[i]);
        } else {
            prefs.remove(key);
        }
    }

    prefs.putBool("autoMap", cfg.autoMapPresets);

    for (int i = 0; i < PRESET_COUNT; i++) {
        char key[8], lblKey[8];
        snprintf(key, sizeof(key), "pid%d", i);
        snprintf(lblKey, sizeof(lblKey), "lbl%d", i);
        prefs.putInt(key, cfg.presetIds[i]);
        prefs.putString(lblKey, cfg.presetLabels[i]);
    }

    prefs.putUChar("actBri",  cfg.ledActiveBri);
    prefs.putUChar("inaBri",  cfg.ledInactiveBri);
    prefs.putUChar("fbStyle", cfg.ledFeedbackStyle);
    prefs.putUInt("snooze",   cfg.otaSnoozeUntil);

    prefs.end();

    Serial.printf("[NVS] Saved config: numWifi=%d, numWleds=%d, actBri=%d, inaBri=%d\n",
                  cfg.numWifiNetworks, cfg.numWleds,
                  cfg.ledActiveBri, cfg.ledInactiveBri);
}

void clearConfig() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    Serial.println("[NVS] Configuration cleared.");
}
