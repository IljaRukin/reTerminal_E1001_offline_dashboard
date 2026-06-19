#ifndef E1001_DASHBOARD_UI_H
#define E1001_DASHBOARD_UI_H

#include <Arduino.h>
#include "driver.h"
#include <TFT_eSPI.h>
#include "dataSeries.h"
#include "rtc.h"

class DashboardUi {
public:
    void begin();
    void render(const sensorInternal_t &sensorSample,
                const systemState_t &systemState,
                const historicData_t &historicData,
                const forecastData_t &forecastData);
    void hibernate();

private:
    void drawHeader(const sensorInternal_t &sensorSample, const systemState_t &systemState);
    void drawForecast(const forecastData_t &forecastData);
    void plotForecast(const forecastData_t &forecastData);
    void plotHistory(const historicData_t &historicData);

    EPaper m_display;
};

#endif  // E1001_DASHBOARD_UI_H