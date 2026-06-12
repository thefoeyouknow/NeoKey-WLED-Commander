#include "wled_client.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <algorithm>
#include <vector>

// ============================================================================
// WLED Client Implementation
// ============================================================================

// ---------------------------------------------------------------------------
// fetchPresetColors — GET /json/presets, extract 4 lowest presets + colors
// ---------------------------------------------------------------------------
bool wledFetchPresetColors(IPAddress ip, int presetIdsOut[PRESET_COUNT],
                           uint32_t colorsOut[PRESET_COUNT],
                           String presetLabelsOut[PRESET_COUNT],
                           bool autoMap) {

  String url = "http://" + ip.toString() + "/presets.json";
  Serial.printf("[WLED] Fetching presets from %s\n", url.c_str());

  HTTPClient http;
  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
  http.setTimeout(HTTP_RESPONSE_TIMEOUT_MS);

  if (!http.begin(url)) {
    Serial.println("[WLED] HTTP begin() failed.");
    return false;
  }

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[WLED] GET /presets.json returned %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[WLED] JSON parse error: %s\n", err.c_str());
    return false;
  }

  JsonObject root = doc.as<JsonObject>();
  
  if (autoMap) {
    std::vector<int> allPresets;
    for (JsonPair kv : root) {
      int id = String(kv.key().c_str()).toInt();
      if (id > 0) { // WLED user presets start at 1
        allPresets.push_back(id);
      }
    }

    if (!allPresets.empty()) {
      std::sort(allPresets.begin(), allPresets.end());
      for (int i = 0; i < PRESET_COUNT; i++) {
        if (i < allPresets.size()) {
          presetIdsOut[i] = allPresets[i];
        } else {
          presetIdsOut[i] = allPresets.back();
        }
      }
    }
  }

  // Now extract their colors and labels

  // Now extract their colors
  for (int i = 0; i < PRESET_COUNT; i++) {
    char idStr[8];
    snprintf(idStr, sizeof(idStr), "%d", presetIdsOut[i]);

    JsonObject preset = doc[idStr];
    if (preset.isNull()) {
      colorsOut[i] = 0xFFFFFF;
      if (presetLabelsOut[i].isEmpty()) presetLabelsOut[i] = "P" + String(presetIdsOut[i]);
      continue;
    }

    // Assign label from WLED if not customized by user
    if (presetLabelsOut[i].isEmpty() && preset["n"].is<const char*>()) {
      presetLabelsOut[i] = preset["n"].as<String>();
    } else if (presetLabelsOut[i].isEmpty()) {
      presetLabelsOut[i] = "P" + String(presetIdsOut[i]);
    }

    JsonArray col;
    if (preset["seg"].is<JsonArray>()) {
      JsonArray segs = preset["seg"].as<JsonArray>();
      if (segs.size() > 0 && segs[0]["col"].is<JsonArray>()) {
        col = segs[0]["col"].as<JsonArray>();
      }
    } else if (preset["seg"].is<JsonObject>()) {
      JsonObject seg = preset["seg"].as<JsonObject>();
      if (seg["col"].is<JsonArray>()) {
        col = seg["col"].as<JsonArray>();
      }
    }

    if (col.isNull() || col.size() == 0) {
      colorsOut[i] = 0xFFFFFF;
      continue;
    }

    JsonArray rgb = col[0].as<JsonArray>();
    uint8_t r = rgb[0] | 0;
    uint8_t g = rgb[1] | 0;
    uint8_t b = rgb[2] | 0;

    colorsOut[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    Serial.printf("[WLED] Key %d -> Lowest Preset %d: color=%02X%02X%02X\n", i,
                  presetIdsOut[i], r, g, b);
  }

  return true;
}

// ---------------------------------------------------------------------------
// activatePreset — POST {"ps": id} to /json/state across multiple IPs
// ---------------------------------------------------------------------------
bool wledActivatePreset(IPAddress ips[], int numIps, int presetId) {
  if (numIps == 0)
    return false;

  bool anySuccess = false;
  char body[32];
  snprintf(body, sizeof(body), "{\"ps\":%d}", presetId);

  for (int i = 0; i < numIps; i++) {
    String url = "http://" + ips[i].toString() + "/json/state";

    bool successForThisIp = false;
    for (int attempt = 0; attempt < 3; attempt++) {
      HTTPClient http;
      // Use very short timeouts so broadcasting doesn't hang the UX
      http.setConnectTimeout(500);
      http.setTimeout(1000);

      if (http.begin(url)) {
        http.addHeader("Content-Type", "application/json");
        int code = http.POST((uint8_t *)body, strlen(body));
        http.end();

        // 200 is success. -11 is Read Timeout (which means WLED received the
        // command but took too long to reply — the preset still applied
        // successfully!).
        if (code == 200 || code == HTTPC_ERROR_READ_TIMEOUT) {
          successForThisIp = true;
          anySuccess = true;
          break;
        } else {
          Serial.printf("[WLED] Activate preset %d on %s failed (attempt %d): HTTP %d\n",
                        presetId, ips[i].toString().c_str(), attempt + 1, code);
        }
      } else {
        Serial.printf("[WLED] Could not connect to %s (attempt %d)\n",
                      ips[i].toString().c_str(), attempt + 1);
      }
      
      if (attempt < 2) delay(100);
    }
    
    if (!successForThisIp) {
        Serial.printf("[WLED] Completely failed to reach %s after 3 attempts.\n", ips[i].toString().c_str());
    }
  }

  if (anySuccess) {
    Serial.printf("[WLED] Broadcasted preset %d to %d devices\n", presetId,
                  numIps);
  }
  return anySuccess;
}

// ---------------------------------------------------------------------------
// pollActivePreset — GET /json/state, return state.ps
// ---------------------------------------------------------------------------
int wledPollActivePreset(IPAddress ip) {
  String url = "http://" + ip.toString() + "/json/state";

  HTTPClient http;
  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
  http.setTimeout(HTTP_RESPONSE_TIMEOUT_MS);

  if (!http.begin(url)) {
    return -1;
  }

  int code = http.GET();
  if (code != 200) {
    http.end();
    return -1;
  }

  // Use a filter to only parse the "ps" field — much lighter than full parse
  JsonDocument filter;
  filter["ps"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("[WLED] Poll parse error: %s\n", err.c_str());
    return -1;
  }

  int ps = doc["ps"] | -1;
  return ps;
}
