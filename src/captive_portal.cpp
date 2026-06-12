#include "captive_portal.h"
#include "config.h"
#include "nvs_store.h"
#include "portal_html.h"
#include "led_animator.h"
#include "neokey_driver.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

// Externs from main.cpp
extern void sysSimulateKeyPress(uint8_t key);
extern int sysGetActivePresetId();
extern uint32_t sysGetPresetColor(uint8_t key);
extern bool sysIsOtaPending();

// ============================================================================
// Captive Portal Implementation
// ============================================================================

static DNSServer  dnsServer;
static WebServer  webServer(80);
static bool       _submitted = false;
static bool       _isBackground = false;

static void handleRoot() {
    webServer.send_P(200, "text/html", PORTAL_HTML);
}

static String escapeJson(const String& s) {
    String out = "";
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

static void handleScanWled() {
    Serial.println("[Portal] Scanning for WLED devices...");
    String json = "[";
    int n = MDNS.queryService("wled", "tcp");
    if (n == 0) n = MDNS.queryService("http", "tcp"); // Fallback
    
    bool first = true;
    for (int i = 0; i < n; i++) {
        String host = MDNS.hostname(i);
        host.toLowerCase();
        if (host.indexOf("wled") >= 0 || MDNS.queryService("wled", "tcp") > 0) {
            if (!first) json += ",";
            json += "{\"name\":\"" + MDNS.hostname(i) + "\",";
            json += "\"ip\":\"" + MDNS.IP(i).toString() + "\"}";
            first = false;
        }
    }
    json += "]";
    webServer.send(200, "application/json", json);
}

static void handleGetConfig() {
    WledConfig cfg;
    loadConfig(cfg);
    String json = "{";
    
    json += "\"ssids\":[";
    for(int i=0; i<cfg.numWifiNetworks; i++) {
        if(i > 0) json += ",";
        json += "\"" + escapeJson(cfg.ssids[i]) + "\"";
    }
    json += "],\"passwords\":[";
    for(int i=0; i<cfg.numWifiNetworks; i++) {
        if(i > 0) json += ",";
        json += "\"" + escapeJson(cfg.passwords[i]) + "\"";
    }
    json += "],\"wleds\":[";
    
    for(int i=0; i<cfg.numWleds; i++) {
        if(i > 0) json += ",";
        json += "\"" + cfg.wledAddresses[i] + "\"";
    }
    json += "],\"actBri\":" + String(cfg.ledActiveBri);
    json += ",\"inaBri\":" + String(cfg.ledInactiveBri);
    json += ",\"fbStyle\":" + String(cfg.ledFeedbackStyle);
    json += ",\"presets\":[";
    for(int i=0; i<4; i++) {
        if(i > 0) json += ",";
        json += String(cfg.presetIds[i]);
    }
    json += "]}";
    webServer.send(200, "application/json", json);
}

static void handleStatus() {
    String json = "{";
    json += "\"connected\":" + String(WiFi.isConnected() ? "true" : "false");
    json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
    json += ",\"fw\":\"" + String(FW_VERSION) + "\"";
    json += ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"uptime\":" + String(millis() / 1000);
    json += ",\"i2c_ok\":" + String(neokeyIsReady() ? "true" : "false");
    json += ",\"activePreset\":" + String(sysGetActivePresetId());
    json += ",\"keyColors\":[";
    for(int i=0; i<4; i++) {
        if(i>0) json += ",";
        json += String(sysGetPresetColor(i));
    }
    json += "]";
    json += ",\"bg\":" + String(_isBackground ? "true" : "false");
    json += ",\"ota\":" + String(sysIsOtaPending() ? "true" : "false");
    json += "}";
    webServer.send(200, "application/json", json);
}

static void handlePress() {
    if (webServer.hasArg("id")) {
        int key = webServer.arg("id").toInt();
        if (key >= 0 && key < 4) {
            sysSimulateKeyPress((uint8_t)key);
            webServer.send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }
    webServer.send(400, "application/json", "{\"ok\":false}");
}

static void handleSave() {
    // Load existing config to avoid wiping untouched fields
    WledConfig cfg;
    loadConfig(cfg);

    String mode = webServer.arg("mode");
    cfg.apMode = (mode == "adhoc");
    
    // Temporary lists for new networks
    String newSsids[5];
    String newPasses[5];
    uint8_t newCount = 0;

    // Read form inputs
    for (int i = 0; i < 5; i++) {
        String ssidArg = "ssid" + String(i);
        String passArg = "pass" + String(i);
        if (webServer.hasArg(ssidArg)) {
            String s = webServer.arg(ssidArg);
            String p = webServer.hasArg(passArg) ? webServer.arg(passArg) : "";
            if (!s.isEmpty()) {
                newSsids[newCount] = s;
                if (p.isEmpty()) {
                    // Retain old password if not provided
                    for (int j = 0; j < cfg.numWifiNetworks; j++) {
                        if (cfg.ssids[j] == s) {
                            p = cfg.passwords[j];
                            break;
                        }
                    }
                }
                newPasses[newCount] = p;
                newCount++;
            }
        }
    }
    
    // Save to cfg
    cfg.numWifiNetworks = newCount;
    for (int i = 0; i < 5; i++) {
        cfg.ssids[i] = (i < newCount) ? newSsids[i] : "";
        cfg.passwords[i] = (i < newCount) ? newPasses[i] : "";
    }
    
    // Clear out old addresses
    for(int i=0; i<MAX_WLED_DEVICES; i++) cfg.wledAddresses[i] = "";
    cfg.numWleds = 0;

    // Read addr0, addr1, etc.
    for (int i = 0; i < MAX_WLED_DEVICES; i++) {
        String argName = "addr" + String(i);
        if (webServer.hasArg(argName)) {
            String val = webServer.arg(argName);
            if (!val.isEmpty()) {
                cfg.wledAddresses[cfg.numWleds++] = val;
            }
        }
    }

    // Brightness sliders
    if (webServer.hasArg("actBri")) {
        cfg.ledActiveBri = (uint8_t)webServer.arg("actBri").toInt();
    }
    if (webServer.hasArg("inaBri")) {
        cfg.ledInactiveBri = (uint8_t)webServer.arg("inaBri").toInt();
    }
    if (webServer.hasArg("fbStyle")) {
        cfg.ledFeedbackStyle = (uint8_t)webServer.arg("fbStyle").toInt();
    }
    for (int i=0; i<4; i++) {
        String argName = "preset" + String(i);
        if (webServer.hasArg(argName)) {
            cfg.presetIds[i] = webServer.arg(argName).toInt();
        }
    }

    // Validate minimums
    if (cfg.numWifiNetworks == 0) {
        webServer.send(400, "text/plain", "At least one SSID is required.");
        return;
    }

    // Save and signal completion
    saveConfig(cfg);

    // Live-apply brightness and style
    animSetBrightness(cfg.ledActiveBri, cfg.ledInactiveBri);
    animSetFeedbackStyle(cfg.ledFeedbackStyle);

    if (_isBackground) {
        // Background mode: return JSON, don't reboot
        webServer.send(200, "application/json", "{\"ok\":true,\"msg\":\"Settings saved.\"}");
    } else {
        // OOBE captive portal mode: show reboot page
        webServer.send_P(200, "text/html", PORTAL_SUCCESS_HTML);
        _submitted = true;
    }
}

static void handleNotFound() {
    if (_isBackground) {
        // In background mode, don't redirect — just 404
        webServer.send(404, "text/plain", "Not found");
        return;
    }
    // Captive portal mode: redirect everything to root
    webServer.sendHeader("Location", "http://192.168.4.1/", true);
    webServer.send(302, "text/plain", "");
}

static void registerEndpoints() {
    webServer.on("/",     HTTP_GET,  handleRoot);
    webServer.on("/config_data", HTTP_GET, handleGetConfig);
    webServer.on("/status", HTTP_GET, handleStatus);
    webServer.on("/api/press", HTTP_GET, handlePress);
    webServer.on("/scan_wled", HTTP_GET, handleScanWled);
    webServer.on("/save", HTTP_POST, handleSave);
    
    // OTA Update Endpoints
    webServer.on("/update", HTTP_POST, []() {
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "application/json", Update.hasError() ? "{\"ok\":false}" : "{\"ok\":true}");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[OTA] Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Success: %u bytes\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    webServer.onNotFound(handleNotFound);
}

void captivePortalRun() {
    Serial.println("[Portal] Starting captive portal...");

    // Stop any existing WiFi connection
    WiFi.disconnect();
    delay(100);

    // Configure AP
    IPAddress apIP(AP_IP_A, AP_IP_B, AP_IP_C, AP_IP_D);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID);

    Serial.printf("[Portal] AP '%s' started at %s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());

    // DNS server: redirect all domains to the AP IP
    dnsServer.start(53, "*", apIP);

    // Start mDNS responder so queryService works locally
    MDNS.begin("wled-commander-setup");

    // Web server routes
    registerEndpoints();
    webServer.begin();

    // animStartAPMode() was called before entering this function.
    // The animUpdate loop below will render the flashing blue pattern.

    // Block here, servicing DNS + HTTP until config is submitted
    _submitted = false;
    while (!_submitted) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        animUpdate();
        delay(2);
    }

    // Give the browser time to render the success page
    unsigned long wait = millis();
    while (millis() - wait < 3000) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        delay(2);
    }

    Serial.println("[Portal] Configuration saved. Rebooting...");
    ESP.restart();
}

void portalStartBackground() {
    _isBackground = true;
    Serial.println("[Portal] Starting background configuration server on port 80");
    registerEndpoints();
    webServer.begin();
}

void portalHandleClient() {
    webServer.handleClient();
}
