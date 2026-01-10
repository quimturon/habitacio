#ifndef NTP_H
#define NTP_H

#include <Arduino.h>
#include <RTClib.h>

// Inicialitza NTP (opcional, però recomanat)
void ntpInit(
    const char* ntpServer = "pool.ntp.org",
    long gmtOffset_sec = 3600,
    int daylightOffset_sec = 3600
);

// Sincronitza el RTC via NTP
bool ntpSyncRTC(RTC_DS3231 &rtc);

// Retorna l'última hora obtinguda per NTP (debug)
bool ntpGetTime(struct tm &timeinfo, uint32_t timeoutMs = 10000);

#endif
