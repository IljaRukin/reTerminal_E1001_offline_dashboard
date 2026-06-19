#include "dashboard_ui.h"
#include "config.h"
#include "bitmaps.h"

#include <Arduino.h>
#include <SPI.h>
#include <cmath>
#include <time.h>
#include <map>

void DashboardUi::begin()
{
    // init
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);

    delay(50);

    m_display.begin();

    // 4 level grayscale
    m_display.initGrayMode(GRAY_LEVEL4);
    m_display.fillScreen(TFT_GRAY_3);
    m_display.update();
}

void DashboardUi::render(const sensorInternal_t &sensorSample,
    const systemState_t &systemState,
    const historicData_t &historicData,
    const forecastData_t &forecastData)
{
    m_display.fillScreen(TFT_GRAY_3);
    //m_display.drawFastHLine(0, GUI_HEADER, m_display.width(), TFT_GRAY_0);
    drawHeader(sensorSample, systemState);
    //m_display.drawFastHLine(0, GUI_FORECAST_INFO, m_display.width(), TFT_GRAY_0);
    drawForecast(forecastData);
    m_display.drawFastHLine(0, GUI_FORECAST_PLOT, m_display.width(), TFT_GRAY_0);
    plotForecast(forecastData);
    m_display.drawFastHLine(0, GUI_HISTORY_PLOT, m_display.width(), TFT_GRAY_0);
    plotHistory(historicData);
    m_display.drawFastHLine(0, GUI_BOTTOM, m_display.width(), TFT_GRAY_0);
    m_display.update();
}

void DashboardUi::hibernate()
{
    m_display.sleep();
}

// ----- drawing routines -----

void DashboardUi::drawHeader(const sensorInternal_t &sensorSample, const systemState_t &systemState)
{
    m_display.setTextColor(TFT_GRAY_0, TFT_GRAY_3);
    m_display.setTextDatum(TL_DATUM);
    m_display.setTextSize(2);

    const size_t bufferSize = 78;
    char buffer[bufferSize] = {};

    struct tm tmInfo{};
    localtime_r(&sensorSample.timeStamp, &tmInfo);
    size_t filled = 0;
    //filled += strftime(buffer+filled, bufferSize-filled, "%d %m %Y", &tmInfo);
    filled += strftime(*(&buffer+filled), bufferSize-filled, "   %H:%M %a %d.%m.%Y  ", &tmInfo);
    snprintf(buffer+filled, bufferSize-filled, "  %.2fC  %.2f%%  %.2fV    RTC:%s WIFI:%s",
             sensorSample.temperature,
             sensorSample.humidity,
             sensorSample.voltage,
             systemState.rtcState ? "Y" : "N",
             systemState.wifiState ? "Y" : "N");
    m_display.drawString(buffer, MARGIN, GUI_HEADER+MARGIN);
}

void DashboardUi::drawForecast(const forecastData_t &forecastData) {
    m_display.setTextColor(TFT_GRAY_0, TFT_GRAY_3);
    m_display.setTextDatum(TL_DATUM);

    if (forecastData.startTime == (time_t)(-1)) {
        m_display.setTextSize(6);
        char buffer[17] = {};
        snprintf(buffer, sizeof(buffer), "No Forecast Data\0");
        m_display.drawString(buffer, MARGIN, GUI_FORECAST_INFO+MARGIN);
        return;
    } else {
        m_display.setSwapBytes(true);//TODO:remove
        float xTemp;
        float yTemp;
        float xPos = 0+MARGIN;
        float yPos = GUI_FORECAST_INFO;
        float avg_temperature_2m = 0;
        float avg_cloud_cover = 0;
        float sum_precipitation = 0;
        float max_wind_speed_10m = 0;
        float max_precipitation_probability = 0;
        size_t hour;
        for (size_t timeblock = 0; timeblock < FORECAST_LENGTH/TIME_STRIDE; ++timeblock) {
            xPos = m_display.width() * (float)timeblock/(FORECAST_LENGTH/TIME_STRIDE);
            for (size_t offset = 0; offset < TIME_STRIDE; ++offset) {
                hour = timeblock*TIME_STRIDE + offset;
                avg_temperature_2m = 0;
                avg_cloud_cover = 0;
                sum_precipitation = 0;
                max_wind_speed_10m = 0;
                max_precipitation_probability = 0;
                avg_cloud_cover += (float)forecastData.cloud_cover[hour]/TIME_STRIDE;
                avg_temperature_2m += (float)forecastData.temperature_2m[hour]/TIME_STRIDE;
                sum_precipitation += (float)forecastData.precipitation[hour];
                max_wind_speed_10m = (float)max(forecastData.wind_speed_10m[hour], max_wind_speed_10m);
                max_precipitation_probability = (float)max(forecastData.precipitation_probability[hour], max_precipitation_probability);
            }
            hour = timeblock*TIME_STRIDE + TIME_STRIDE/2;

            //weather symbol
            if (hour%24>=6 && hour%24<=18) {
                if (avg_cloud_cover<25) {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,day_clear);
                } else if (avg_cloud_cover<50) {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,day_partly);
                } else if (avg_cloud_cover<75) {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,day_mostly);
                } else {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,day_cloudy);
                }
            } else {
                if (avg_cloud_cover<25) {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,night_clear);
                } else if (avg_cloud_cover<50) {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,night_partly);
                } else if (avg_cloud_cover<75) {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,night_mostly);
                } else {
                    m_display.pushImage(xPos,yPos,BIG_ICON_SIZE,BIG_ICON_SIZE,night_cloudy);
                }
            }

            //wind
            xTemp = xPos + BIG_ICON_SIZE;
            yTemp = yPos;
            size_t windKMH = 10;
            while (windKMH < max_wind_speed_10m && windKMH <= 10+20*5) {
            m_display.pushImage(xTemp,yTemp,SMALL_ICON_SIZE,SMALL_ICON_SIZE,breeze);
                yTemp += SMALL_ICON_SIZE;
                windKMH += 20;
            }

            //rain
            xTemp = xPos;
            yTemp = yPos + BIG_ICON_SIZE;
            size_t rainMM = 2;
            while (rainMM < sum_precipitation && rainMM <= 2<<5) {
                if (avg_temperature_2m>0) {
                    m_display.pushImage(xTemp,yTemp,SMALL_ICON_SIZE,SMALL_ICON_SIZE,rain);
                } else {
                    m_display.pushImage(xTemp,yTemp,SMALL_ICON_SIZE,SMALL_ICON_SIZE,snow);
                }
                xTemp += SMALL_ICON_SIZE;
                rainMM *= 2;
            }
            
            // special symbol
            xTemp = xPos + BIG_ICON_SIZE;
            yTemp = yPos + BIG_ICON_SIZE;
            // isLightning = false;
            // if (isLightning) {
            //     m_display.pushImage(xTemp,yTemp,SMALL_ICON_SIZE,SMALL_ICON_SIZE,lightning);
            // }
            // isFog = false;
            // if (isFog) {
            //     m_display.pushImage(xTemp,yTemp,SMALL_ICON_SIZE,SMALL_ICON_SIZE,fog);
            // }
            xTemp -= 1;
            m_display.setTextSize(1);
            char buffer[4] = {};
            snprintf(buffer, sizeof(buffer), "%d", (int)max_precipitation_probability);
            m_display.drawString(buffer, xTemp, yTemp);
            
            // time label
            xTemp = xPos;
            yTemp = yPos + BIG_ICON_SIZE+SMALL_ICON_SIZE;
            m_display.setTextSize(1);
            struct tm tmInfo{};
            time_t timeStamp = forecastData.startTime + hour*60*60;
            localtime_r(&timeStamp, &tmInfo);
            char timeBuffer[12] = {};
            strftime(timeBuffer, sizeof(timeBuffer), " %H:%M %a ", &tmInfo);
            m_display.drawString(timeBuffer, xTemp, yTemp);

            //xPos += BIG_ICON_SIZE+SMALL_ICON_SIZE+2*MARGIN;
        }
        return;
    }
}

float tickStep(float minValue, float maxValue) {
    float range = maxValue - minValue;
    float step = 5.0f;
    if (range <= 0.0f) {
        return step;
    }
    while (ceilf(range / step) > 7.0f) {
        step += 5.0f;
    }
    return step;
};

void DashboardUi::plotForecast(const forecastData_t &forecastData) {
    m_display.setTextColor(TFT_GRAY_0, TFT_GRAY_3);
    m_display.setTextDatum(TL_DATUM);

    if (forecastData.startTime == (time_t)(-1)) {
        m_display.setTextSize(8);
        char buffer[17] = {};
        snprintf(buffer, sizeof(buffer), "No Forecast Data\0");
        m_display.drawString(buffer, MARGIN, GUI_FORECAST_PLOT+MARGIN);
    } else {
        float xPos;
        float xPosPrev = -1;
        float xScaling = (float)m_display.width()/(float)FORECAST_LENGTH;
        float yPos;
        float yPosPrev_relative_humidity = -1;
        float yPosPrev_temperature = -1;
        float yPosPrev_pressure_msl = -1;
        float yScaling_relative_humidity = ((float)GUI_HISTORY_PLOT-(float)GUI_FORECAST_PLOT)/100.0;
        float yOffset_relative_humidity = 0;
        float yScaling_temperature = ((float)GUI_HISTORY_PLOT-(float)GUI_FORECAST_PLOT)/(forecastData.max_temperature_2m-forecastData.min_temperature_2m);
        float yOffset_temperature = forecastData.min_temperature_2m;
        float yScaling_pressure_msl = ((float)GUI_HISTORY_PLOT-(float)GUI_FORECAST_PLOT)/(forecastData.max_pressure_msl-forecastData.min_pressure_msl);
        float yOffset_pressure_msl = forecastData.min_pressure_msl;

        float tempStep = tickStep(forecastData.min_temperature_2m, forecastData.max_temperature_2m);
        float tempStart = ceilf(forecastData.min_temperature_2m / tempStep) * tempStep;
        float tempEnd = floorf(forecastData.max_temperature_2m / tempStep) * tempStep;

        float presStep = tickStep(forecastData.min_pressure_msl, forecastData.max_pressure_msl);
        float presStart = ceilf(forecastData.min_pressure_msl / presStep) * presStep;
        float presEnd = floorf(forecastData.max_pressure_msl / presStep) * presStep;

        m_display.setTextSize(1);

        for (float value = tempStart; value <= tempEnd + 0.1f; value += tempStep) {
            int yTick = GUI_HISTORY_PLOT - (int)((value - yOffset_temperature) * yScaling_temperature);
            if (yTick < GUI_FORECAST_PLOT || yTick > GUI_HISTORY_PLOT) continue;
            m_display.drawFastHLine(0, yTick, 2, TFT_GRAY_0);
            char label[8];
            snprintf(label, sizeof(label), "%.0f", value);
            if (yTick - 10 < GUI_FORECAST_PLOT) {
                yTick += 5;
            } else if (yTick + 10 > GUI_HISTORY_PLOT) {
                yTick -= 5;
            }
            m_display.drawString(label, 4 +40, yTick - 4);
        }

        for (float value = presStart; value <= presEnd + 0.1f; value += presStep) {
            int yTick = GUI_HISTORY_PLOT - (int)((value - yOffset_pressure_msl) * yScaling_pressure_msl);
            if (yTick < GUI_FORECAST_PLOT || yTick > GUI_HISTORY_PLOT) continue;
            m_display.drawFastHLine(m_display.width() - 2, yTick, 2, TFT_GRAY_0);
            char label[8];
            snprintf(label, sizeof(label), "%.0f", value);
            if (yTick - 10 < GUI_FORECAST_PLOT) {
                yTick += 5;
            } else if (yTick + 10 > GUI_HISTORY_PLOT) {
                yTick -= 5;
            }
            m_display.drawString(label, m_display.width() - 30 -40, yTick - 4);
        }

        for (size_t hour = 0; hour < FORECAST_LENGTH; ++hour) {
            xPos = (float)hour * xScaling + xScaling/2;
            yPos = GUI_HISTORY_PLOT - (forecastData.relative_humidity_2m[hour] - yOffset_relative_humidity) * yScaling_relative_humidity;
            m_display.drawCircle((int)xPos, (int)yPos, 2, TFT_GRAY_2);
            if (xPosPrev != -1 || yPosPrev_relative_humidity != -1) {
                m_display.drawLine((int)xPosPrev, (int)yPosPrev_relative_humidity, (int)xPos, (int)yPos, TFT_GRAY_1);
            }
            yPosPrev_relative_humidity = yPos;
            yPos = GUI_HISTORY_PLOT - (forecastData.temperature_2m[hour] - yOffset_temperature) * yScaling_temperature;
            m_display.drawCircle((int)xPos, (int)yPos, 2, TFT_GRAY_0);
            if (xPosPrev != -1 || yPosPrev_temperature != -1) {
                m_display.drawLine((int)xPosPrev, (int)yPosPrev_temperature, (int)xPos, (int)yPos, TFT_GRAY_0);
            }
            yPosPrev_temperature = yPos;
            yPos = GUI_HISTORY_PLOT - (forecastData.pressure_msl[hour] - yOffset_pressure_msl) * yScaling_pressure_msl;
            m_display.drawCircle((int)xPos, (int)yPos, 2, TFT_GRAY_1);
            if (xPosPrev != -1 || yPosPrev_pressure_msl != -1) {
                m_display.drawLine((int)xPosPrev, (int)yPosPrev_pressure_msl, (int)xPos, (int)yPos, TFT_GRAY_2);
            }
            yPosPrev_pressure_msl = yPos;
            xPosPrev = xPos;
        }

        // time label
        int yTemp = GUI_HISTORY_PLOT-8;
        int xTemp = m_display.width()-100;
        m_display.setTextSize(1);
        struct tm tmInfo{};
        time_t timeStamp = forecastData.requestTime;
        localtime_r(&timeStamp, &tmInfo);
        char timeBuffer[20] = {};
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M %d.%m.%Y", &tmInfo);
        m_display.drawString(timeBuffer, xTemp, yTemp);
    }
}

void DashboardUi::plotHistory(const historicData_t &historicData) {
    m_display.setTextColor(TFT_GRAY_0, TFT_GRAY_3);
    m_display.setTextDatum(TL_DATUM);
    m_display.setTextSize(8);
    
    if (historicData.min_Time >= historicData.max_Time) {
        m_display.setTextSize(8);
        char buffer[17] = {};
        snprintf(buffer, sizeof(buffer), "No Historic Data\0");
        m_display.drawString(buffer, MARGIN, GUI_HISTORY_PLOT+MARGIN);
    } else {
        float xPos;
        float xPosPrev = -1;
        float xScaling = (float)m_display.width()/(historicData.max_Time-historicData.min_Time);
        float yPos;
        float yPosPrev_relative_humidity = -1;
        float yScaling_relative_humidity = ((float)GUI_BOTTOM-(float)GUI_HISTORY_PLOT)/100.0;
        float yOffset_relative_humidity = 0;
        for (auto entry : historicData.relative_humidity) {
            xPos = ((float)entry.first - historicData.min_Time) * xScaling;
            yPos = GUI_BOTTOM - (entry.second - yOffset_relative_humidity) * yScaling_relative_humidity;
            m_display.drawCircle((int)xPos, (int)yPos, 2, TFT_GRAY_1);
            if (xPosPrev != -1 || yPosPrev_relative_humidity != -1) {
                m_display.drawLine((int)xPosPrev, (int)yPosPrev_relative_humidity, (int)xPos, (int)yPos, TFT_GRAY_1);
            }
            yPosPrev_relative_humidity = yPos;
            xPosPrev = xPos;
        }
        xPosPrev = -1;
        float yPosPrev_temperature = -1;
        float yScaling_temperature = ((float)GUI_BOTTOM-(float)GUI_HISTORY_PLOT)/(historicData.max_temperature-historicData.min_temperature);
        float yOffset_temperature = historicData.min_temperature;
        for (auto entry : historicData.temperature) {
            xPos = ((float)entry.first - historicData.min_Time) * xScaling;
            yPos = GUI_BOTTOM - (entry.second - yOffset_temperature) * yScaling_temperature;
            m_display.drawCircle((int)xPos, (int)yPos, 2, TFT_GRAY_0);
            if (xPosPrev != -1 || yPosPrev_temperature != -1) {
                m_display.drawLine((int)xPosPrev, (int)yPosPrev_temperature, (int)xPos, (int)yPos, TFT_GRAY_0);
            }
            yPosPrev_temperature = yPos;
            xPosPrev = xPos;
        }
        
        float tempStep = tickStep(historicData.min_temperature, historicData.max_temperature);
        float tempStart = ceilf(historicData.min_temperature / tempStep) * tempStep;
        float tempEnd = floorf(historicData.max_temperature / tempStep) * tempStep;

        // float presStep = tickStep(historicData.min_pressure, historicData.max_pressure);
        // float presStart = ceilf(historicData.min_pressure / presStep) * presStep;
        // float presEnd = floorf(historicData.max_pressure / presStep) * presStep;

        float presStep = tickStep(0, 100);
        float presStart = ceilf(0 / presStep) * presStep;
        float presEnd = floorf(100 / presStep) * presStep;

        m_display.setTextSize(1);
        for (float value = tempStart; value <= tempEnd + 0.1f; value += tempStep) {
            int yTick = GUI_BOTTOM - (int)((value - yOffset_temperature) * yScaling_temperature);
            if (yTick < GUI_HISTORY_PLOT || yTick > GUI_BOTTOM) continue;
            m_display.drawFastHLine(0, yTick, 2, TFT_GRAY_0);
            char label[8];
            snprintf(label, sizeof(label), "%.0f", value);
            if (yTick - 10 < GUI_HISTORY_PLOT) {
                yTick += 5;
            } else if (yTick + 10 > GUI_BOTTOM) {
                yTick -= 5;
            }
            m_display.drawString(label, 4 +40, yTick - 4);
        }

        // for (float value = presStart; value <= presEnd + 0.1f; value += presStep) {
        //     int yTick = GUI_BOTTOM - (int)((value - yOffset_pressure) * yScaling_pressure);
        //     if (yTick < GUI_HISTORY_PLOT || yTick > GUI_BOTTOM) continue;
        //     m_display.drawFastHLine(m_display.width() - 2, yTick, 2, TFT_GRAY_0);
        //     char label[8];
        //     snprintf(label, sizeof(label), "%.0f", value);
        //     if (yTick - 10 < GUI_HISTORY_PLOT) {
        //         yTick += 5;
        //     } else if (yTick + 10 > GUI_BOTTOM) {
        //         yTick -= 5;
        //     }
        //     m_display.drawString(label, m_display.width() - 16 -40, yTick - 4);
        // }

        for (float value = presStart; value <= presEnd + 0.1f; value += presStep) {
            int yTick = GUI_BOTTOM - (int)((value - yOffset_relative_humidity) * yScaling_relative_humidity);
            if (yTick < GUI_HISTORY_PLOT || yTick > GUI_BOTTOM) continue;
            m_display.drawFastHLine(m_display.width() - 2, yTick, 2, TFT_GRAY_0);
            char label[8];
            snprintf(label, sizeof(label), "%.0f", value);
            if (yTick - 10 < GUI_HISTORY_PLOT) {
                yTick += 5;
            } else if (yTick + 10 > GUI_BOTTOM) {
                yTick -= 5;
            }
            m_display.drawString(label, m_display.width() - 16 -40, yTick - 4);
        }

        std::map<int, time_t> timeMap{
            {0, historicData.min_Time},
            {m_display.width() - 100, historicData.max_Time}
        };
        for (auto sample : timeMap) {
            // time label
            int yTemp = GUI_BOTTOM-8;
            int xTemp = sample.first;
            m_display.setTextSize(1);
            struct tm tmInfo{};
            time_t timeStamp = sample.second;
            localtime_r(&timeStamp, &tmInfo);
            char timeBuffer[20] = {};
            strftime(timeBuffer, sizeof(timeBuffer), "%H:%M %d.%m.%Y", &tmInfo);
            m_display.drawString(timeBuffer, xTemp, yTemp);
        }
    }
}

/*
# drawing
TFT_GRAY_0 = 0 = black
TFT_GRAY_1 = 1 = dark gray
TFT_GRAY_2 = 2 = light gray
TFT_GRAY_3 = 3 = white
m_display.drawRect(x, y, w, h, TFT_GRAY_0);
m_display.fillRect(x, y, w, h, TFT_GRAY_0);
m_display.drawFastHLine(x, y, w, TFT_GRAY_0);
m_display.drawFastVLine(x, y, w, TFT_GRAY_0);
m_display.drawLine(x0, y0, x1, y1, TFT_GRAY_0);
m_display.drawCircle(x, y, w, TFT_GRAY_0);
m_display.drawPixel(x, y, TFT_GRAY_0);
m_display.drawString("str", x, y, TFT_GRAY_0);
*/