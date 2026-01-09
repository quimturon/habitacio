#include "wifi_manager.h"

void loadWiFiCredentials(char* ssid, char* pass) {
    EEPROM.begin(EEPROM_SIZE);

    int i = 0;
    while (i < 63 && EEPROM.read(SSID_ADDR + i) != '\0') {
        ssid[i] = EEPROM.read(SSID_ADDR + i);
        i++;
    }
    ssid[i] = '\0';

    int j = 0;
    while (j < 63 && EEPROM.read(PASS_ADDR + j) != '\0') {
        pass[j] = EEPROM.read(PASS_ADDR + j);
        j++;
    }
    pass[j] = '\0';
}

bool setup_wifi() {
    char ssid[64];
    char pass[64];

    loadWiFiCredentials(ssid, pass);

    Serial.println("Connectant a WiFi...");
    Serial.print("SSID: "); Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connectat!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("\nWiFi ERROR!");
    WiFi.disconnect(true);
    return false;
}

void ensureWiFi() {
    static unsigned long lastAttempt = 0;

    if (WiFi.status() == WL_CONNECTED) return;
    if (millis() - lastAttempt < 15000) return;

    lastAttempt = millis();
    Serial.println("⚠️ WiFi perdut, reconnectant...");
    setup_wifi();
}
