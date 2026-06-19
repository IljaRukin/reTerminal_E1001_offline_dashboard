#ifndef E1001_DASHBOARD_WIFI_MANAGER_H
#define E1001_DASHBOARD_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

enum class WifiState : uint8_t {
    Off,
    StaActive,
    ApActive,
};

inline const char* wifiStateString(WifiState state) {
    switch (state) {
        case WifiState::Off:         return "Off";
        case WifiState::StaActive:   return "StaActive";
        case WifiState::ApActive:    return "ApActive";
        default:                     return "Unknown";
    }
}

class WifiManager {
public:

    void handleIndex();
    void handleSetup();
    void handleIngest();
    void handleConfig();
    void handleNotFound();

    // state-machine
    void begin();
    void requestOff();
    void requestSta(const char *ssid, const char *pass);
    void requestAp(const char *ssid, const char *pass);
    bool checkConnection();
    void servePage();

    WifiState state() const { return _state; }

private:
    uint32_t _initTime = (time_t)(-1);
    WifiState  _state = WifiState::Off;
    WebServer* _server = nullptr;

};

#endif  // E1001_DASHBOARD_WIFI_MANAGER_H