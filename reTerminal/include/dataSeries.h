#ifndef E1001_DASHBOARD_FORECAST_H
#define E1001_DASHBOARD_FORECAST_H

#include <Arduino.h>
#include "config.h"
#include "rtc.h"

#include <map>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define FORECAST_LENGTH (24*3)
#define TIME_STRIDE     (6)

struct forecastData_t {
  time_t requestTime = (time_t)(-1);
  time_t startTime = (time_t)(-1);//"time":"iso8601" -> epoch
  float precipitation_probability[FORECAST_LENGTH] = {0};//"precipitation_probability":"%" -> text
  float cloud_cover[FORECAST_LENGTH] = {0};//"cloud_cover":"%" -> images
  float precipitation[FORECAST_LENGTH] = {0};//"precipitation":"mm" -> icon
  float wind_speed_10m[FORECAST_LENGTH] = {0};//"wind_speed_10m":"km/h" -> icon
  float min_wind_speed_10m = 0;//99999;
  float max_wind_speed_10m = 20;//-99999;
  float temperature_2m[FORECAST_LENGTH] = {0};//"temperature_2m":"°C" -> plot
  float min_temperature_2m = 15;//99999;
  float max_temperature_2m = 25;//-99999;
  float pressure_msl[FORECAST_LENGTH] = {0};//"pressure_msl":"hPa" -> plot
  float min_pressure_msl = 980;//-99999;
  float max_pressure_msl = 1020;//99999;
  float relative_humidity_2m[FORECAST_LENGTH] = {0};//"relative_humidity_2m":"%" -> plot
};

struct historicData_t {
    time_t min_Time = (time_t)(-1);
    time_t max_Time = (time_t)(-1);
    float min_temperature = 5;//99999;
    float max_temperature = 20;//-99999;
    std::map<time_t,float> temperature;//"temperature":"°C" -> plot
    std::map<time_t,float> relative_humidity;//"relative_humidity":"%" -> plot
};

inline bool requestForecast(String& json)
{
    String url;
    url.reserve(384);
    url  = "https://api.open-meteo.com/v1/forecast";
    url += "?latitude=";  url += String(nvsForecastLatitude, 4);
    url += "&longitude="; url += String(nvsForecastLongitude, 4);
    url += "&hourly=cloud_cover,precipitation,precipitation_probability,temperature_2m";
    url += ",pressure_msl,relative_humidity_2m,wind_speed_10m";
    url += "&forecast_days=3";
    url += "&timezone=auto";

    LOG.printf("[FC] GET %s\n", url.c_str());

    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    int code = http.GET();
    if (code != 200) {
        LOG.printf("[FC] HTTP %d\n", code);
        http.end();
        return false;
    }

    json = http.getString();
    http.end();

    return true;
}

inline bool parseForecast(const String& json, forecastData_t& data)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return false;
    }

    if (!doc["hourly"].is<JsonObject>()) {
        return false;
    }

    JsonObject hourly = doc["hourly"].as<JsonObject>();

    if (!hourly["time"].is<JsonArray>() ||
        !hourly["cloud_cover"].is<JsonArray>() ||
        !hourly["precipitation"].is<JsonArray>() ||
        !hourly["precipitation_probability"].is<JsonArray>() ||
        !hourly["temperature_2m"].is<JsonArray>() ||
        !hourly["relative_humidity_2m"].is<JsonArray>() ||
        !hourly["wind_speed_10m"].is<JsonArray>() ||
        !hourly["pressure_msl"].is<JsonArray>()) {
        return false;
    }

    JsonArrayConst srcTime = hourly["time"].as<JsonArrayConst>();
    if (srcTime.size() == 0) {
        return false;
    }
    const char* firstTimeStr = srcTime[0].as<const char*>();
    if (!firstTimeStr) {
        return false;
    }
    data.startTime = parseIso8601ToEpoch(String(firstTimeStr));

    JsonArrayConst srcCloud = hourly["cloud_cover"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcCloud.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcCloud[i];
        data.cloud_cover[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
    }

    JsonArrayConst srcPrecip = hourly["precipitation"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcPrecip.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcPrecip[i];
        data.precipitation[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
    }

    JsonArrayConst srcProb = hourly["precipitation_probability"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcProb.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcProb[i];
        data.precipitation_probability[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
    }

    JsonArrayConst srcTemp = hourly["temperature_2m"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcTemp.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcTemp[i];
        data.temperature_2m[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
        data.min_temperature_2m = min(data.min_temperature_2m, data.temperature_2m[i]);
        data.max_temperature_2m = max(data.max_temperature_2m, data.temperature_2m[i]);
    }

    JsonArrayConst srcHumidity = hourly["relative_humidity_2m"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcHumidity.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcHumidity[i];
        data.relative_humidity_2m[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
    }

    JsonArrayConst srcWind = hourly["wind_speed_10m"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcWind.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcWind[i];
        data.wind_speed_10m[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
        data.min_wind_speed_10m = min(data.min_wind_speed_10m, data.wind_speed_10m[i]);
        data.max_wind_speed_10m = max(data.max_wind_speed_10m, data.wind_speed_10m[i]);
    }

    JsonArrayConst srcPressure = hourly["pressure_msl"].as<JsonArrayConst>();
    for (size_t i = 0; i < srcPressure.size() && i < FORECAST_LENGTH; ++i) {
        JsonVariantConst value = srcPressure[i];
        data.pressure_msl[i] = value.is<float>() || value.is<int>() || value.is<double>() ? value.as<float>() : 0.0f;
        data.min_pressure_msl = min(data.min_pressure_msl, data.pressure_msl[i]);
        data.max_pressure_msl = max(data.max_pressure_msl, data.pressure_msl[i]);
    }
    
    data.requestTime = doc["requestTime"].as<time_t>();

    return true;
}

#endif  // E1001_DASHBOARD_FORECAST_H