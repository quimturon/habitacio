#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_now.h>

// === Constants ===
#define NUM_PRESETS 4

// Estrcutura de tira
struct LEDStrip {
    Adafruit_NeoPixel strip;
    uint8_t brightness;
    uint8_t targetBrightness;
    uint8_t preset;
    uint8_t briSteps;
};

// Declaració de tiras
extern LEDStrip ledStrips[];  
extern const int NUM_STRIPS;

// Funcions
void setupLEDs();
void LEDTask(void *pvParameters);
void enviaBrillantor(int stripIndex = 0);
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

#endif
