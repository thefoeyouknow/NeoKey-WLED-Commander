#include "captive_portal.h"
#include "config.h"
#include "led_animator.h"
#include "neokey_driver.h"
#include "nvs_store.h"
#include "wifi_manager.h"
#include "wled_client.h"
#include "ota_manager.h"
#include <time.h>
#include <Arduino.h>

// ============================================================================
// NeoKey WLED Commander v0.2 — Main Orchestrator
// ============================================================================

// --- Runtime state ---
static WledConfig cfg;
static uint32_t presetColors[NUM_KEYS] = {0};
static int activePresetId = -1;
static unsigned long lastPollTime = 0;
static bool running = false;
static bool presetsFetched = false;

// --- Serial command buffer ---
static char cmdBuf[SERIAL_CMD_BUF_SIZE];
static uint8_t cmdLen = 0;

// --- OTA state ---
static bool _otaPending = false;
static String _otaUrl = "";
static unsigned long _lastOtaCheckMillis = 0;
static bool _firstCheckDone = false;
static unsigned long _key0PressStartMillis = 0;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void processSerialCommand(const char *cmd);
static void printStatus();
static void printHelp();
void sysSimulateKeyPress(uint8_t key);
int sysGetActivePresetId();
uint32_t sysGetPresetColor(uint8_t key);

// ---------------------------------------------------------------------------
// Key event handler — called from NeoKey seesaw callback
// ---------------------------------------------------------------------------
static void onKeyEvent(uint8_t keyIndex, bool isPressed) {
  if (keyIndex >= NUM_KEYS) return;

  if (keyIndex == 0) {
    if (isPressed) {
      _key0PressStartMillis = millis();
      if (_otaPending) return; // Don't trigger WLED if OTA is pending
    } else {
      if (_otaPending) {
        unsigned long duration = millis() - _key0PressStartMillis;
        if (duration < 30) return; // Ignore mechanical bounce
        if (duration >= 1500) {
          Serial.println("[OTA] Key 0 long press detected. Starting update...");
          animStartOtaUpdating();
          if (!otaPerformUpdate(_otaUrl)) {
             _otaPending = false;
             animStartRunning();
          }
        } else {
          Serial.println("[OTA] Key 0 short press detected. Dismissing update for 48h.");
          _otaPending = false;
          time_t nowUnix;
          time(&nowUnix);
          cfg.otaSnoozeUntil = nowUnix + OTA_SNOOZE_SECONDS;
          saveConfig(cfg);
          animStartRunning();
        }
        return; // Consume the key release for OTA
      }
    }
  }

  // Only trigger WLED on RISING edge (isPressed == true)
  if (isPressed) {
    int presetId = cfg.presetIds[keyIndex];
    Serial.printf("[Key] %d pressed -> Preset %d\n", keyIndex, presetId);

    // Instant local LED feedback BEFORE the HTTP call
    activePresetId = presetId;
    applyRunningState(activePresetId, cfg.presetIds, presetColors);
    animTriggerKeyPress(keyIndex);

    // Send command to WLED (non-critical if it fails; poll will re-sync)
    if (numCachedIps > 0) {
      if (!wledActivatePreset(cachedWledIps, numCachedIps, presetId)) {
        wifiInvalidateCache();
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Check for OOBE hardware reset (Key 0 + Key 3 held at boot)
// ---------------------------------------------------------------------------
static bool checkOOBEReset() {
  if (!neokeyIsPressed(0) || !neokeyIsPressed(3)) {
    return false;
  }

  Serial.println("[Boot] Key 0 + Key 3 detected. Hold to factory reset...");

  neokeySetColor(0, 255, 0, 0);
  neokeySetColor(3, 255, 0, 0);
  neokeyShow();

  unsigned long start = millis();
  while (millis() - start < OOBE_RESET_HOLD_MS) {
    if (!neokeyIsPressed(0) || !neokeyIsPressed(3)) {
      Serial.println("[Boot] Reset cancelled — keys released early.");
      neokeySetColor(0, 0, 0, 0);
      neokeySetColor(3, 0, 0, 0);
      neokeyShow();
      return false;
    }
    delay(50);
  }

  Serial.println("[Boot] Factory reset confirmed!");
  clearConfig();
  return true;
}

// ---------------------------------------------------------------------------
// Boot Wi-Fi connection with KITT animation
// ---------------------------------------------------------------------------
static bool connectWithAnimation() {
  animStartKitt();
  animUpdate();

  for (int i = 0; i < cfg.numWifiNetworks; i++) {
    if (cfg.ssids[i].isEmpty())
      continue;
    bool connected =
        wifiConnectSTA(cfg.ssids[i], cfg.passwords[i], WIFI_CONNECT_TIMEOUT_MS);
    if (connected)
      return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Fetch colors from WLED with retry
// ---------------------------------------------------------------------------
static bool fetchColorsWithRetry(int maxRetries) {
  IPAddress ip = wifiGetPrimaryIP();
  if (ip == INADDR_NONE)
    return false;

  for (int attempt = 0; attempt < maxRetries; attempt++) {
    Serial.printf("[WLED] Fetching preset colors (attempt %d/%d)...\n",
                  attempt + 1, maxRetries);

    if (wledFetchPresetColors(ip, cfg.presetIds, presetColors)) {
      return true;
    }

    wifiInvalidateCache();
    if (wifiResolveWledAddresses(cfg.wledAddresses, cfg.numWleds, cfg.apMode)) {
      ip = wifiGetPrimaryIP();
    } else {
      break;
    }

    delay(1000);
    animUpdate();
  }

  return false;
}

// ---------------------------------------------------------------------------
// Background state poll (non-blocking, called from loop)
// ---------------------------------------------------------------------------
static void pollWledState() {
  if (!running)
    return;

  if (!cfg.apMode && WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWifiReconnect = 0;
    if (millis() - lastWifiReconnect > 10000) {
      lastWifiReconnect = millis();
      Serial.println("[WiFi] Connection lost. Attempting background reconnect...");
      WiFi.reconnect();
    }
    return; // Cannot poll WLED without Wi-Fi
  }

  // Check for OTA updates
  if (!_otaPending && !cfg.apMode) {
    unsigned long now = millis();
    if (_lastOtaCheckMillis == 0) _lastOtaCheckMillis = now; // Delay first check
    
    if (now - _lastOtaCheckMillis > OTA_CHECK_INTERVAL_MS || (!_firstCheckDone && now > 10000)) {
        time_t nowUnix;
        time(&nowUnix);
        
        if (nowUnix > 1000000000) { // NTP is synced
            _lastOtaCheckMillis = now;
            _firstCheckDone = true;
            
            if (nowUnix > cfg.otaSnoozeUntil) {
                if (otaCheckForUpdates(_otaUrl)) {
                    _otaPending = true;
                    animStartOtaAvailable();
                }
            }
        }
    }
  }

  if (millis() - lastPollTime > POLL_INTERVAL_MS) {
    lastPollTime = millis();

    IPAddress ip = wifiGetPrimaryIP();
    if (ip == INADDR_NONE) {
      if (wifiResolveWledAddresses(cfg.wledAddresses, cfg.numWleds,
                                   cfg.apMode)) {
        ip = wifiGetPrimaryIP();
      } else {
        return;
      }
    }

    if (!presetsFetched) {
      Serial.println("[Sync] Retrying preset fetch...");
      if (wledFetchPresetColors(ip, cfg.presetIds, presetColors)) {
        presetsFetched = true;

        // Play success animation then enter running state
        animStartSuccess(presetColors);
        // We don't block here in loop(), animUpdate() will handle the
        // transition but we should apply the state right away so it transitions
        // into it.
        animStartRunning();
        activePresetId = wledPollActivePreset(ip);
        if (activePresetId < 0)
          activePresetId = cfg.presetIds[0];
        applyRunningState(activePresetId, cfg.presetIds, presetColors);
      }
      return;
    }

    int polledPreset = wledPollActivePreset(ip);
    if (polledPreset < 0) {
      wifiInvalidateCache();
      return;
    }

    // Re-fetch preset colors every poll to catch live edits
    uint32_t newColors[NUM_KEYS] = {0};
    bool colorsChanged = false;
    if (wledFetchPresetColors(ip, cfg.presetIds, newColors)) {
      for (int i = 0; i < NUM_KEYS; i++) {
        if (newColors[i] != presetColors[i]) {
          colorsChanged = true;
          presetColors[i] = newColors[i];
        }
      }
    }

    if (polledPreset != activePresetId || colorsChanged) {
      if (polledPreset != activePresetId) {
        Serial.printf("[Sync] Active preset: %d -> %d\n", activePresetId,
                      polledPreset);
      }
      if (colorsChanged) {
        Serial.println("[Sync] Preset colors updated.");
      }
      activePresetId = polledPreset;
      applyRunningState(activePresetId, cfg.presetIds, presetColors);
    }
  }
}

// ---------------------------------------------------------------------------
// Serial command processing
// ---------------------------------------------------------------------------
static unsigned long lastSerialRxTime = 0;

static void readSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    lastSerialRxTime = millis();
    if (c == '\n' || c == '\r') {
      if (cmdLen > 0) {
        cmdBuf[cmdLen] = '\0';
        processSerialCommand(cmdBuf);
        cmdLen = 0;
      }
    } else if (cmdLen < SERIAL_CMD_BUF_SIZE - 1) {
      cmdBuf[cmdLen++] = c;
    }
  }

  // Auto-execute if we received data but no newline after 200ms
  if (cmdLen > 0 && (millis() - lastSerialRxTime > 200)) {
    cmdBuf[cmdLen] = '\0';
    processSerialCommand(cmdBuf);
    cmdLen = 0;
  }
}

static void processSerialCommand(const char *cmd) {
  String s = String(cmd);
  s.trim();
  s.toLowerCase();

  if (s == "help" || s == "?") {
    printHelp();
  } else if (s == "status") {
    printStatus();
  } else if (s == "version") {
    Serial.printf("Firmware: v%s\n", FW_VERSION);
  } else if (s == "reset") {
    Serial.println("[Cmd] Factory reset — clearing NVS and rebooting...");
    clearConfig();
    delay(500);
    ESP.restart();
  } else if (s == "reboot") {
    Serial.println("[Cmd] Rebooting...");
    delay(500);
    ESP.restart();
  } else if (s == "scan") {
    neokeyI2CScan();
  } else if (s == "scanall") {
    neokeyI2CScanAll();
  } else if (s == "portal") {
    Serial.println("[Cmd] Entering captive portal...");
    captivePortalRun(); // Never returns
  } else if (s.startsWith("key ")) {
    String kStr = s.substring(4);
    kStr.trim();
    if (kStr.length() > 0 && isDigit(kStr[0])) {
      int k = kStr.toInt();
      if (k >= 0 && k < NUM_KEYS) {
        sysSimulateKeyPress((uint8_t)k);
      } else {
        Serial.printf("[Cmd] Invalid key: %d (0-%d)\n", k, NUM_KEYS - 1);
      }
    } else {
      Serial.printf("[Cmd] Invalid key format: '%s'\n", kStr.c_str());
    }
  } else if (s == "heap") {
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Min free heap: %u bytes\n", ESP.getMinFreeHeap());
  } else {
    Serial.printf("[Cmd] Unknown: '%s' — type 'help'\n", cmd);
  }
}

static void printHelp() {
  Serial.println();
  Serial.println("=== NeoKey WLED Commander — Serial Commands ===");
  Serial.println("  help     Show this help");
  Serial.println("  status   Show device state and diagnostics");
  Serial.println("  version  Show firmware version");
  Serial.println("  key N    Simulate key press (0-3)");
  Serial.println("  scan     Run I2C bus scan");
  Serial.println("  reset    Factory reset (clear NVS + reboot)");
  Serial.println("  reboot   Reboot device");
  Serial.println("  portal   Enter captive portal setup mode");
  Serial.println("  heap     Show free memory");
  Serial.println();
}

static void printStatus() {
  Serial.println();
  Serial.println("=== Device Status ===");
  Serial.printf("  Firmware:     v%s\n", FW_VERSION);
  Serial.printf("  NeoKey:       %s\n",
                neokeyIsReady() ? "OK" : "NOT CONNECTED");
  Serial.printf("  WiFi:         %s\n", WiFi.isConnected()
                                            ? WiFi.localIP().toString().c_str()
                                            : "DISCONNECTED");
  Serial.printf("  WiFi RSSI:    %d dBm\n", WiFi.RSSI());
  Serial.printf("  Mode:         %s\n",
                cfg.apMode ? "Ad-Hoc" : "Infrastructure");
  Serial.print("  SSIDs:        ");
  for (int i = 0; i < cfg.numWifiNetworks; i++) {
    if (i > 0)
      Serial.print(", ");
    Serial.print(cfg.ssids[i]);
  }
  Serial.println();

  Serial.printf("  WLED targets (%d):\n", cfg.numWleds);
  for (int i = 0; i < cfg.numWleds; i++) {
    Serial.printf("    [%d] %s\n", i, cfg.wledAddresses[i].c_str());
  }

  IPAddress ip = wifiGetPrimaryIP();
  Serial.printf("  Primary IP:   %s\n",
                ip != INADDR_NONE ? ip.toString().c_str() : "(unresolved)");
  Serial.printf("  Active preset: %d\n", activePresetId);

  Serial.println("  Key map:");
  for (int i = 0; i < NUM_KEYS; i++) {
    uint8_t r = (presetColors[i] >> 16) & 0xFF;
    uint8_t g = (presetColors[i] >> 8) & 0xFF;
    uint8_t b = presetColors[i] & 0xFF;
    Serial.printf("    Key %d -> Preset %d  color=#%02X%02X%02X%s\n", i,
                  cfg.presetIds[i], r, g, b,
                  (cfg.presetIds[i] == activePresetId) ? "  [ACTIVE]" : "");
  }

  Serial.printf("  Free heap:    %u bytes\n", ESP.getFreeHeap());
  Serial.printf("  Uptime:       %lu s\n", millis() / 1000);
  Serial.println();
}

void sysSimulateKeyPress(uint8_t key) {
  if (!running) {
    Serial.println("[Cmd] Not in running state — cannot simulate keys.");
    return;
  }
  Serial.printf("[Cmd] Simulating Key %d press\n", key);
  onKeyEvent(key, true);
  delay(50);
  onKeyEvent(key, false);
}

int sysGetActivePresetId() {
  return activePresetId;
}

uint32_t sysGetPresetColor(uint8_t key) {
  if (key < NUM_KEYS) return presetColors[key];
  return 0;
}

bool sysIsOtaPending() {
  return _otaPending;
}

// ============================================================================
// setup()
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("========================================");
  Serial.printf(" NeoKey WLED Commander v%s\n", FW_VERSION);
  Serial.println("========================================");
  Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
  Serial.printf("CPU:   %d MHz\n", getCpuFrequencyMhz());
  Serial.printf("Flash: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("Heap:  %d bytes free\n", ESP.getFreeHeap());
  Serial.println("Type 'help' for serial commands.");
  Serial.println();

  // --- 1. Initialize NeoKey hardware ---
  if (!neokeyInit()) {
    Serial.println("[Boot] NeoKey init failed. Entering serial-only mode.");
    Serial.println(
        "[Boot] Use 'scan' to diagnose I2C, 'reset' to factory reset.");
    // Don't halt — allow serial commands for diagnostics
    while (true) {
      readSerialCommands();
      delay(10);
    }
  }

  // --- 2. Check for OOBE hardware reset ---
  if (checkOOBEReset()) {
    captivePortalRun(); // Never returns (reboots)
  }

  // --- 3. Load configuration ---
  if (!loadConfig(cfg)) {
    Serial.println("[Boot] No saved config. Entering setup portal.");
    captivePortalRun(); // Never returns
  }

  // Apply saved brightness levels and styles
  animSetBrightness(cfg.ledActiveBri, cfg.ledInactiveBri);
  animSetFeedbackStyle(cfg.ledFeedbackStyle);

  // --- 4. Start KITT animation and connect to Wi-Fi ---
  unsigned long bootTime = millis();
  if (!connectWithAnimation()) {
    Serial.println("[Boot] WiFi connect failed. Entering setup portal.");
    animStartAPMode();
    captivePortalRun(); // Never returns
  }

  // --- 5. Start Background Portal ---
  // This allows re-configuring WLED targets at any time via
  // wled-commander.local
  portalStartBackground();

  animUpdate();

  // --- 6. Resolve WLED addresses ---
  if (!wifiResolveWledAddresses(cfg.wledAddresses, cfg.numWleds, cfg.apMode)) {
    Serial.println("[Boot] No WLED targets configured. Waiting for web config "
                   "at wled-commander.local");

    // Pulse yellow to indicate waiting for WLED config
    animStartOff();
    for (int i = 0; i < NUM_KEYS; i++)
      neokeySetColor(i, 60, 60, 0);
    neokeyShow();
    return; // running remains false, loop() will serve web client
  }

  animUpdate();

  // --- 7. Fetch preset colors from WLED ---
  presetsFetched = fetchColorsWithRetry(3);
  if (!presetsFetched) {
    Serial.println(
        "[Boot] Failed to fetch colors. Will retry in background...");
    // Pulse orange to indicate waiting for WLED
    animStartOff();
    for (int i = 0; i < NUM_KEYS; i++)
      neokeySetColor(i, 60, 30, 0);
    neokeyShow();
  }

  // --- 7b. Enforce 3-second minimum boot animation ---
  while (millis() - bootTime < 3000) {
    animUpdate();
    delay(10);
  }

  // --- 8. Play success animation ---
  if (presetsFetched) {
    animStartSuccess(presetColors);
    while (animCurrentMode() == ANIM_SUCCESS) {
      animUpdate();
      delay(10);
    }
  }

  // --- 9. Enter running state ---
  animStartRunning();

  if (presetsFetched) {
    IPAddress ip = wifiGetPrimaryIP();
    activePresetId = wledPollActivePreset(ip);
    if (activePresetId < 0)
      activePresetId = cfg.presetIds[0];

    applyRunningState(activePresetId, cfg.presetIds, presetColors);
  }

  neokeySetCallback(onKeyEvent);
  lastPollTime = millis();
  running = true;

  Serial.println("[Boot] Running. Ready for input.");
  printStatus();
}

// ============================================================================
// loop()
// ============================================================================
void loop() {
  // 1. Poll NeoKey for key events (triggers callbacks)
  neokeyPoll();

  // 2. Update animations (no-op in RUNNING mode)
  animUpdate();

  // 3. Background WLED state sync
  pollWledState();

  // 4. Process serial commands
  readSerialCommands();

  // 5. Handle background web requests
  portalHandleClient();

  // Small yield to prevent watchdog
  delay(10);
}