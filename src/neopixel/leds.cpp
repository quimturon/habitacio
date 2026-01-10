#include "leds.h"

#define NUM_STRIPS 2
#define NUM_LEDS 42
#define LED_PINS {19, 18}

LEDStrip ledStrips[NUM_STRIPS] = {
    {Adafruit_NeoPixel(NUM_LEDS, 19, NEO_GRBW + NEO_KHZ800), 255, 255, 1},
    {Adafruit_NeoPixel(NUM_LEDS, 18, NEO_GRB + NEO_KHZ800), 255, 255, 1}
};

void setupLEDs() {
    for(int i=0;i<NUM_STRIPS;i++){
        ledStrips[i].strip.begin();
        ledStrips[i].strip.show();
        ledStrips[i].strip.setBrightness(ledStrips[i].brightness);
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicialitzant ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(onDataRecv);
}

void LEDTask(void *pvParameters) {
    uint16_t hue = 0;
    while(true) {
        for(int s=0;s<NUM_STRIPS;s++){
            // Fading de brillantor
            if(ledStrips[s].brightness < ledStrips[s].targetBrightness) ledStrips[s].brightness++;
            else if(ledStrips[s].brightness > ledStrips[s].targetBrightness) ledStrips[s].brightness--;
            ledStrips[s].strip.setBrightness(ledStrips[s].brightness);

            // Prests
            switch(ledStrips[s].preset){
                case 1: // RAINBOW
                    for(int i=0;i<NUM_LEDS;i++){
                        uint32_t color = ledStrips[s].strip.ColorHSV((hue+i*65536/NUM_LEDS)%65536,255,255);
                        ledStrips[s].strip.setPixelColor(i,color);
                    }
                    break;
                case 2: // STATIC
                    for(int i=0;i<NUM_LEDS;i++) ledStrips[s].strip.setPixelColor(i,ledStrips[s].strip.Color(255,255,255,255));
                    break;
                case 3: // WARM
                    for(int i=0;i<NUM_LEDS;i++) ledStrips[s].strip.setPixelColor(i,ledStrips[s].strip.Color(255,120,0,255));
                    break;
                case 4: // COLORCYCLE
                    static uint16_t hueCycle = 0;
                    uint32_t color = ledStrips[s].strip.ColorHSV(hueCycle%65536,255,255);
                    for(int i=0;i<NUM_LEDS;i++) ledStrips[s].strip.setPixelColor(i,color);
                    hueCycle += 128;
                    break;
            }

            ledStrips[s].strip.show();
        }
        hue += 256;
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}

// Funcions enviaBrillantor i onDataRecv: pots actualitzar-les per enviar/recebre info de totes les tiras
void enviaBrillantor(int stripIndex) {
    uint8_t controladorAdress[] = {0x80, 0xF3, 0xDA, 0x65, 0x5C, 0xB8};

    if(stripIndex < 0 || stripIndex >= NUM_STRIPS) return;

    char msg[20];
    sprintf(msg, "bri%d:%d", stripIndex, ledStrips[stripIndex].targetBrightness);
    esp_now_send(controladorAdress, (uint8_t *)msg, strlen(msg)+1);

    Serial.printf("Brillantor enviada tira %d: %d\n", stripIndex, ledStrips[stripIndex].targetBrightness);
}


void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len){
    String msg = String((char*)incomingData); msg.trim();
    Serial.printf("📩 Rebut: %s\n",msg.c_str());
    if(msg=="toggle0"){
        for(int s=0;s<NUM_STRIPS;s++) ledStrips[s].targetBrightness = (ledStrips[s].targetBrightness>0)?0:150;
        enviaBrillantor(0);
    }
    else if(msg=="+bri0"){
        for(int s=0;s<NUM_STRIPS;s++) ledStrips[s].targetBrightness = min(ledStrips[s].targetBrightness+25,255);
        enviaBrillantor(0);
    }
    else if(msg=="-bri0"){
        for(int s=0;s<NUM_STRIPS;s++) ledStrips[s].targetBrightness = max(ledStrips[s].targetBrightness-25,5);
        enviaBrillantor(0);
    }
    else if(msg=="preset0"){
        for(int s=0;s<NUM_STRIPS;s++){
            ledStrips[s].preset += 1;
            if(ledStrips[s].preset>NUM_PRESETS) ledStrips[s].preset=1;
        }
        enviaBrillantor(1);
    }else if(msg=="toggle1"){
        for(int s=0;s<NUM_STRIPS;s++) ledStrips[s].targetBrightness = (ledStrips[s].targetBrightness>0)?0:150;
        enviaBrillantor(1);
    }
    else if(msg=="+bri1"){
        for(int s=0;s<NUM_STRIPS;s++) ledStrips[s].targetBrightness = min(ledStrips[s].targetBrightness+25,255);
        enviaBrillantor(1);
    }
    else if(msg=="-bri1"){
        for(int s=0;s<NUM_STRIPS;s++) ledStrips[s].targetBrightness = max(ledStrips[s].targetBrightness-25,5);
        enviaBrillantor(1);
    }
    else if(msg=="preset1"){
        for(int s=0;s<NUM_STRIPS;s++){
            ledStrips[s].preset += 1;
            if(ledStrips[s].preset>NUM_PRESETS) ledStrips[s].preset=1;
        }
        enviaBrillantor(1);
    }
}
