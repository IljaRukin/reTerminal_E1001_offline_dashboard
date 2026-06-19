#ifndef E1001_DASHBOARD_BATTERY_H
#define E1001_DASHBOARD_BATTERY_H

#include <Arduino.h>

class BatteryMonitor {
public:
    void begin();
    void end();
    bool read(float &volts);
private:
    bool m_initialized = false;
};

#endif  // E1001_DASHBOARD_BATTERY_H