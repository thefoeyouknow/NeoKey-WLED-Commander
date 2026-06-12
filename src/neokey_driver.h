#pragma once
// ============================================================================
// NeoKey Driver — I2C initialization, NeoPixel control, key event routing
// ============================================================================

#include <Arduino.h>
#include <Adafruit_NeoKey_1x4.h>
#include <seesaw_neopixel.h>

// Callback type for key events (key index 0-3, isPressed true/false)
typedef void (*KeyEventCallback)(uint8_t keyIndex, bool isPressed);

// Initialize I2C bus and NeoKey hardware with retry. Returns false on failure.
bool neokeyInit();

// Register a callback to be invoked on key state change (RISING and FALLING edges).
void neokeySetCallback(KeyEventCallback cb);

// Set a key's NeoPixel color (raw RGB, no brightness scaling).
void neokeySetColor(uint8_t key, uint8_t r, uint8_t g, uint8_t b);

// Push buffered colors to the NeoPixels.
void neokeyShow();

// Poll the seesaw for key events. Call from loop().
void neokeyPoll();

// Read raw key state (true = currently pressed). Used for OOBE reset check.
bool neokeyIsPressed(uint8_t key);

// Scan the I2C bus and log all detected devices. Diagnostic tool.
void neokeyI2CScan();

// Brute-force scan all typical XIAO pins to find the NeoKey
void neokeyI2CScanAll();

// Returns true if the NeoKey was successfully initialized.
bool neokeyIsReady();
