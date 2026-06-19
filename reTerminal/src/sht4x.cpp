#include "sht4x.h"
#include "config.h"

bool Sht4xSensor::probe()
{
    m_sht.begin(Wire, SHT4X_ADDR);

    Wire.beginTransmission(SHT4X_ADDR);
    const uint8_t error = Wire.endTransmission();
    if (error != 0) {
        LOG.printf("[SHT4x] probe failed: %u\n", static_cast<unsigned>(error));
        return false;
    }

    return true;
}

bool Sht4xSensor::readSerial(uint32_t &serialNumber)
{
    uint16_t error = 0;
    char     msg[256];

    error = m_sht.serialNumber(serialNumber);
    if (error) {
        errorToString(error, msg, sizeof(msg));
        LOG.println(msg);
        return false;
    }
    return true;
}

bool Sht4xSensor::readSensor(float &temperature, float &humidity)
{
    uint16_t error = 0;
    char     msg[256];

    error = m_sht.measureHighPrecision(temperature, humidity);
    if (error) {
        errorToString(error, msg, sizeof(msg));
        LOG.println(msg);
        return false;
    }
    return true;
}
