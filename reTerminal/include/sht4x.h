#ifndef E1001_DASHBOARDm_sht4X_H
#define E1001_DASHBOARDm_sht4X_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSht4x.h>

class Sht4xSensor {
public:
    bool probe();
    bool readSensor(float &temperature, float &humidity);
    bool readSerial(uint32_t &serialNumber);

private:
    SensirionI2cSht4x m_sht;
    bool m_initialized = false;
};

#endif  // E1001_DASHBOARDm_sht4X_H