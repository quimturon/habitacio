#pragma once
#include <Arduino.h>

// EEPROM
String readVersion();
void saveVersion(const String& version);

// OTA
bool checkForUpdate(String &newVersion);
void performOTA(const String &newVersion);
