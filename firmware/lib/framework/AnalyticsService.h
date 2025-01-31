#pragma once

/**
 *   ESP32 SvelteKit
 *
 *   A simple, secure and extensible framework for IoT projects for ESP32 platforms
 *   with responsive Sveltekit front-end built with TailwindCSS and DaisyUI.
 *   https://github.com/theelims/ESP32-sveltekit
 *
 *   Copyright (C) 2023 theelims
 *
 *   All Rights Reserved. This software may be modified and distributed under
 *   the terms of the LGPL v3 license. See the LICENSE file for details.
 **/

#include <WiFi.h>
#include <AsyncTCP.h>

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPFS.h>
#include <NotificationEvents.h>

#define MAX_ESP_ANALYTICS_SIZE 1024
#define ANALYTICS_INTERVAL 2000

class AnalyticsService
{
public:
    AnalyticsService(NotificationEvents *notificationEvents) : _notificationEvents(notificationEvents){};

    void begin()
    {
        xTaskCreatePinnedToCore(
            this->_loopImpl,            // Function that should be called
            "Analytics Service",        // Name of the task (for debugging)
            4096,                       // Stack size (bytes)
            this,                       // Pass reference to this class instance
            (tskIDLE_PRIORITY + 1),     // task priority
            NULL,                       // Task handle
            ESP32SVELTEKIT_RUNNING_CORE // Pin to application core
        );
    };

protected:
    NotificationEvents *_notificationEvents;

    static void _loopImpl(void *_this) { static_cast<AnalyticsService *>(_this)->_loop(); }
    void _loop()
    {
        TickType_t xLastWakeTime;
        xLastWakeTime = xTaskGetTickCount();
        while (1)
        {
            StaticJsonDocument<MAX_ESP_ANALYTICS_SIZE> doc;
            String message;
            doc["uptime"] = millis() / 1000;
            doc["free_heap"] = ESP.getFreeHeap();
            doc["min_free_heap"] = ESP.getMinFreeHeap();
            doc["max_alloc_heap"] = ESP.getMaxAllocHeap();
            doc["fs_used"] = ESPFS.usedBytes();
            doc["fs_total"] = ESPFS.totalBytes();
            doc["core_temp"] = temperatureRead();

            serializeJson(doc, message);
            _notificationEvents->send(message, "analytics", millis());

            vTaskDelayUntil(&xLastWakeTime, ANALYTICS_INTERVAL / portTICK_PERIOD_MS);
        }
    };
};
