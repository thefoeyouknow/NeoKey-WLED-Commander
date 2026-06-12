#pragma once
// ============================================================================
// Captive Portal — WebServer + DNSServer for OOBE Wi-Fi setup
// ============================================================================

#include <Arduino.h>

// Start the captive portal (AP mode). Blocks until saved.
void captivePortalRun();

// Start the web server in background mode (STA mode). Non-blocking.
void portalStartBackground();

// Handle background web client requests. Call in loop().
void portalHandleClient();
