#ifndef E1001_DASHBOARD_CONFIG_H
#define E1001_DASHBOARD_CONFIG_H

// ============================================================
// reTerminal E1001 — Offline Dashboard configuration
// ============================================================

// --- Firmware ---
#define FW_VERSION           "1.0.0"
#define FW_NAME              "reTerminal E1001 Dashboard"

// --- Wake / sleep behaviour ---
// Wake every 1 minute via RTC alarm.
#define RTC_WAKE_SECONDS     (30 * 60)

// --- Log serial ---
#define PIN_SERIAL_RX        44
#define PIN_SERIAL_TX        43
#define LOG_BAUD             115200
#define LOG                  Serial1

// --- I2C bus (shared between PCF8563 RTC and SHT4x sensor) ---
#define PIN_I2C_SDA          19
#define PIN_I2C_SCL          20
#define I2C_FREQ_HZ          400000UL

// PCF8563 RTC I2C address
#define PCF8563_ADDR         0x51

// SHT4x I2C address (ADDR pin low = 0x44, high = 0x45)
#define SHT4X_ADDR           0x44

// --- Onboard LED (E1001: GPIO6, inverted logic: LOW = on) ---
#define LED_PIN              6

// --- User buttons ---
#define BTN_LEFT_PIN         5   // KEY0 — wake + toggle STA for 2 min
#define BTN_MIDDLE_PIN       4   // KEY1 — wake + toggle AP
#define BTN_RIGHT_PIN        3   // KEY2 — wake (same as timer)

// --- Battery monitor (E1001: ADC GPIO1 + enable GPIO21) ---
#define BAT_ADC_PIN          1
#define BAT_ENABLE_PIN       21

// --- SD card (E1001/E1002/E1004: SD_EN_PIN = 16) ---
#define SD_EN_PIN            16
#define SD_DET_PIN           15
#define SD_CS_PIN            14
#define SD_MOSI_PIN          9
#define SD_MISO_PIN          8
#define SD_SCK_PIN           7

// --- Buzzer ---
#define BUZZER_PIN 45

// --- ePaper display (Seeed_GFX EPaper) ---
// Pins are configured inside Seeed_GFX via BOARD_SCREEN_COMBO 520.
// No explicit pin map needed here.

//--------------------------------------------------

// --- SD card paths ---
#define INTERNAL_CSV_PATH    "/internalHistory.csv"
#define EXTERNAL_CSV_PATH    "/externalHistory.csv"
#define FORECAST_PATH        "/forecast.json"

// CSV header (created on first run if missing)
#define CSV_HEADER_INTERNAL  "#time(time_t),temperature(float),humidity(float)\n"
#define CSV_HEADER_EXTERNAL  "#time(time_t),temperature(float),humidity(float),pressure(float),brightness(float)\n"

struct sensorInternal_t {
    time_t  timeStamp;
    float   temperature;
    float   humidity;
    float   voltage;
};

struct sensorExternal_t {
    time_t  timeStamp;
    float   temperature;
    float   humidity;
    float   pressure;
    float   brightness;
    float   voltage;
};

struct systemState_t {
    bool    wifiState;
    bool    rtcState;
};

// Forecast
#define DEFAULT_LAT          53.0758   // Bremen
#define DEFAULT_LON          8.8072

// --- WIFI STA ---
#define STA_SSID              "MyHotspot"
#define STA_PASSWORD          "dwx8i9n8k6ikqb5"

// STA mode
#define STA_SETUP_TIMEOUT_MS  (30UL * 1000UL)  // 10s

// --- WIFI AP ---
#define AP_SSID              "reTerminal"
#define AP_PASSWORD          "admin"
#define AP_MASTER_KEY        "admin"
#define AP_CHANNEL           6

// --- Display GUI ---
#define MARGIN                (1)
#define GUI_HEADER            (0)
#define GUI_BOTTOM            (480)
#define GUI_FORECAST_INFO     (GUI_HEADER + 16)
#define GUI_FORECAST_PLOT     (GUI_FORECAST_INFO + 64 + 8)
#define GUI_HISTORY_PLOT      (GUI_FORECAST_PLOT + (GUI_BOTTOM-GUI_FORECAST_PLOT)/2)

//--------------------------------------------------

#include <Preferences.h>

static String nvsStaSsid;
static String nvsStaPassword;
static String nvsApMasterKey;
static String nvsApSsid;
static String nvsApPassword;
static float nvsForecastLatitude;
static float nvsForecastLongitude;

static void loadNvsCredentials()
{
    Preferences prefs;
    if (!prefs.begin("config", true)) {
        nvsStaSsid = STA_SSID;
        nvsStaPassword = STA_PASSWORD;
        nvsApMasterKey = AP_MASTER_KEY;
        nvsApSsid = AP_SSID;
        nvsApPassword = AP_PASSWORD;
        nvsForecastLongitude = DEFAULT_LON;
        nvsForecastLatitude = DEFAULT_LAT;
        return;
    }
    nvsStaSsid = prefs.getString("StaSsid", STA_SSID);
    nvsStaPassword = prefs.getString("StaPassword", STA_PASSWORD);
    nvsApMasterKey = prefs.getString("ApMasterKey", AP_MASTER_KEY);
    nvsApSsid = prefs.getString("ApSsid", AP_SSID);
    nvsApPassword = prefs.getString("ApPassword", AP_PASSWORD);
    nvsForecastLatitude = prefs.getFloat("ForecastLatitude", DEFAULT_LAT);
    nvsForecastLongitude = prefs.getFloat("ForecastLongitude", DEFAULT_LON);
    prefs.end();
}

static void saveNvsCredentials(std::string p_nvsStaSsid,
                               std::string p_nvsStaPassword,
                            //    std::string p_nvsApMasterKey,
                            //    std::string p_nvsApSsid,
                            //    std::string p_nvsApPassword,
                               float p_nvsForecastLatitude,
                               float p_nvsForecastLongitude)
{
    Preferences prefs;
    if (prefs.begin("config", true)) {
        if (p_nvsStaSsid.size()>0) {
            prefs.putString("StaSsid", p_nvsStaSsid.c_str());
        }
        if (p_nvsStaPassword.size()>0) {
            prefs.putString("StaPassword", p_nvsStaPassword.c_str());
        }
        // if (p_nvsApMasterKey.size()>0) {
        //     prefs.putString("ApMasterKey", p_nvsApMasterKey.c_str());
        // }
        // if (p_nvsApSsid.size()>0) {
        //     prefs.putString("ApSsid", p_nvsApSsid.c_str());
        // }
        // if (p_nvsApPassword.size()>0) {
        //     prefs.putString("ApPassword", p_nvsApPassword.c_str());
        // }
        if (p_nvsForecastLatitude<0) {
            prefs.putFloat("ForecastLatitude", p_nvsForecastLatitude);
        }
        if (p_nvsForecastLongitude<0) {
            prefs.putFloat("ForecastLongitude", p_nvsForecastLongitude);
        }
    }
    prefs.end();
    //reload
    loadNvsCredentials();
}

#endif  // E1001_DASHBOARD_CONFIG_H