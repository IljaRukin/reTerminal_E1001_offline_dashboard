#ifndef E1001_DASHBOARD_RTC_H
#define E1001_DASHBOARD_RTC_H

#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>

#define USE_COMPILE_TIME

struct RtcTime {
    int  year;       // full year (e.g. 2026)
    int  month;      // 1–12
    int  day;        // 1–31
    int  weekday;    // 0=Sunday … 6=Saturday
    int  hour;       // 0–23
    int  minute;     // 0–59
    int  second;     // 0–59
    bool voltageOK;  // false → VL flag set, battery was drained, time unreliable
};

// ---------- PCF8563 register map (only the registers used here) --------------
#define REG_CTRL1       0x00   // Control/Status 1 — bit5 STOP halts the clock
#define REG_CTRL2       0x01   // Control/Status 2
#define REG_SECONDS     0x02   // bit7 = VL (voltage-low flag); bits6:0 = seconds
#define REG_MINUTES     0x03   // bits6:0 = minutes
#define REG_HOURS       0x04   // bits5:0 = hours
#define REG_DAYS        0x05   // bits5:0 = day-of-month
#define REG_WEEKDAYS    0x06   // bits2:0 = weekday (0=Sunday)
#define REG_MONTHS      0x07   // bit7 = century (0→2000s, 1→1900s); bits4:0 = month
#define REG_YEARS       0x08   // bits7:0 = year within century (BCD, 00–99)
#define REG_CLKOUT      0x0D   // CLKOUT control — bit7 FE enables clock output pin

static const char *kWeekdayNames[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static time_t getEpochTime()
{
    return time(nullptr);
}

static inline uint8_t bcdToDec(uint8_t bcd)
{
    return static_cast<uint8_t>(((bcd >> 4) * 10U) + (bcd & 0x0FU));
}

static inline uint8_t decToBcd(uint8_t dec)
{
    return static_cast<uint8_t>(((dec / 10U) << 4) | (dec % 10U));
}

static void dateIso(char *out, size_t outSize, const RtcTime &rtc)
{
    snprintf(out, outSize, "%04d-%02d-%02dT%02d:%02d:%02d",
             rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);
}

static String formatTimestamp(time_t t) {
    struct tm* timeinfo = localtime(&t);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%d-%m-%Y_%H:%M:%S", timeinfo);
    return String(buffer);
}

static void dateString(char *out, size_t outSize, const RtcTime &rtc)
{
    snprintf(out, outSize, "%04d-%02d-%02d %02d:%02d:%02d",
             rtc.day, rtc.month, rtc.year, rtc.hour, rtc.minute, rtc.second);
}

static time_t parseIso8601ToEpoch(const String& iso)
{
    if (iso.length() < 16U) {
        return 0;
    }
    if (iso[4] != '-' || iso[7] != '-' || iso[10] != 'T' || iso[13] != ':') {
        return false;
    }

    const int year = iso.substring(0, 4).toInt();
    const int month = iso.substring(5, 7).toInt();
    const int day = iso.substring(8, 10).toInt();
    const int hour = iso.substring(11, 13).toInt();
    const int minute = iso.substring(14, 16).toInt();
    int second = 0;
    size_t pos = 16U;

    if (iso.length() >= 19U && iso[16] == ':' && iso[17] >= '0' && iso[17] <= '9') {
        second = iso.substring(17, 19).toInt();
        pos = 19U;
    }

    if (iso.length() > pos) {
        if (iso[pos] == 'Z' || iso[pos] == 'z') {
            ++pos;
        }
        if (iso.length() != pos) {
            return 0;
        }
    }

    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 60) {
        return 0;
    }

    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    return mktime(&t);
}

class RtcPcf8563 {
public:
    static bool rtcProbe();
    static bool rtcInit();
    static bool rtcSetTime(int year, int month, int day, int hour, int minute, int second);
    static bool rtcGetTime(RtcTime &rt);
    static void syncSystemClock(const RtcTime &rt);
    static bool rtcVoltageOK();
#ifdef USE_COMPILE_TIME
    static void getCompileTime(int &year, int &month, int &day,
        int &hour, int &minute, int &second);
#endif  // USE_COMPILE_TIME

private:
    static bool rtcReadRegs(uint8_t reg, uint8_t *buf, size_t len);
    static bool rtcWriteReg(uint8_t reg, uint8_t value);
};

#endif  // E1001_DASHBOARD_RTC_H