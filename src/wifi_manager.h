#pragma once
// ============================================================================
// Wi-Fi Manager — STA/AP connection and mDNS resolution
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>

// Connect to Wi-Fi in STA mode. Returns true on success.
// Caller should run the KITT animation while this blocks.
bool wifiConnectSTA(const String& ssid, const String& password,
                    unsigned long timeoutMs);

#include "config.h"

// Resolve an mDNS hostname (e.g. "wled-sign") to an IP address.
// The ".local" suffix is stripped if present.
// Returns INADDR_NONE on failure.
IPAddress wifiResolveMDNS(const String& hostname);

// Access the cached IP array
extern IPAddress cachedWledIps[MAX_WLED_DEVICES];
extern uint8_t numCachedIps;

// Get the primary (first) cached WLED IP. Returns INADDR_NONE if not set.
IPAddress wifiGetPrimaryIP();

// Invalidate all cached IPs (triggers re-resolve on next use).
void wifiInvalidateCache();

// Resolve all WLED addresses from config (handles both mDNS and direct IP).
// Stores results in the cache. Returns true if at least one succeeded.
bool wifiResolveWledAddresses(const String wledAddresses[], uint8_t count, bool apMode);
