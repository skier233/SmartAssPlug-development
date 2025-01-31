#pragma once
/**
 *   SmartAss Plug
 *
 *   The smartest butt plug you can imagine.
 *   https://github.com/theelims/SmartAssPlug
 *
 *   Copyright (C) 2023 theelims
 *
 *   All Rights Reserved. This software may be modified and distributed under
 *   the terms of the Attribution-ShareAlike 4.0 International license.
 *   See the LICENSE file for details.
 *
 **/

#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <SimpleKalmanFilter.h>
#include <RingBuf.h>

#define FILTER_SETTINGS_FILE "/config/filterSettings.json"
#define FILTER_SETTINGS_PATH "/rest/filterSettings"

class FilterData
{
public:
    float measurementError;
    float estimateError;
    float processNoise;
    bool interpolateGlitches;

    static void read(FilterData &settings, JsonObject &root)
    {
        root["measurement_error"] = settings.measurementError;
        root["estimate_error"] = settings.estimateError;
        root["process_noise"] = settings.processNoise;
        root["interpolate_glitches"] = settings.interpolateGlitches;
    }

    static StateUpdateResult update(JsonObject &root, FilterData &settings)
    {
        settings.measurementError = root["measurement_error"] | 1.0;
        settings.estimateError = root["estimate_error"] | 1.0;
        settings.processNoise = root["process_noise"] | 1.0;
        settings.interpolateGlitches = root["interpolate_glitches"] | true;
        return StateUpdateResult::CHANGED;
    }
};

class EdgingFilterService : public StatefulService<FilterData>
{
public:
    EdgingFilterService(AsyncWebServer *server, FS *fs, SecurityManager *securityManager);

    void begin();

    void onConfigUpdated();

    float updateEstimate(float input);

private:
    HttpEndpoint<FilterData> _httpEndpoint;
    FSPersistence<FilterData> _fsPersistence;
    SimpleKalmanFilter pressureKalmanFilter = SimpleKalmanFilter(_state.measurementError, _state.estimateError, _state.processNoise);
    RingBuf<float, 3> glitchBuffer;
};
