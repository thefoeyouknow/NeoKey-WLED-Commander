#include "neokey_driver.h"
#include "config.h"
#include <Wire.h>

// ============================================================================
// NeoKey Driver Implementation
// ============================================================================

static Adafruit_NeoKey_1x4 neokey;
static KeyEventCallback _userCallback = nullptr;
static bool _ready = false;

// ---------------------------------------------------------------------------
// Seesaw callback — dispatched by neokey.read()
//
// The library's registerCallback expects: NeoKey1x4Callback (*cb)(keyEvent)
// i.e. a function that takes a keyEvent and returns a NeoKey1x4Callback.
// NeoKey1x4Callback is typedef'd as void (*)(keyEvent).
// ---------------------------------------------------------------------------
static NeoKey1x4Callback _seesawCB(keyEvent evt) {
    if (_userCallback) {
        if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
            _userCallback((uint8_t)evt.bit.NUM, true);
        } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
            _userCallback((uint8_t)evt.bit.NUM, false);
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// I2C Bus Scan — diagnostic helper
// ---------------------------------------------------------------------------
void neokeyI2CScan() {
    Serial.println("[I2C] Scanning bus (SDA=" + String(SDA_PIN) +
                   ", SCL=" + String(SCL_PIN) + ")...");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("[I2C]   Found device at 0x%02X", addr);
            if (addr == NEOKEY_ADDR) Serial.print(" <-- NeoKey expected");
            Serial.println();
            found++;
        }
    }
    if (found == 0) {
        Serial.println("[I2C]   No devices found! Check wiring:");
        Serial.println("[I2C]     3V3 -> NeoKey VIN");
        Serial.println("[I2C]     GND -> NeoKey GND");
        Serial.printf("[I2C]     D0 (GPIO %d) -> NeoKey SDA\n", SDA_PIN);
        Serial.printf("[I2C]     D1 (GPIO %d) -> NeoKey SCL\n", SCL_PIN);
        Serial.println("[I2C]   Type 'scanall' to brute-force check all pins.");
    } else {
        Serial.printf("[I2C]   %d device(s) found.\n", found);
    }
}

void neokeyI2CScanAll() {
    Serial.println("\n[I2C] Running DEEP brute-force scan...");
    Serial.println("[I2C] Checking ALL 127 I2C addresses on ALL 11 XIAO digital pins...");
    
    // XIAO ESP32-S3 pins D0-D10 map
    int pins[] = {1, 2, 3, 4, 5, 6, 43, 44, 7, 8, 9};
    const char* pinNames[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10"};
    int numPins = 11;
    bool foundAny = false;

    // Ensure Wire is cleanly stopped before we start brute forcing
    Wire.end();

    for (int i = 0; i < numPins; i++) {
        for (int j = 0; j < numPins; j++) {
            if (i == j) continue; // SDA and SCL can't be the same pin
            
            int sda = pins[i];
            int scl = pins[j];
            
            Wire.begin(sda, scl);
            delay(5); // Give it a tiny moment to stabilize
            
            for (uint8_t addr = 1; addr < 127; addr++) {
                Wire.beginTransmission(addr);
                if (Wire.endTransmission() == 0) {
                    Serial.printf("\n[I2C] !!! SUCCESS !!! Found device at Address 0x%02X on:\n", addr);
                    Serial.printf("[I2C]     %s (GPIO %d) = SDA\n", pinNames[i], sda);
                    Serial.printf("[I2C]     %s (GPIO %d) = SCL\n", pinNames[j], scl);
                    foundAny = true;
                }
            }
            Wire.end();
        }
    }
    
    if (!foundAny) {
        Serial.println("\n[I2C] Deep scan complete. NO DEVICES FOUND ON ANY PIN COMBINATION.");
        Serial.println("[I2C] The ESP32 is completely blind to the NeoKey.");
        Serial.println("[I2C] Potential issues:");
        Serial.println("[I2C]  1. NeoKey isn't receiving 3.3V power (check VIN/GND)");
        Serial.println("[I2C]  2. Jumper wires are broken internally (very common)");
        Serial.println("[I2C]  3. Stemma QT connector is upside down/loose");
        Serial.println("[I2C]  4. Solder joints on the pins are cold/cracked");
    }
    
    // Restore default Wire state
    Wire.begin(SDA_PIN, SCL_PIN);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool neokeyInit() {
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(100);  // Let the I2C bus and seesaw chip stabilize after power-on

    // Run an I2C scan first for diagnostics
    neokeyI2CScan();

    // Retry loop — the seesaw chip can be slow to boot
    for (int attempt = 1; attempt <= NEOKEY_INIT_RETRIES; attempt++) {
        Serial.printf("[NeoKey] Init attempt %d/%d...\n",
                      attempt, NEOKEY_INIT_RETRIES);

        if (neokey.begin(NEOKEY_ADDR)) {
            // Override the library's default brightness (40) — we manage
            // brightness ourselves via channel-wise scaling.
            neokey.pixels.setBrightness(255);

            // Register the internal seesaw callback on all 4 keys
            for (uint8_t i = 0; i < NUM_KEYS; i++) {
                neokey.registerCallback(i, _seesawCB);
            }

            // Start with all LEDs off
            for (uint8_t i = 0; i < NUM_KEYS; i++) {
                neokey.pixels.setPixelColor(i, 0);
            }
            neokey.pixels.show();

            _ready = true;
            Serial.println("[NeoKey] Initialized successfully.");
            return true;
        }

        Serial.printf("[NeoKey] begin() failed (attempt %d/%d)\n",
                      attempt, NEOKEY_INIT_RETRIES);
        if (attempt < NEOKEY_INIT_RETRIES) {
            delay(NEOKEY_RETRY_DELAY_MS);
        }
    }

    Serial.println("[NeoKey] FATAL: All init attempts failed.");
    Serial.println("[NeoKey] Verify wiring and NeoKey I2C address (0x"
                   + String(NEOKEY_ADDR, HEX) + ").");
    return false;
}

void neokeySetCallback(KeyEventCallback cb) {
    _userCallback = cb;
}

void neokeySetColor(uint8_t key, uint8_t r, uint8_t g, uint8_t b) {
    if (key < NUM_KEYS && _ready) {
        neokey.pixels.setPixelColor(key, r, g, b);
    }
}

void neokeyShow() {
    if (_ready) {
        neokey.pixels.show();
    }
}

void neokeyPoll() {
    if (_ready) {
        neokey.read();
    }
}

bool neokeyIsPressed(uint8_t key) {
    if (!_ready || key >= NUM_KEYS) return false;
    // NeoKey 1x4 buttons are on seesaw GPIO pins 4-7 (active LOW, pulled up).
    uint32_t mask = 1UL << (NEOKEY_1X4_BUTTONA + key);
    uint32_t state = neokey.digitalReadBulk(mask);
    return (state & mask) == 0;
}

bool neokeyIsReady() {
    return _ready;
}
