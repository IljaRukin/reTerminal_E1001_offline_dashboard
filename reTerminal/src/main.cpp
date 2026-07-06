#include <Arduino.h>
#include <algorithm>
#include <stdio.h>
#include <vector>

#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <soc/rtc_periph.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

#include "config.h"
#include "wake_and_sleep.h"
#include "dataSeries.h"
#include "rtc.h"
#include "led.h"
#include "battery_monitor.h"
#include "buzzer.h"
#include "sht4x.h"
#include "sd_manager.h"
#include "wifi_manager.h"
#include "dashboard_ui.h"

RtcPcf8563 g_rtc;
Sht4xSensor g_sht;
BatteryMonitor g_bat;
WifiManager g_wifi;
SdManager g_sd;
DashboardUi g_ui;
Buzzer g_buz;
LED led;

time_t readingEpoch = (time_t)(-1);

void setup()
{
    // LED on
    led.init();
    led.ledOn();

    // init serial
    LOG.printf("[BOOT] %s v%s\n", FW_NAME, FW_VERSION);
    LOG.begin(LOG_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
    while (!LOG) {
        delay(5); // Wait for serial port to connect
    }

    // log firmware & debug
    LOG.printf("\n ---------- [BOOT] %s v%s ---------- \n", FW_NAME, FW_VERSION);
    esp_reset_reason_t reset_reason = esp_reset_reason();
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    wakeupState_t wakeupState = getWakeupState(wakeup_cause);
    LOG.printf("[BOOT] reset reason: %s\n", resetReasonString(reset_reason));
    LOG.printf("[BOOT] wakeup reason: %s\n", wakeupReasonString(wakeup_cause));
    LOG.printf("[BOOT] wakeup state: %s\n", wakeupStateString(wakeupState));

    // init I2C for SHT4x + RTC PCF8563.
    if (Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)) {
        LOG.printf("[I2C] Bus started: SDA=GPIO%d  SCL=GPIO%d\n", PIN_I2C_SDA, PIN_I2C_SCL);
    } else {
        LOG.printf("[I2C] ERROR: Bus start failed: SDA=GPIO%d  SCL=GPIO%d\n", PIN_I2C_SDA, PIN_I2C_SCL);
    }
    if (Wire.setClock(I2C_FREQ_HZ)) {
        LOG.printf("[I2C] Bus clock set to %u Hz\n", I2C_FREQ_HZ);
    } else {
        LOG.printf("[I2C] ERROR: Bus clock set failed: %u Hz\n", I2C_FREQ_HZ);
    }
    
    // RTC time
    bool voltageOK = false;
    // check the PCF8563 is reachable
    LOG.printf("[RTC] Probing PCF8563 at I2C address 0x%02X.\n", PCF8563_ADDR);
    if (g_rtc.rtcProbe()) {
        LOG.printf("[RTC] probing successful.\n");
        // clear STOP bit, disable CLKOUT
        if (g_rtc.rtcInit()) {
            // decide whether the time needs to be set
            voltageOK = g_rtc.rtcVoltageOK();
            bool syncClock = voltageOK;
            if (!voltageOK) {
                LOG.printf("[RTC] Battery OK — retaining stored time.\n");
                LOG.printf("[RTC] WARNING: Voltage Low flag set — backup battery may be depleted.\n");
                LOG.printf("[RTC] Time is unreliable; resetting to compile time.\n");
                #ifdef USE_COMPILE_TIME
                int cy, cm, cd, ch, cmin, cs;
                g_rtc.getCompileTime(cy, cm, cd, ch, cmin, cs);
                LOG.printf("[RTC] Setting time from compile timestamp: "
                        "%04d-%02d-%02d  %02d:%02d:%02d\n",
                        cy, cm, cd, ch, cmin, cs);
                if (g_rtc.rtcSetTime(cy, cm, cd, ch, cmin, cs)) {
                    syncClock = true;
                    LOG.printf("[RTC] set rtc to compile time.\n");
                } else {
                    LOG.printf("[RTC] ERROR: failed to set rtc to compile time.\n");
                }
                #endif
            }
            // read time and sync the ESP32 system clock
            if (syncClock) {
                RtcTime rt;
                if (g_rtc.rtcGetTime(rt)) {
                    g_rtc.syncSystemClock(rt);
                    LOG.printf("[RTC] Current time: %04d-%02d-%02d (%s) %02d:%02d:%02d\n",
                            rt.year, rt.month, rt.day, kWeekdayNames[rt.weekday],
                            rt.hour, rt.minute, rt.second);
                    LOG.printf("[RTC] ESP32 system clock synced.\n");

                    LOG.printf("[TIME] %04d-%02d-%02d (%s) %02d:%02d:%02d%s\n",
                                rt.year, rt.month, rt.day,
                                kWeekdayNames[rt.weekday],
                                rt.hour, rt.minute, rt.second,
                                rt.voltageOK ? "" : "  [VL: battery low!]");
                } else {
                    LOG.printf("[RTC] ERROR: could not read rtc time.\n");
                }
            }
        } else {
            LOG.printf("[RTC] could not initialise PCF8563.\n");
        }
    } else {
        LOG.printf("[RTC] NOT FOUND: check wiring.\n");
    }

    // read nv memory
    loadNvsCredentials();

    // init sensors
    g_bat.begin();
    // get internal sensors data
    sensorInternal_t internalSensorSample;
    readingEpoch = getEpochTime();
    internalSensorSample.timeStamp = readingEpoch;
    
    // read sensors: temperature, humidity
    LOG.printf("[SHT] Probing SHT4X at I2C address 0x%02X.\n", PCF8563_ADDR);
    if (g_sht.probe()) {
        LOG.printf("[SHT] probing successful.\n");
        if (g_sht.readSensor(internalSensorSample.temperature, internalSensorSample.humidity)) {
            LOG.printf("[SHT] %.2f C  %.2f %%\n", internalSensorSample.temperature, internalSensorSample.humidity);
        } else {
            LOG.printf("[SHT] read failed\n");
        }
    } else {
        LOG.printf("[SHT] NOT FOUND: check wiring.\n");
    }

    // read sensors: battery voltage
    if (g_bat.read(internalSensorSample.voltage)) {
        LOG.printf("[BAT] %.2f V\n", internalSensorSample.voltage);
    } else {
        LOG.printf("[BAT] read failed\n");
    }
    g_bat.end();

    // wifi sta mode
    g_wifi.begin();
    if (wakeupState==wakeupState_t::LeftButton || wakeupState==wakeupState_t::MidButton) {
        g_wifi.requestSta(nvsStaSsid.c_str(), nvsStaPassword.c_str());
    }
    LOG.printf("\n[WiFi] wifi in state %s\n", wifiStateString(g_wifi.state()));
    g_wifi.checkConnection();

    // load data from sd
    forecastData_t forecastData;
    bool validForecast = false;
    historicData_t historicData;
    bool validHistory = false;
    {
        String dataString;
        g_sd.begin();
        if (g_sd.isCardInserted()) {
            if (g_sd.mountCard()) {
                //g_sd.listDir("/",0);
                
                //append latest reading
                if (g_sd.appendReading(internalSensorSample)) {
                    LOG.printf("[SD] reading appended\n");
                } else {
                    LOG.printf("[SD] append failed\n");
                }

                //load history
                validHistory = g_sd.readRecentReadings(historicData, readingEpoch - 3 * 24*60*60);
                if (validHistory) {
                    LOG.printf("[SD] history loaded\n");
                } else {
                    LOG.printf("[FC] failed to load history\n");
                }
                
                //load forecast
                g_sd.ensureCsvHeader();
                if (g_sd.readForecast(dataString)) {
                    LOG.printf("[SD] forecast loaded\n");
                    validForecast = parseForecast(dataString, forecastData);
                } else {
                    LOG.printf("[FC] failed to load forecast\n");
                }

            }
        } else {
            LOG.printf("[SD] No card detected at startup. Please insert a card.\n");
        }

        // if (forecastData.requestTime != (time_t)(-1) && forecastData.requestTime > readingEpoch - 2 * 60*60) {
        //     LOG.printf("[FC] forecast recently updated\n");
        // } else {
        //     LOG.printf("[FC] forecast out of date\n");
        if (g_wifi.state() != WifiState::StaActive) {
            LOG.printf("[STA] wifi not connected, can not request forecast\n");
        } else {
            LOG.printf("[STA] requesting new forecast\n");
            validForecast = requestForecast(dataString);
            if (validForecast) {
                forecastData.requestTime = (time_t)(-1);
                dataString.remove(dataString.length() - 1);
                dataString = dataString + ",\"requestTime\":"+String(readingEpoch)+"\n}";
                LOG.printf("[FC] forecast request successful\n");
                if (parseForecast(dataString, forecastData)) {
                    if (g_sd.writeForecast(dataString)) {
                        LOG.printf("[FC] fresh forecast stored\n");
                    } else {
                        LOG.printf("[FC] failed to store forecast\n");
                    }
                } else {
                    LOG.printf("[FC] forecast parse failed\n");
                }
            } else {
                LOG.printf("[FC] forecast request failed\n");
            }
        }
        // }
        dataString.clear();
        g_sd.unmountCard();
        g_sd.end();
    }

    //wifi ap mode
    if (wakeupState==wakeupState_t::MidButton) {
        g_wifi.requestAp(nvsApSsid.c_str(), nvsApPassword.c_str());
    }
    LOG.printf("\n[WiFi] wifi in state %s\n", wifiStateString(g_wifi.state()));

    // update screen
    {
        systemState_t systemState;
        systemState.wifiState    = g_wifi.state() == WifiState::ApActive;
        systemState.rtcState    = voltageOK;

        g_ui.begin();
        g_ui.render(internalSensorSample, systemState, historicData, forecastData);
        g_ui.hibernate();
    }

    // LED off
    led.ledOff();

    // play buzzer melody
    g_buz.begin();
    g_buz.buzzer_melody();
}

void loop()
{
    if (g_wifi.state() == WifiState::ApActive) {
        
        time_t currentEpoch = (time_t)(-1);
        while (true) {
            g_wifi.servePage();
            currentEpoch = getEpochTime();
            if(currentEpoch - readingEpoch > RTC_WAKE_SECONDS) {
                g_sd.begin();
                if (g_sd.isCardInserted()) {
                    if (g_sd.mountCard()) {
                        //g_sd.listDir("/",0);
                        
                        // init sensors
                        g_bat.begin();
                        // get internal sensors data
                        sensorInternal_t internalSensorSample;
                        readingEpoch = getEpochTime();
                        internalSensorSample.timeStamp = readingEpoch;
                        
                        // read sensors: temperature, humidity
                        LOG.printf("[SHT] Probing SHT4X at I2C address 0x%02X.\n", PCF8563_ADDR);
                        if (g_sht.probe()) {
                            LOG.printf("[SHT] probing successful.\n");
                            if (g_sht.readSensor(internalSensorSample.temperature, internalSensorSample.humidity)) {
                                LOG.printf("[SHT] %.2f C  %.2f %%\n", internalSensorSample.temperature, internalSensorSample.humidity);
                            } else {
                                LOG.printf("[SHT] read failed\n");
                            }
                        } else {
                            LOG.printf("[SHT] NOT FOUND: check wiring.\n");
                        }

                        // read sensors: battery voltage
                        if (g_bat.read(internalSensorSample.voltage)) {
                            LOG.printf("[BAT] %.2f V\n", internalSensorSample.voltage);
                        } else {
                            LOG.printf("[BAT] read failed\n");
                        }
                        g_bat.end();

                        //append latest reading
                        if (g_sd.appendReading(internalSensorSample)) {
                            LOG.printf("[SD] reading appended\n");
                        } else {
                            LOG.printf("[SD] append failed\n");
                        }
                    }
                } else {
                    LOG.printf("[SD] No card detected at startup. Please insert a card.\n");
                }
                g_sd.unmountCard();
                g_sd.end();
            }
            delay(50);
            yield();
        }

    } else {
    // disable wifi if enabled
        g_wifi.requestOff();
        
        enterDeepSleep(RTC_WAKE_SECONDS);
    }
}
