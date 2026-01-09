#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>

// EEPROM
#define EEPROM_SIZE 160
#define SSID_ADDR 0
#define PASS_ADDR 64

// Funcions públiques
bool setup_wifi();
void ensureWiFi();
void loadWiFiCredentials(char* ssid, char* pass);

#endif
