#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <LiquidCrystal_I2C.h>

#include <AiEsp32RotaryEncoder.h>
#include <RTClib.h>

// ================= MENU SETTING =================
int menu = 0;
int menuIndex = 0;


// --- OLED --- 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Wi-Fi ---
struct WiFiCred {
    const char* ssid;
    const char* pass;
};

// --- OTA ---
String FW_VERSION;
bool otaInProgress = false;
bool needOTA = false;
bool otaConfirmation = false;
const char* releasesAPI  = "https://api.github.com/repos/quimturon/habitacio/releases/latest";
const char* firmwareURL = "https://github.com/quimturon/habitacio/releases/latest/download/firmware.bin";

// --- EEPROM ---
#define EEPROM_SIZE 160
#define SSID_ADDR 0       // Offset SSID
#define PASS_ADDR 64      // Offset password
#define VERSION_ADDR 128  // Offset firmware

// ================= PIN DEFINITIONS =================
#define ENC1_A 34
#define ENC1_B 35

#define ENC2_A 36
#define ENC2_B 39

#define ENC3_A 32
#define ENC3_B 33
#define ENC4_A 25
#define ENC4_B 26
#define ENC5_A 14
#define ENC5_B 12

#define ENCODER_STEPS 4

#define BUTTON1 15
#define BUTTON2 27
#define BUTTON3 4
#define BUTTON4 5
#define BUTTON5 3
#define ENC1_BTN 0
#define ENC2_BTN 1
#define ENC3_BTN 13
#define ENC4_BTN 23
#define ENC5_BTN -1 // no té botó

bool buttonState[] = {0,0,0,0,0,0,0,0,0,0};
bool buttonState1 = 0;
bool buttonState2 = 0;
bool buttonState3 = 0;
bool buttonState4 = 0;
bool buttonState5 = 0;
bool buttonState6 = 0;
bool buttonState7 = 0;
bool buttonState8 = 0;
bool buttonState9 = 0;
bool buttonState10 = 0;

bool lastButtonState1 = HIGH;
bool lastButtonState2 = HIGH;
bool lastButtonState3 = HIGH;
bool lastButtonState4 = HIGH;
bool lastButtonState5 = HIGH;
bool lastButtonState6 = HIGH;
bool lastButtonState7 = HIGH;
bool lastButtonState8 = HIGH;
bool lastButtonState9 = HIGH;
bool lastButtonState10 = HIGH;

// ================= ROTARY ENCODERS =================
AiEsp32RotaryEncoder enc1(ENC1_A, ENC1_B, ENC1_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc2(ENC2_A, ENC2_B, ENC2_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc3(ENC3_A, ENC3_B, ENC3_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc4(ENC4_A, ENC4_B, ENC4_BTN, -1, ENCODER_STEPS);
AiEsp32RotaryEncoder enc5(ENC5_B, ENC5_A, ENC5_BTN, -1, ENCODER_STEPS);

long encVal[5] = {0};

// ================= DISPLAYS =================
bool reescriure = false;
LiquidCrystal_I2C lcd2004(0x27, 20, 4);
LiquidCrystal_I2C lcd1602(0x24, 16, 2);

// ================= RTC =================
RTC_DS3231 rtc;

// ================= FUNCTION PROTOTYPES =================
void updateLCD2004();
void updateLCD1602();
void updateOLED();
void debugPrint(const String &msg);

// ================= ISR =================
void IRAM_ATTR readEncoder0() { enc1.readEncoder_ISR(); }
void IRAM_ATTR readEncoder1() { enc2.readEncoder_ISR(); }
void IRAM_ATTR readEncoder2() { enc3.readEncoder_ISR(); }
void IRAM_ATTR readEncoder3() { enc4.readEncoder_ISR(); }
void IRAM_ATTR readEncoder4() { enc5.readEncoder_ISR(); }

// ================= LCD / OLED =================
void updateLCD2004(int menu, int menuIndex) {
  lcd2004.clear();
  if (menu == 0){
        lcd2004.setCursor(0,0); lcd2004.printf("Firware actual:%s", FW_VERSION.c_str());
        if (needOTA) {
            lcd2004.setCursor(0,1); lcd2004.printf("Versio nova dispo:%s", FW_VERSION.c_str());
            lcd2004.setCursor(0,3); lcd2004.print("Actualitzant...");
        } else {
            lcd2004.setCursor(0,2); lcd2004.print("Tot actualitzat");
        }
  } else if (menu == 1){
        lcd2004.setCursor(0,0); lcd2004.print("Time Settings");
  } else if (menu == 2){
        lcd2004.setCursor(0,0); lcd2004.print("Lightning");
  }
}

void updateLCD1602(int menu, int menuIndex) {
    lcd1602.clear();
    if (menu == 0){
        lcd1602.setCursor(0,0); lcd1602.print("Firmware Update");
    }
    else if (menu == 1){
        lcd1602.setCursor(0,0); lcd1602.print("Time Set");
    }
    else if (menu == 2){
        lcd1602.setCursor(0,0); lcd1602.print("Lighting");
    }

}


void updateOLED(char* buf) {
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); display.println("ENC+BUTTONS");
  display.setCursor(0,10); display.printf("E1:%ld BE:%d   B:%d", encVal[0],buttonState6 ,buttonState1);
  display.setCursor(0,20); display.printf("E2:%ld BE:%d   B:%d", encVal[1],buttonState7 ,buttonState2);
  display.setCursor(0,30); display.printf("E3:%ld BE:%d   B:%d", encVal[2], buttonState8, buttonState3);
  display.setCursor(0,40); display.printf("E4:%ld BE:%d   B:%d", encVal[3], buttonState9, buttonState4);
  display.setCursor(0,50); display.printf("E5:%ld BE:%d   B:%d", encVal[4], buttonState10, buttonState5);
  // RTC
  display.setCursor(SCREEN_WIDTH-8*6, SCREEN_HEIGHT-8); display.print(buf);
  display.display();
}

// ================= DEBUG =================
void debugPrint(const String &msg){
  Serial.println(msg);
  display.setCursor(0, SCREEN_HEIGHT-8);
  display.fillRect(0, SCREEN_HEIGHT-8, SCREEN_WIDTH,8,SSD1306_BLACK);
  display.print(msg);
  display.display();
}

void saveVersion(const String& version) {
    EEPROM.begin(EEPROM_SIZE);
    int i = 0;
    for (; version[i] != '\0' && i < 31; i++) EEPROM.write(VERSION_ADDR + i, version[i]);
    EEPROM.write(VERSION_ADDR + i, '\0');
    EEPROM.commit();
    Serial.println("Firmware version saved to EEPROM.");
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

// --- OTA ---
bool getFirmwareURL(String &binURL) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, releasesAPI);
    http.addHeader("User-Agent", "ESP32");

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Error GET releases: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println("Error parsejant JSON");
        return false;
    }

    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        String name = asset["name"].as<String>();
        if (name == "firmware.bin") {
            binURL = asset["browser_download_url"].as<String>();
            Serial.print("Firmware URL: "); Serial.println(binURL);
            return true;
        }
    }

    Serial.println("No s'ha trobat firmware.bin a la release");
    return false;
}

void performOTA(const String &newVersionStr) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Inici OTA...");
    display.display();

    // Obtenir URL del firmware
    String binURL;
    if (!getFirmwareURL(binURL)) {
        Serial.println("No es pot obtenir URL del firmware");
        display.setCursor(0,10);
        display.println("Error OTA: no URL");
        display.display();
        delay(3000);
        return;
    }

    Serial.print("Descarregant firmware: "); Serial.println(binURL);

    WiFiClientSecure clientSecure;
    clientSecure.setInsecure();

    HTTPClient http;
    http.begin(clientSecure, binURL);
    http.addHeader("User-Agent", "ESP32");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    Serial.printf("HTTP code: %d\n", httpCode);
    if (httpCode != HTTP_CODE_OK) {
        display.setCursor(0,10);
        display.println("Error HTTP OTA");
        display.display();
        delay(3000);
        http.end();
        return;
    }

    int total = http.getSize();
    if (total <= 0) {
        display.setCursor(0,10);
        display.println("Error OTA: mida 0");
        display.display();
        delay(3000);
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();

    if (!Update.begin(total)) {
        display.setCursor(0,10);
        display.println("Update FAIL");
        display.display();
        http.end();
        return;
    }

    int written = 0;
    uint8_t buffer[256];

    // Dibuixar contorn barra una sola vegada
    display.drawRect(0, 20, 128, 10, SSD1306_WHITE);
    display.display();

    while (http.connected() && written < total) {
        int r = 0;
        size_t available = stream->available();
        if (available) {
            r = stream->readBytes(buffer, min((int)available, 256));
            Update.write(buffer, r);
            written += r;
        }

        // Netejar només la zona de progress abans de redibuixar
        display.fillRect(0, 20, 128, 30, SSD1306_BLACK); // neteja barra + percentatge
        display.drawRect(0, 20, 128, 10, SSD1306_WHITE); // contorn barra

        int w = map(written, 0, total, 0, 128);
        display.fillRect(0, 20, w, 10, SSD1306_WHITE); // barra progress

        display.setCursor(0, 35);
        display.printf("%d %%", (written*100)/total); // percentatge

        display.display();
        delay(10); // petit delay per refrescar OLED
    }

    if (Update.end()) {
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("OTA OK!");
        display.println("Reiniciant...");
        FW_VERSION = newVersionStr;
        saveVersion(newVersionStr);
        display.display();
        delay(2000);
        ESP.restart();
    } else {
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("OTA ERROR");
        display.display();
    }

    http.end();
}

// --- Funció per comparar versions ---
// Retorna true si versionB > versionA
bool isVersionNewer(const String& versionA, const String& versionB) {
    int majorA=0, minorA=0, patchA=0;
    int majorB=0, minorB=0, patchB=0;

    sscanf(versionA.c_str(), "%d.%d.%d", &majorA, &minorA, &patchA);
    sscanf(versionB.c_str(), "%d.%d.%d", &majorB, &minorB, &patchB);

    if (majorB > majorA) return true;
    if (majorB < majorA) return false;

    if (minorB > minorA) return true;
    if (minorB < minorA) return false;

    if (patchB > patchA) return true;
    return false;
}
bool checkForUpdate(String &newVersion) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure(); // HTTPS sense validar certificat

    HTTPClient http;
    http.begin(client, releasesAPI);
    http.addHeader("User-Agent", "ESP32"); // Només user-agent
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Error GET releases: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println("Error parsejant JSON");
        return false;
    }

    newVersion = doc["tag_name"].as<String>();
    newVersion.replace("v", ""); // treu la v si cal

    Serial.print("FW local: "); Serial.print(FW_VERSION);
    Serial.print(" | FW remote: "); Serial.println(newVersion);

    return isVersionNewer(FW_VERSION, newVersion);
}

// Funció per carregar SSID i password des de l'EEPROM
void loadWiFiCredentials(char* ssid, char* pass) {
    EEPROM.begin(160);

    // Llegir SSID
    int i = 0;
    while (i < 63 && EEPROM.read(i) != '\0') {
        ssid[i] = EEPROM.read(i);
        i++;
    }
    ssid[i] = '\0';

    // Llegir password
    int j = 0;
    while (j < 63 && EEPROM.read(64 + j) != '\0') {
        pass[j] = EEPROM.read(64 + j);
        j++;
    }
    pass[j] = '\0';
}

// --- Wi-Fi ---
bool setup_wifi() {
    char ssid[64];
    char pass[64];
    loadWiFiCredentials(ssid, pass);
    Serial.print("SSID: "); Serial.println(ssid);
Serial.print("Password: "); Serial.println(pass);
    Serial.println("Connectant a WiFi...");

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, pass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connectat!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    return true;
    } else {
        Serial.println("\nWiFi ERROR!");
        Serial.print("Status code: "); Serial.println(WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connectat!");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }else{
      Serial.println("\nWiFi ERROR!");
    }

    WiFi.disconnect(true);
    delay(500);
    return false; 
}
void ensureWiFi() {
    static unsigned long lastAttempt = 0;

    if (WiFi.status() == WL_CONNECTED) return;

    if (millis() - lastAttempt < 15000) return; // evita bucle constant

    lastAttempt = millis();
    Serial.println("⚠️ WiFi perdut, reconnectant...");
    setup_wifi();
}

// --- Setup ---
void setup() {

    Serial.begin(115200);
    Serial.println("Iniciant ESP32...");

    FW_VERSION = readVersion();
    Serial.print("Versió llegida EEPROM: "); 
    Serial.println(FW_VERSION);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("Iniciant ESP32...");
    display.setCursor(0,20);
    display.print("V");
    display.println(FW_VERSION);
    display.display();
    if (!setup_wifi()) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("ERROR WIFI");
      display.display();
      delay(5000);
      ESP.restart();  // o deixa'l offline si prefereixes
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("WIFI OK");
    display.display();

    // LCDs
    lcd2004.init(); 
    lcd1602.init(); 
    lcd2004.backlight();
    lcd1602.backlight();

    // OLED
    if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay(); display.display();
    }

    // Configura pins A/B dels encoders 3 i 4
    pinMode(ENC3_A, INPUT_PULLUP); pinMode(ENC3_B, INPUT_PULLUP);
    pinMode(ENC4_A, INPUT_PULLUP); pinMode(ENC4_B, INPUT_PULLUP);

    // Encoders
    enc1.begin(); enc1.setup(readEncoder0); enc1.setAcceleration(0);
    enc2.begin(); enc2.setup(readEncoder1); enc2.setAcceleration(0);
    enc3.begin(); enc3.setup(readEncoder2); enc3.setAcceleration(0);
    enc4.begin(); enc4.setup(readEncoder3); enc4.setAcceleration(0);
    enc5.begin(); enc5.setup(readEncoder4); enc5.setAcceleration(0);

    // Botons extra
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(BUTTON2, INPUT_PULLUP);
    pinMode(BUTTON3, INPUT_PULLUP);
    pinMode(BUTTON4, INPUT_PULLUP);
    pinMode(BUTTON5, INPUT_PULLUP);

    pinMode(ENC1_BTN, INPUT_PULLUP);
    pinMode(ENC2_BTN, INPUT_PULLUP);
    pinMode(ENC3_BTN, INPUT_PULLUP);
    pinMode(ENC4_BTN, INPUT_PULLUP);
    pinMode(ENC5_BTN, INPUT_PULLUP);

    // RTC
    if(!rtc.begin()) debugPrint("No s'ha trobat el RTC!");

}

// --- Loop ---
void loop() {
    ensureWiFi();

    // --- RTC ---
    DateTime now = rtc.now();
    char buf[9]; snprintf(buf,sizeof(buf),"%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // --- Encoders ---
    bool encoderMoved = false;

    if (enc1.encoderChanged()) { encVal[0] = enc1.readEncoder(); encoderMoved = true; }
    if (enc2.encoderChanged()) { encVal[1] = enc2.readEncoder(); encoderMoved = true; }
    if (enc3.encoderChanged()) { encVal[2] = enc3.readEncoder(); encoderMoved = true; }
    if (enc4.encoderChanged()) { encVal[3] = enc4.readEncoder(); encoderMoved = true; }
    if (enc5.encoderChanged()) { encVal[4] = enc5.readEncoder(); encoderMoved = true; }

    if (encoderMoved) reescriure = true;

    // --- Botons ---
    buttonState1 = digitalRead(BUTTON1);
    buttonState2 = digitalRead(BUTTON2);
    buttonState3 = digitalRead(BUTTON3);
    buttonState4 = digitalRead(BUTTON4);
    buttonState5 = digitalRead(BUTTON5);
    buttonState6 = digitalRead(ENC1_BTN);
    buttonState7 = digitalRead(ENC2_BTN);
    buttonState8 = digitalRead(ENC3_BTN);
    buttonState9 = digitalRead(ENC4_BTN);
    buttonState10 = digitalRead(ENC5_BTN);

    // Detectar canvi (només quan es prem)
    // Menu 1 = Firmware Update
    // Menu 2 = Time Set
    // Menu 3 = lighting
    if (lastButtonState1 == HIGH && buttonState1 == LOW) {
        reescriure = true;
        if (menu == 0) {
            if (otaConfirmation) {
                String newVersion;
                if (checkForUpdate(newVersion)) {
                    Serial.println("Nova versió disponible. Inici OTA...");
                    needOTA = true;
                    updateLCD2004(menu, menuIndex);
                } else {
                    Serial.println("Tens la última versió.");
                    needOTA = false;
                    updateLCD2004(menu, menuIndex);
                }
            }else{
                updateLCD2004(menu, menuIndex);
            }      
        }
        if (menu == 1) {
            //Accio hora 1
        }else if (menu == 2) {
            // canviar llum
        }
    }
    if (lastButtonState2 == HIGH && buttonState2 == LOW) {
        reescriure  = true;
        reescriure = true;
        if (menu == 0) {
            // Acció menú 0
        }
        if (menu == 1) {
            // Acció menú 1
        }
        if (menu == 2) {
            // Acció menú 2
        }
    }
    if (lastButtonState3 == HIGH && buttonState3 == LOW) {
        reescriure  = true;
        reescriure = true;
        if (menu == 0) {
            // Acció menú 0
        }
        if (menu == 1) {
            // Acció menú 1
        }
        if (menu == 2) {
            // Acció menú 2
        }
    }
    if (lastButtonState4 == HIGH && buttonState4 == LOW) {
        reescriure = true;
        reescriure = true;
        if (menu == 0) {
            // Acció menú 0
        }
        if (menu == 1) {
            // Acció menú 1
        }
        if (menu == 2) {
            // Acció menú 2
        }
    }
    if (lastButtonState5 == HIGH && buttonState5 == LOW) {
        if (menu > 1) menu = 0;
        else menu += 1;   // canvia de menú
        reescriure = true;
    }
    if (lastButtonState6 == HIGH && buttonState6 == LOW) {
        reescriure = true;
    }
    if (lastButtonState7 == HIGH && buttonState7 == LOW) {
        reescriure = true;
    }
    if (lastButtonState8 == HIGH && buttonState8 == LOW) {
        reescriure = true;
    }
    if (lastButtonState9 == HIGH && buttonState9 == LOW) {
        reescriure = true;
    }
    if (lastButtonState10 == HIGH && buttonState10 == LOW) {
        reescriure = true;
    }

    // Guardar estat anterior
    lastButtonState1 = buttonState1;
    lastButtonState2 = buttonState2;
    lastButtonState3 = buttonState3;
    lastButtonState4 = buttonState4;
    lastButtonState5 = buttonState5;
    lastButtonState6 = buttonState6;
    lastButtonState7 = buttonState7;
    lastButtonState8 = buttonState8;
    lastButtonState9 = buttonState9;
    lastButtonState10 = buttonState10;


    updateOLED(buf);
    if (reescriure) {
        updateLCD2004(menu, menuIndex);
        updateLCD1602(menu, menuIndex);
        reescriure = false;
    }
}