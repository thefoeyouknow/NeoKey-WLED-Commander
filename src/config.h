#pragma once
// ============================================================================
// NeoKey WLED Commander — Compile-time Configuration
// ============================================================================

#define FW_VERSION              "1.06"

// ---------------------------------------------------------------------------
// NeoKey Hardware
// ---------------------------------------------------------------------------
#define NEOKEY_ADDR             0x30    // Default I2C address
#define NUM_KEYS                4
// If colors are swapped (e.g., Red shows as Green), set to 1.
// (Macro removed to fix color mapping)

// ---------------------------------------------------------------------------
// I2C Pin Routing (XIAO ESP32-S3 → NeoKey 1x4)
// Yellow (SCL) -> D2 (GPIO 3)
// Blue (SDA) -> D1 (GPIO 2)
// ---------------------------------------------------------------------------
#define SDA_PIN                 2       // GPIO 2 (D1)
#define SCL_PIN                 3       // GPIO 3 (D2)



// ---------------------------------------------------------------------------
// WLED Network Configuration
// ---------------------------------------------------------------------------
#define MAX_WLED_DEVICES        5

// ---------------------------------------------------------------------------
// WLED Preset Mapping
// ---------------------------------------------------------------------------
#define PRESET_COUNT            4
#define DEFAULT_PRESET_IDS      {0, 0, 0, 0}   // Initially 0, dynamically populated

// ---------------------------------------------------------------------------
// NeoPixel Brightness (0-255)
// ---------------------------------------------------------------------------
#define LED_INACTIVE_BRIGHTNESS 26      // 10%
#define LED_ACTIVE_BRIGHTNESS   191     // 75%

// ---------------------------------------------------------------------------
// Animation Timing (ms)
// ---------------------------------------------------------------------------
#define KITT_SPEED_MS           200     // KITT sweep step interval (5fps)
#define KITT_FADE_FACTOR        3       // Trail dimming divisor
#define SUCCESS_BLINK_MS        250     // Per-key reveal delay
#define SUCCESS_HOLD_MS         500     // Hold after all keys lit

// ---------------------------------------------------------------------------
// Network Timing (ms)
// ---------------------------------------------------------------------------
#define WIFI_CONNECT_TIMEOUT_MS 25000   // Max wait for STA association
#define MDNS_RETRY_MS           10000   // Cooldown before mDNS re-resolve
#define HTTP_CONNECT_TIMEOUT_MS 3000
#define HTTP_RESPONSE_TIMEOUT_MS 5000
#define POLL_INTERVAL_MS        5000    // Background WLED state poll

// ---------------------------------------------------------------------------
// OOBE / Captive Portal
// ---------------------------------------------------------------------------
#define AP_SSID                 "WLED-Key-Setup"
#define AP_IP_A                 192
#define AP_IP_B                 168
#define AP_IP_C                 4
#define AP_IP_D                 1
#define OOBE_RESET_HOLD_MS      3000    // Key 0+3 hold time for NVS wipe

// ---------------------------------------------------------------------------
// NVS (Non-Volatile Storage)
// ---------------------------------------------------------------------------
#define NVS_NAMESPACE           "wledcmd"

// ---------------------------------------------------------------------------
// I2C Diagnostics
// ---------------------------------------------------------------------------
#define NEOKEY_INIT_RETRIES     3       // Retry neokey.begin() on failure
#define NEOKEY_RETRY_DELAY_MS   500     // Delay between init retries

// ---------------------------------------------------------------------------
// Serial Command Buffer
// ---------------------------------------------------------------------------
#define SERIAL_CMD_BUF_SIZE     64

// ---------------------------------------------------------------------------
// Over-The-Air (OTA) Updates
// ---------------------------------------------------------------------------
#define OTA_CHECK_URL           "https://raw.githubusercontent.com/thefoeyouknow/NeoKey-WLED-Commander/main/version.json"
#define OTA_CHECK_INTERVAL_MS   (24UL * 60UL * 60UL * 1000UL) // 24 hours
#define OTA_SNOOZE_SECONDS      (48UL * 60UL * 60UL)          // 48 hours
