#include "led_animator.h"
#include "neokey_driver.h"
#include <math.h>

// ============================================================================
// LED Animator Implementation
// ============================================================================

static AnimationMode _mode = ANIM_OFF;
static unsigned long _lastStep = 0;
static unsigned long _lastFadeUpdate = 0;

// --- Fade Engine state ---
static float _currentR[NUM_KEYS] = {0};
static float _currentG[NUM_KEYS] = {0};
static float _currentB[NUM_KEYS] = {0};

static float _targetR[NUM_KEYS] = {0};
static float _targetG[NUM_KEYS] = {0};
static float _targetB[NUM_KEYS] = {0};

// Fade speed (brightness units per ms). Higher = faster fade.
static const float FADE_SPEED = 0.5f; 

// --- KITT state ---
static int  _kittPos   = 0;
static int  _kittDir   = 1;

// --- Success state ---
static uint32_t _successColors[NUM_KEYS] = {0};
static int  _successStep  = 0;
static bool _successDone  = false;

// --- Runtime brightness ---
static uint8_t _activeBri   = LED_ACTIVE_BRIGHTNESS;
static uint8_t _inactiveBri = LED_INACTIVE_BRIGHTNESS;
static uint8_t _feedbackStyle = 1; // 0=Solid, 1=Fade, 2=Flash

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline uint8_t unpackR(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t unpackG(uint32_t c) { return (c >>  8) & 0xFF; }
static inline uint8_t unpackB(uint32_t c) { return  c        & 0xFF; }

static void setAllTargetsOff() {
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        _targetR[i] = 0; _targetG[i] = 0; _targetB[i] = 0;
    }
}

static void snapToTargets() {
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        _currentR[i] = _targetR[i];
        _currentG[i] = _targetG[i];
        _currentB[i] = _targetB[i];
        neokeySetColor(i, (uint8_t)_currentR[i], (uint8_t)_currentG[i], (uint8_t)_currentB[i]);
    }
    neokeyShow();
}

static void updateFadeEngine() {
    unsigned long now = millis();
    unsigned long dt = now - _lastFadeUpdate;
    if (dt == 0) return;
    _lastFadeUpdate = now;

    bool changed = false;
    float step = FADE_SPEED * dt;

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        // Red
        if (fabs(_currentR[i] - _targetR[i]) > step) {
            _currentR[i] += (_currentR[i] < _targetR[i]) ? step : -step;
            changed = true;
        } else if (_currentR[i] != _targetR[i]) {
            _currentR[i] = _targetR[i];
            changed = true;
        }

        // Green
        if (fabs(_currentG[i] - _targetG[i]) > step) {
            _currentG[i] += (_currentG[i] < _targetG[i]) ? step : -step;
            changed = true;
        } else if (_currentG[i] != _targetG[i]) {
            _currentG[i] = _targetG[i];
            changed = true;
        }

        // Blue
        if (fabs(_currentB[i] - _targetB[i]) > step) {
            _currentB[i] += (_currentB[i] < _targetB[i]) ? step : -step;
            changed = true;
        } else if (_currentB[i] != _targetB[i]) {
            _currentB[i] = _targetB[i];
            changed = true;
        }

        if (changed) {
            neokeySetColor(i, (uint8_t)_currentR[i], (uint8_t)_currentG[i], (uint8_t)_currentB[i]);
        }
    }

    if (changed) {
        neokeyShow();
    }
}

// ---------------------------------------------------------------------------
// KITT Animation
// ---------------------------------------------------------------------------

static void kittUpdate() {
    unsigned long now = millis();
    if (now - _lastStep < KITT_SPEED_MS) return;
    _lastStep = now;

    // Set target for current pos to bright red
    _targetR[_kittPos] = 255;
    _targetG[_kittPos] = 0;
    _targetB[_kittPos] = 0;
    
    // Snap it instantly for impact, letting it fade down organically
    _currentR[_kittPos] = 255;

    // Advance position
    _kittPos += _kittDir;
    if (_kittPos >= NUM_KEYS) {
        _kittPos = NUM_KEYS - 2;
        _kittDir = -1;
    } else if (_kittPos < 0) {
        _kittPos = 1;
        _kittDir = 1;
    }

    // Set all other targets to 0 so they fade out smoothly
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        if (i != _kittPos) {
            _targetR[i] = 0;
            _targetG[i] = 0;
            _targetB[i] = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// Success Animation
// ---------------------------------------------------------------------------

static void successUpdate() {
    unsigned long now = millis();

    if (_successStep < NUM_KEYS) {
        if (now - _lastStep < SUCCESS_BLINK_MS) return;
        _lastStep = now;

        uint32_t color = _successColors[_successStep];
        _targetR[_successStep] = (unpackR(color) * _activeBri) / 255.0f;
        _targetG[_successStep] = (unpackG(color) * _activeBri) / 255.0f;
        _targetB[_successStep] = (unpackB(color) * _activeBri) / 255.0f;
        
        _successStep++;
    } else if (!_successDone) {
        if (now - _lastStep < SUCCESS_HOLD_MS) return;
        _successDone = true;
        _mode = ANIM_RUNNING;
    }
}

// ---------------------------------------------------------------------------
// AP Mode Animation
// ---------------------------------------------------------------------------

static void apModeUpdate() {
    // Smooth blue breathing
    unsigned long now = millis();
    float wave = (sin(now / 300.0f) + 1.0f) / 2.0f; // 0.0 to 1.0
    float bri = 50 + (205 * wave); // 50 to 255

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        _targetR[i] = 0;
        _targetG[i] = 0;
        _targetB[i] = bri;
    }
}

// ---------------------------------------------------------------------------
// OTA Animations
// ---------------------------------------------------------------------------

static void otaAvailableUpdate() {
    unsigned long now = millis();
    // 2Hz double flash: ON 100ms, OFF 100ms, ON 100ms, OFF 700ms -> total 1000ms
    unsigned long phase = now % 1000;
    
    float bri = 0;
    if (phase < 100) bri = 25.5f; // 10%
    else if (phase < 200) bri = 0;
    else if (phase < 300) bri = 25.5f;
    else bri = 0;

    _targetR[0] = 0;
    _targetG[0] = bri;
    _targetB[0] = bri;
}

static void otaUpdatingUpdate() {
    unsigned long now = millis();

    if (_successStep < NUM_KEYS) {
        if (now - _lastStep < 200) return;
        _lastStep = now;

        _targetR[_successStep] = 0;
        _targetG[_successStep] = 255;
        _targetB[_successStep] = 255;
        
        _successStep++;
    } else {
        // Hold solid cyan
        for (uint8_t i = 0; i < NUM_KEYS; i++) {
            _targetR[i] = 0;
            _targetG[i] = 255;
            _targetB[i] = 255;
        }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void animStartKitt() {
    _mode = ANIM_KITT;
    _kittPos = 0;
    _kittDir = 1;
    _lastStep = 0;
    _lastFadeUpdate = millis();
    setAllTargetsOff();
    snapToTargets();
    Serial.println("[Anim] KITT started.");
}

void animStartSuccess(uint32_t colors[NUM_KEYS]) {
    _mode = ANIM_SUCCESS;
    _successStep = 0;
    _successDone = false;
    _lastStep = millis();
    _lastFadeUpdate = millis();
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        _successColors[i] = colors[i];
    }
    setAllTargetsOff();
    snapToTargets();
    Serial.println("[Anim] Success sequence started.");
}

void animStartRunning() {
    _mode = ANIM_RUNNING;
    _lastFadeUpdate = millis();
    Serial.println("[Anim] Running mode.");
}

void animStartOff() {
    _mode = ANIM_OFF;
    setAllTargetsOff();
    snapToTargets();
}

void animStartAPMode() {
    _mode = ANIM_AP_MODE;
    _lastFadeUpdate = millis();
    Serial.println("[Anim] AP Mode started.");
}

void animStartOtaAvailable() {
    _mode = ANIM_OTA_AVAILABLE;
    _lastFadeUpdate = millis();
    Serial.println("[Anim] OTA Available mode started.");
}

void animStartOtaUpdating() {
    _mode = ANIM_OTA_UPDATING;
    _successStep = 0; // reuse success variables for the sweep
    _successDone = false;
    _lastStep = millis();
    _lastFadeUpdate = millis();
    setAllTargetsOff();
    snapToTargets();
    Serial.println("[Anim] OTA Updating sequence started.");
}

AnimationMode animUpdate() {
    switch (_mode) {
        case ANIM_KITT:
            kittUpdate();
            break;
        case ANIM_SUCCESS:
            successUpdate();
            if (_successDone) return ANIM_SUCCESS;
            break;
        case ANIM_AP_MODE:
            apModeUpdate();
            break;
        case ANIM_OTA_AVAILABLE:
            otaAvailableUpdate();
            break;
        case ANIM_OTA_UPDATING:
            otaUpdatingUpdate();
            break;
        case ANIM_RUNNING:
        case ANIM_OFF:
            break;
    }
    
    // Always run fade engine for smooth transitions
    updateFadeEngine();
    
    return _mode;
}

AnimationMode animCurrentMode() {
    return _mode;
}

void applyRunningState(int activePresetId, 
                       const int presetIds[NUM_KEYS],
                       const uint32_t colors[NUM_KEYS]) {
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        if (_mode == ANIM_OTA_AVAILABLE && i == 0) continue; // Let OTA animation control Key 0
        
        uint8_t bri = (presetIds[i] == activePresetId) ? _activeBri : _inactiveBri;
        _targetR[i] = (unpackR(colors[i]) * bri) / 255.0f;
        _targetG[i] = (unpackG(colors[i]) * bri) / 255.0f;
        _targetB[i] = (unpackB(colors[i]) * bri) / 255.0f;
    }
}

void animTriggerKeyPress(uint8_t key) {
    if (key >= NUM_KEYS) return;
    
    if (_feedbackStyle == 2) {
        // Instant bright white flash (fades back automatically)
        _currentR[key] = 255.0f;
        _currentG[key] = 255.0f;
        _currentB[key] = 255.0f;
        neokeySetColor(key, 255, 255, 255);
        neokeyShow();
    } else if (_feedbackStyle == 0) {
        // Instant solid snap
        snapToTargets();
    }
    // Style 1 (Fade) does nothing extra, lets the engine fade to targets naturally
}

void animSetBrightness(uint8_t active, uint8_t inactive) {
    _activeBri   = active;
    _inactiveBri = inactive;
    Serial.printf("[Anim] Brightness set: active=%d, inactive=%d\n", active, inactive);
}

void animSetFeedbackStyle(uint8_t style) {
    _feedbackStyle = style;
    Serial.printf("[Anim] Feedback style set: %d\n", style);
}
