#include "ntp.h"
#include <time.h>

static const char* _ntpServer;
static long _gmtOffset;
static int _daylightOffset;
static bool _ntpConfigured = false;

void ntpInit(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec) {
    _ntpServer = ntpServer;
    _gmtOffset = gmtOffset_sec;
    _daylightOffset = daylightOffset_sec;

    configTime(_gmtOffset, _daylightOffset, _ntpServer);
    _ntpConfigured = true;
}

bool ntpGetTime(struct tm &timeinfo, uint32_t timeoutMs) {
    if (!_ntpConfigured) return false;
    return getLocalTime(&timeinfo, timeoutMs);
}

bool ntpSyncRTC(RTC_DS3231 &rtc) {
    if (!_ntpConfigured) return false;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10000)) {
        Serial.println("❌ NTP: no s'ha pogut obtenir l'hora");
        return false;
    }

    rtc.adjust(DateTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    ));

    Serial.println("✅ NTP: RTC actualitzat");
    return true;
}
