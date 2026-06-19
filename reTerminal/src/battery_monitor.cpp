#include "battery_monitor.h"
#include "config.h"

void BatteryMonitor::begin()
{
    pinMode(BAT_ENABLE_PIN, OUTPUT);
    digitalWrite(BAT_ENABLE_PIN, HIGH);
    analogReadResolution(12);
    analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
    m_initialized = true;
}

void BatteryMonitor::end()
{
    digitalWrite(BAT_ENABLE_PIN, LOW);
    m_initialized = false;
}

bool BatteryMonitor::read(float &volts)
{
    if (!m_initialized) {
        return false;
    } else {
        int mv = analogReadMilliVolts(BAT_ADC_PIN);
        // 2x divider on the carrier board.
        volts = (static_cast<float>(mv) / 1000.0f) * 2.0f;
        return true;
    }
    return false;
}

