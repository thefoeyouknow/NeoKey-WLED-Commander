#include "ota_manager.h"
#include "config.h"
#include "led_animator.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

bool otaCheckForUpdates(String& outUrl) {
    Serial.println("[OTA] Checking for updates from GitHub...");
    
    WiFiClientSecure client;
    client.setInsecure(); // Do not validate certs to prevent breaking if root CA expires
    
    HTTPClient http;
    http.setTimeout(5000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (http.begin(client, OTA_CHECK_URL)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = http.getString();
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                String version = doc["version"] | "";
                String url = doc["url"] | "";
                
                if (version != "" && version != FW_VERSION) {
                    Serial.printf("[OTA] New version found: %s\n", version.c_str());
                    outUrl = url;
                    http.end();
                    return true;
                } else {
                    Serial.println("[OTA] Already up to date.");
                }
            } else {
                Serial.printf("[OTA] JSON parse failed: %s\n", error.c_str());
            }
        } else {
            Serial.printf("[OTA] HTTP GET failed, code: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("[OTA] Unable to connect to check URL");
    }
    
    return false;
}

bool otaPerformUpdate(const String& url) {
    Serial.printf("[OTA] Downloading update from: %s\n", url.c_str());
    
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.setTimeout(15000); // 15s connect/response timeout
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (!http.begin(client, url)) {
        Serial.println("[OTA] Unable to connect to download URL");
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP GET failed, code: %d\n", httpCode);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    Serial.printf("[OTA] Content-Length: %d\n", contentLength);

    // GitHub may return -1 (chunked). Use UPDATE_SIZE_UNKNOWN so
    // Update.begin() allocates the full OTA partition.
    size_t updateSize = (contentLength > 0) ? (size_t)contentLength : UPDATE_SIZE_UNKNOWN;

    if (!Update.begin(updateSize)) {
        Serial.println("[OTA] Not enough space to begin OTA");
        http.end();
        return false;
    }

    Serial.printf("[OTA] Begin OTA (%s). This may take a while...\n",
                  (contentLength > 0) ? String(contentLength).c_str() : "unknown size");

    size_t written = 0;
    uint8_t buff[1024] = { 0 };
    WiFiClient * stream = http.getStreamPtr();
    unsigned long lastDataTime = millis();
    const unsigned long OTA_DOWNLOAD_TIMEOUT_MS = 60000; // 60s max stall

    while (http.connected() || stream->available()) {
        animUpdate(); // Keep LEDs alive

        size_t avail = stream->available();
        if (avail) {
            int c = stream->readBytes(buff, min(avail, sizeof(buff)));
            size_t w = Update.write(buff, c);
            if (w > 0) {
                written += w;
                lastDataTime = millis(); // Reset stall timer
            }
        } else {
            // No data available right now — check for stall timeout
            if (millis() - lastDataTime > OTA_DOWNLOAD_TIMEOUT_MS) {
                Serial.println("[OTA] Download stalled (timeout). Aborting.");
                Update.abort();
                http.end();
                return false;
            }
            delay(10);
        }

        // If we know the size and we've got it all, stop
        if (contentLength > 0 && written >= (size_t)contentLength) break;
    }

    Serial.printf("[OTA] Downloaded %u bytes.\n", written);

    if (written == 0) {
        Serial.println("[OTA] No data received. Aborting.");
        Update.abort();
        http.end();
        return false;
    }

    if (Update.end(true)) { // true = set size to what was written
        Serial.println("[OTA] Flash write done!");
        if (Update.isFinished()) {
            Serial.println("[OTA] Update successfully completed. Rebooting...");
            http.end();
            delay(500); // Give serial time to flush
            ESP.restart();
            return true; // unreachable, but correct
        } else {
            Serial.println("[OTA] Update not finished. Something went wrong!");
        }
    } else {
        Serial.printf("[OTA] Error Occurred. Error #: %d\n", Update.getError());
    }

    http.end();
    return false;
}
