#include "ota.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>
#include <Adafruit_SSD1306.h>

// ================= CONFIG =================
#define EEPROM_SIZE 160
#define VERSION_ADDR 128

static const char* releasesAPI =
    "https://api.github.com/repos/quimturon/habitacio/releases/latest";

// === Globals definits al main.cpp ===
extern Adafruit_SSD1306 display;
extern String FW_VERSION;

// ================= EEPROM =================
void saveVersion(const String& version) {
    EEPROM.begin(EEPROM_SIZE);
    int i = 0;
    for (; version[i] != '\0' && i < 31; i++)
        EEPROM.write(VERSION_ADDR + i, version[i]);
    EEPROM.write(VERSION_ADDR + i, '\0');
    EEPROM.commit();
}

String readVersion() {
    EEPROM.begin(EEPROM_SIZE);
    char buffer[32];
    int i = 0;
    while (EEPROM.read(VERSION_ADDR + i) != '\0' && i < 31) {
        buffer[i] = EEPROM.read(VERSION_ADDR + i);
        i++;
    }
    buffer[i] = '\0';
    return String(buffer);
}

// ================= VERSION COMPARE =================
static bool isVersionNewer(const String& current, const String& latest) {
    int majorC=0, minorC=0, patchC=0;
    int majorL=0, minorL=0, patchL=0;

    sscanf(current.c_str(), "%d.%d.%d", &majorC, &minorC, &patchC);
    sscanf(latest.c_str(), "%d.%d.%d", &majorL, &minorL, &patchL);

    if (majorL > majorC) return true;
    if (majorL < majorC) return false;
    if (minorL > minorC) return true;
    if (minorL < minorC) return false;
    return patchL > patchC;
}


// ================= CHECK UPDATE =================
bool checkForUpdate(String &newVersion) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, releasesAPI);
    http.addHeader("User-Agent", "ESP32");

    if (http.GET() != HTTP_CODE_OK) {
        http.end();
        return false;
    }

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());
    http.end();

    newVersion = doc["tag_name"].as<String>();
    newVersion.replace("v", "");

    return isVersionNewer(FW_VERSION, newVersion);
}

// ================= OTA =================
void performOTA(const String &newVersion) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Inici OTA...");
    display.display();
    
    FW_VERSION = newVersion;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client,
      "https://github.com/quimturon/habitacio/releases/latest/download/firmware.bin");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (http.GET() != HTTP_CODE_OK) {
        http.end();
        return;
    }

    int total = http.getSize();
    WiFiClient *stream = http.getStreamPtr();

    if (!Update.begin(total)) {
        http.end();
        return;
    }

    uint8_t buffer[256];
    int written = 0;

    display.drawRect(0, 20, 128, 10, SSD1306_WHITE);
    display.display();

    while (http.connected() && written < total) {
        size_t available = stream->available();
        if (available) {
            int r = stream->readBytes(buffer, min((int)available, 256));
            Update.write(buffer, r);
            written += r;
        }

        display.fillRect(0, 20, 128, 30, SSD1306_BLACK);
        display.drawRect(0, 20, 128, 10, SSD1306_WHITE);

        int w = map(written, 0, total, 0, 128);
        display.fillRect(0, 20, w, 10, SSD1306_WHITE);

        display.setCursor(0, 35);
        display.printf("%d %%", (written * 100) / total);
        display.display();
        delay(10);
    }

    if (Update.end()) {
        saveVersion(newVersion);
        delay(2000);
        ESP.restart();
    }

    http.end();
}
