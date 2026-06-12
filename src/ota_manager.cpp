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
    http.setTimeout(10000); // 10s timeout for download
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            int contentLength = http.getSize();
            bool canBegin = Update.begin(contentLength);
            
            if (canBegin) {
                Serial.printf("[OTA] Begin OTA (%d bytes). This may take a while...\n", contentLength);
                
                // Keep LED animation smooth while downloading
                size_t written = 0;
                uint8_t buff[1024] = { 0 };
                WiFiClient * stream = http.getStreamPtr();
                
                while(http.connected() && (written < contentLength)) {
                    animUpdate(); // Yield to LED animation
                    
                    size_t size = stream->available();
                    if(size) {
                        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        Update.write(buff, c);
                        written += c;
                    }
                    delay(1); // Yield to ESP task scheduler
                }
                
                if (written == contentLength) {
                    Serial.println("[OTA] Download successfully completed.");
                } else {
                    Serial.printf("[OTA] Download incomplete: %d/%d\n", written, contentLength);
                }
                
                if (Update.end()) {
                    Serial.println("[OTA] Flash write done!");
                    if (Update.isFinished()) {
                        Serial.println("[OTA] Update successfully completed. Rebooting...");
                        delay(500); // Give serial time to flush
                        ESP.restart();
                        return true;
                    } else {
                        Serial.println("[OTA] Update not finished. Something went wrong!");
                    }
                } else {
                    Serial.printf("[OTA] Error Occurred. Error #: %d\n", Update.getError());
                }
            } else {
                Serial.println("[OTA] Not enough space to begin OTA");
            }
        } else {
            Serial.printf("[OTA] HTTP GET failed, code: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("[OTA] Unable to connect to download URL");
    }
    
    return false;
}
