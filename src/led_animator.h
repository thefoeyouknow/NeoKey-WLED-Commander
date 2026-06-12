#pragma once
// ============================================================================
// LED Animator — KITT sweep, success reveal, and running-state brightness
// ============================================================================

#include <Arduino.h>
#include "config.h"

enum AnimationMode {
    ANIM_OFF,       // All LEDs dark
    ANIM_KITT,      // White sweep back-and-forth
    ANIM_SUCCESS,   // Sequential color reveal
    ANIM_RUNNING,   // Static preset colors at 25%/75%
    ANIM_AP_MODE,   // 2Hz Blue flash for setup mode
    ANIM_OTA_AVAILABLE, // 10% Cyan double flash on Key 0
    ANIM_OTA_UPDATING   // Cyan sweep and hold
};

// Start the KITT "thinking" animation.
void animStartKitt();

// Start the success reveal. Pass the 4 preset colors as packed 0xRRGGBB.
void animStartSuccess(uint32_t colors[NUM_KEYS]);

// Transition to running mode (static, managed by applyRunningState).
void animStartRunning();

// Turn all LEDs off.
void animStartOff();

// Start AP Mode animation (flashing blue).
void animStartAPMode();

// Start OTA Available animation (cyan double flash on key 0).
void animStartOtaAvailable();

// Start OTA Updating animation (cyan sweep).
void animStartOtaUpdating();

// Update animation state. Call every loop() iteration.
// Returns the current animation mode.
AnimationMode animUpdate();

// Apply the running-state LED pattern (sets targets for fading).
// activePresetId: the WLED preset currently active (-1 if unknown).
// presetIds[]: the WLED preset IDs mapped to keys 0-3.
// colors[]: packed 0xRRGGBB for each key.
void applyRunningState(int activePresetId, 
                       const int presetIds[NUM_KEYS],
                       const uint32_t colors[NUM_KEYS]);

// Trigger interactive key feedback (behavior depends on style config).
void animTriggerKeyPress(uint8_t key);

// Get the current animation mode.
AnimationMode animCurrentMode();

// Set runtime brightness levels for active/inactive keys (0-255).
void animSetBrightness(uint8_t active, uint8_t inactive);
void animSetFeedbackStyle(uint8_t style);
