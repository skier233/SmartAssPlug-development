/**
 *   ESP32 SvelteKit
 *
 *   A simple, secure and extensible framework for IoT projects for ESP32 platforms
 *   with responsive Sveltekit front-end built with TailwindCSS and DaisyUI.
 *   https://github.com/theelims/ESP32-sveltekit
 *
 *   Copyright (C) 2018 - 2023 rjwats
 *   Copyright (C) 2023 theelims
 *
 *   All Rights Reserved. This software may be modified and distributed under
 *   the terms of the LGPL v3 license. See the LICENSE file for details.
 **/

#include <WiFiSettingsService.h>

WiFiSettingsService::WiFiSettingsService(AsyncWebServer *server, FS *fs, SecurityManager *securityManager, NotificationEvents *notificationEvents) : _httpEndpoint(WiFiSettings::read, WiFiSettings::update, this, server, WIFI_SETTINGS_SERVICE_PATH, securityManager),
                                                                                                                                                     _fsPersistence(WiFiSettings::read, WiFiSettings::update, this, fs, WIFI_SETTINGS_FILE),
                                                                                                                                                     _lastConnectionAttempt(0),
                                                                                                                                                     _notificationEvents(notificationEvents)
{
    // We want the device to come up in opmode=0 (WIFI_OFF), when erasing the flash this is not the default.
    // If needed, we save opmode=0 before disabling persistence so the device boots with WiFi disabled in the future.
    if (WiFi.getMode() != WIFI_OFF)
    {
        WiFi.mode(WIFI_OFF);
    }

    // Disable WiFi config persistance and auto reconnect
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);

    // Init the wifi driver on ESP32
    WiFi.mode(WIFI_MODE_MAX);
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.onEvent(
        std::bind(&WiFiSettingsService::onStationModeDisconnected, this, std::placeholders::_1, std::placeholders::_2),
        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(std::bind(&WiFiSettingsService::onStationModeStop, this, std::placeholders::_1, std::placeholders::_2),
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_STOP);

    addUpdateHandler([&](const String &originId)
                     { reconfigureWiFiConnection(); },
                     false);
}

void WiFiSettingsService::begin()
{
    _fsPersistence.readFromFS();
    reconfigureWiFiConnection();
}

void WiFiSettingsService::reconfigureWiFiConnection()
{
    // reset last connection attempt to force loop to reconnect immediately
    _lastConnectionAttempt = 0;

    // disconnect and de-configure wifi
    if (WiFi.disconnect(true))
    {
        _stopping = true;
    }
}

void WiFiSettingsService::loop()
{
    unsigned long currentMillis = millis();
    if (!_lastConnectionAttempt || (unsigned long)(currentMillis - _lastConnectionAttempt) >= WIFI_RECONNECTION_DELAY)
    {
        _lastConnectionAttempt = currentMillis;
        manageSTA();
    }

    if (!_lastRssiUpdate || (unsigned long)(currentMillis - _lastRssiUpdate) >= RSSI_EVENT_DELAY)
    {
        _lastRssiUpdate = currentMillis;
        updateRSSI();
    }
}

String WiFiSettingsService::getHostname()
{
    return _state.hostname;
}

void WiFiSettingsService::manageSTA()
{
    // Abort if already connected, or if we have no SSID
    if (WiFi.isConnected() || _state.ssid.length() == 0)
    {
        return;
    }
    // Connect or reconnect as required
    if ((WiFi.getMode() & WIFI_STA) == 0)
    {
        Serial.println(F("Connecting to WiFi."));
        if (_state.staticIPConfig)
        {
            // configure for static IP
            WiFi.config(_state.localIP, _state.gatewayIP, _state.subnetMask, _state.dnsIP1, _state.dnsIP2);
        }
        else
        {
            // configure for DHCP
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        }
        WiFi.setHostname(_state.hostname.c_str());
        // attempt to connect to the network
        WiFi.begin(_state.ssid.c_str(), _state.password.c_str());
    }
}

void WiFiSettingsService::updateRSSI()
{
    // if WiFi is disconnected send disconnect
    if (WiFi.isConnected())
    {
        String rssi = String(WiFi.RSSI());
        _notificationEvents->send(rssi, "rssi", millis());
        // Serial.printf("SSE RSSI: %i \n", rssi.toInt());
    }
    else
    {
        _notificationEvents->send("disconnected", "rssi", millis());
    }
}

void WiFiSettingsService::onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    WiFi.disconnect(true);
}
void WiFiSettingsService::onStationModeStop(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (_stopping)
    {
        _lastConnectionAttempt = 0;
        _stopping = false;
    }
}
