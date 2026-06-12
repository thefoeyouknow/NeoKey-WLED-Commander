#include "wifi_manager.h"
#include "config.h"
#include "led_animator.h"
#include <ESPmDNS.h>
#include <time.h>

// ============================================================================
// Wi-Fi Manager Implementation
// ============================================================================

IPAddress cachedWledIps[MAX_WLED_DEVICES];
uint8_t numCachedIps = 0;

bool wifiConnectSTA(const String& ssid, const String& password,
                    unsigned long timeoutMs) {
    Serial.printf("[WiFi] Connecting to '%s'...\n", ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true); // Erase internal SDK state without shutting down the radio PHY
    delay(100);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // Must be called AFTER begin() when STA is active
    WiFi.setSleep(false); // Prevent sleep-related AUTH_EXPIRE with some routers
    WiFi.setHostname("WLED-Commander"); // Helps with some router DHCP/AUTH rejection bugs

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            Serial.println("[WiFi] Connection timed out.");
            WiFi.disconnect();
            return false;
        }
        delay(10);  // Yield to WiFi stack
        animUpdate(); // Keep KITT running smoothly
    }

    Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    
    // Keep mDNS alive in Infrastructure mode
    if (MDNS.begin("wled-commander")) {
        Serial.println("[mDNS] Responder started (wled-commander.local)");
    } else {
        Serial.println("[mDNS] Failed to start responder.");
    }
    
    // Sync NTP time for OTA snooze timers
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[WiFi] NTP sync initiated.");
    
    return true;
}

IPAddress wifiResolveMDNS(const String& hostname) {
    // Strip ".local" if present
    String name = hostname;
    if (name.endsWith(".local")) {
        name = name.substring(0, name.length() - 6);
    }

    Serial.printf("[mDNS] Resolving '%s.local'...\n", name.c_str());

    IPAddress result = MDNS.queryHost(name, 5000);  // 5s timeout
    if (result == INADDR_NONE) {
        Serial.println("[mDNS] Resolution failed.");
    } else {
        Serial.printf("[mDNS] Resolved to %s\n", result.toString().c_str());
    }
    return result;
}

IPAddress wifiGetPrimaryIP() {
    if (numCachedIps > 0) return cachedWledIps[0];
    return INADDR_NONE;
}

void wifiInvalidateCache() {
    for (int i = 0; i < MAX_WLED_DEVICES; i++) {
        cachedWledIps[i] = INADDR_NONE;
    }
    numCachedIps = 0;
    Serial.println("[WiFi] IP cache invalidated.");
}

bool wifiResolveWledAddresses(const String wledAddresses[], uint8_t count, bool apMode) {
    numCachedIps = 0;
    bool anySuccess = false;

    for (uint8_t i = 0; i < count; i++) {
        if (wledAddresses[i].isEmpty()) continue;

        IPAddress directIP;
        if (directIP.fromString(wledAddresses[i])) {
            Serial.printf("[WiFi] [%d] Using direct IP: %s\n", i, directIP.toString().c_str());
            cachedWledIps[numCachedIps++] = directIP;
            anySuccess = true;
            continue;
        }

        if (apMode) {
            Serial.printf("[WiFi] [%d] AP mode requires direct IP, skipping %s\n", i, wledAddresses[i].c_str());
            continue;
        }

        IPAddress resolved = wifiResolveMDNS(wledAddresses[i]);
        if (resolved != INADDR_NONE) {
            cachedWledIps[numCachedIps++] = resolved;
            anySuccess = true;
        }
    }

    return anySuccess;
}
