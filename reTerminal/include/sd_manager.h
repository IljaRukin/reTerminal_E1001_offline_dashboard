#ifndef E1001_DASHBOARD_SDCARD_H
#define E1001_DASHBOARD_SDCARD_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "config.h"
#include "dataSeries.h"

class SdManager {
public:
    bool isCardInserted();
    bool begin();
    bool end();

    bool mountCard();
    bool unmountCard();
    bool isMounted() const { return m_mounted; }

    bool ensureCsvHeader();
    bool appendReading(sensorInternal_t sensorInternal);
    bool readRecentReadings(historicData_t &historicData, time_t timeStart);

    bool readForecast(String &json);
    bool writeForecast(const String &json);

private:
    bool m_initialized = false;
    bool m_mounted = false;
    SPIClass m_spi = SPIClass(HSPI);
    void printIndent(uint8_t level);
    void listDir(File dir, uint8_t level);
};

#endif  // E1001_DASHBOARD_SDCARD_H