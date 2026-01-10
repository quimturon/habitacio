//llibreries de sistema
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
//llibreries de wifi i dades
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>
#include <esp_now.h>
//llibreries de pantalles
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <LiquidCrystal_I2C.h>
//llibreries de llums
#include <Adafruit_NeoPixel.h>
//llibreries de inputs
#include <AiEsp32RotaryEncoder.h>
#include <RTClib.h>
//llibreries propies
#include "ota/ota.h"
#include "wifi/wifi_manager.h"
#include "neopixel/leds.h"

// ================= MENU SETTING =================
int menu = 0;
int menuIndex = 0;
DateTime lastUpdate;

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
String NEW_VERSION;
bool otaInProgress = false;
int needOTA = 0;
const char* releasesAPI  = "https://api.github.com/repos/quimturon/habitacio/releases/latest";
const char* firmwareURL = "https://github.com/quimturon/habitacio/releases/latest/download/firmware.bin";

// --- EEPROM ---
#define EEPROM_SIZE 160
#define SSID_ADDR 0       // Offset SSID
#define PASS_ADDR 64      // Offset password
#define VERSION_ADDR 128  // Offset firmware

// --- espNOW ---
uint8_t controladorAdress[] = {0x80, 0xF3, 0xDA, 0x65, 0x5C, 0xB8};

// --- ledStrips ---
uint8_t bri0;
uint8_t bri1;
uint8_t targetBri0;
uint8_t targetBri1;
uint8_t lastBri0;
uint8_t lastBri1;
uint8_t briSteps = 25;

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
        lcd2004.setCursor(0,0); 
        lcd2004.print("Firmware: "); 
        lcd2004.print(FW_VERSION);

        if (needOTA == 1) {
            lcd2004.setCursor(0,2); lcd2004.print("Nova versio:"); 
            lcd2004.print(NEW_VERSION);
            lcd2004.setCursor(0,3); lcd2004.print("Actualitzant..."); 

        } else if (needOTA == 2) {
            lcd2004.setCursor(0,2); 
            lcd2004.print("Tot actualitzat el:");
            lcd2004.setCursor(0,3);
            char buf[17]; // DD/MM/YYYY HH:MM
            sprintf(buf, "%02d/%02d/%04d %02d:%02d",
                    lastUpdate.day(), lastUpdate.month(), lastUpdate.year(),
                    lastUpdate.hour(), lastUpdate.minute());
            lcd2004.print(buf);
        }
    } else if (menu == 1){
        lcd2004.setCursor(0,0); lcd2004.printf("Despatx  %d%%", ledStrips[0].targetBrightness);
        lcd2004.setCursor(0,1); lcd2004.printf("Paret    %d%%", ledStrips[1].targetBrightness);
        lcd2004.setCursor(0,2); lcd2004.printf("Tauleta  %d%%", ledStrips[2].targetBrightness);
        lcd2004.setCursor(0,3); lcd2004.printf("General  %d%%", ledStrips[3].targetBrightness);
    }
}

void updateLCD1602(int menu, int menuIndex) {
    lcd1602.clear();
    if (menu == 0){
        lcd1602.setCursor(4,0); lcd1602.print("Firmware");
    }
    else if (menu == 1){
        lcd1602.setCursor(5,0); lcd1602.print("Llums");
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

    // Setup LEDs i ESP-NOW
    setupLEDs();

    xTaskCreatePinnedToCore(
        LEDTask,
        "LED Task",
        4000,
        NULL,
        1,
        NULL,
        0
    );

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, controladorAdress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(controladorAdress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("❌ Error afegint el peer");
        return;
    }
  }

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

    if(enc1.encoderChanged()) { // controla la tira 1
        encVal[0] = enc1.readEncoder();
        int delta = enc1.readEncoder();
        if(delta>0) ledStrips[0].targetBrightness = min(ledStrips[0].targetBrightness+briSteps,255);
        else if(delta<0) ledStrips[0].targetBrightness = max(ledStrips[0].targetBrightness-briSteps,0);
        enviaBrillantor(0);
        enc1.reset();
        reescriure = true;
    }

    if(enc2.encoderChanged()) { // controla la tira 1
        encVal[1] = enc2.readEncoder();
        int delta = enc2.readEncoder();
        if(delta>0) ledStrips[1].targetBrightness = min(ledStrips[1].targetBrightness+briSteps,255);
        else if(delta<0) ledStrips[1].targetBrightness = max(ledStrips[1].targetBrightness-briSteps,5);
        enviaBrillantor(1);
        enc2.reset();
        reescriure = true;
    }


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
            String newVersion;
            if (checkForUpdate(newVersion)) {
                Serial.println("Nova versió disponible. Inici OTA...");
                needOTA = 1;
                NEW_VERSION = newVersion;          // Guardem per mostrar al menú
                updateLCD2004(menu, menuIndex);    // Mostrem la nova versió
                performOTA(newVersion);            // Inici OTA
            } else {
                Serial.println("Tens la última versió.");
                lastUpdate = rtc.now();
                needOTA = 2;
                updateLCD2004(menu, menuIndex);
            }
        }
        if (menu == 1) {
            if (ledStrips[0].targetBrightness > 0) {
                lastBri0 = ledStrips[0].targetBrightness;
                ledStrips[0].targetBrightness = 0;  // apaga la tira 1
            } else {
                ledStrips[0].targetBrightness = lastBri0;  // posa brillantor mitjana
            }
            enviaBrillantor(0);  // envia només per la tira 1
        }
    }
    if (lastButtonState2 == HIGH && buttonState2 == LOW) {
        reescriure  = true;
        reescriure = true;
        if (menu == 0) {
            // Acció menú 0
        }
        if (menu == 1) {
            if (ledStrips[1].targetBrightness > 0) {
                lastBri1 = ledStrips[1].targetBrightness;
                ledStrips[1].targetBrightness = 0;  // apaga la tira 2
            } else {
                ledStrips[1].targetBrightness = lastBri1;  // posa brillantor mitjana
            }
            enviaBrillantor(1);  // envia només per la tira 1
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
    }
    if (lastButtonState5 == HIGH && buttonState5 == LOW) {
        if (menu == 1) menu = 0;
        else menu += 1;   // canvia de menú
        reescriure = true;
    }
    if (lastButtonState6 == HIGH && buttonState6 == LOW) {
        ledStrips[0].preset += 1;
        if(ledStrips[0].preset > NUM_PRESETS) ledStrips[0].preset = 1;
        reescriure = true;
    }
    if (lastButtonState7 == HIGH && buttonState7 == LOW) {
        ledStrips[1].preset += 1;
        if(ledStrips[1].preset > NUM_PRESETS) ledStrips[1].preset = 1;
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
