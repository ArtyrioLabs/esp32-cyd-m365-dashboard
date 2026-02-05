#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "ScooterData.h"
#include "DisplayUI.h"
#include "M365BLE.h"

#define TOUCH_CLK   25
#define TOUCH_MISO  39
#define TOUCH_MOSI  32
#define TOUCH_CS    33
#define TOUCH_IRQ   36

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS);

TFT_eSPI tft = TFT_eSPI();
DisplayUI ui(tft);
M365BLE ble;

unsigned long lastUIUpdate = 0;
unsigned long lastTouch = 0;
const unsigned long UI_INTERVAL = 50;
const unsigned long TOUCH_DEBOUNCE = 200;

void setup() {
    Serial.begin(115200);
    Serial.println("\n[M365 Dashboard]");
    
    ui.begin();
    ui.showStatus("Starting...");
    
    // Touch on VSPI
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(TOUCH_IRQ, INPUT_PULLUP);
    
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touch.begin(touchSPI);
    touch.setRotation(1);
    
    ble.begin();
    ui.showStatus("Scanning...");
}

void loop() {
    unsigned long now = millis();
    
    ble.update();
    
    // UI update at 20 FPS
    if (now - lastUIUpdate >= UI_INTERVAL) {
        lastUIUpdate = now;
        
        if (ble.isConnected()) {
            ui.update(ble.getData());
        } else {
            static BLEState lastState = BLEState::DISCONNECTED;
            BLEState currentState = ble.getState();
            
            if (currentState != lastState) {
                ui.showStatus(ble.getStateName());
                lastState = currentState;
            }
        }
    }
    
    // Touch handling
    if (digitalRead(TOUCH_IRQ) == LOW && now - lastTouch >= TOUCH_DEBOUNCE) {
        delay(3);
        TS_Point p = touch.getPoint();
        
        if (p.z > 50) {
            ui.handleTouch(160, 120);
            lastTouch = now;
            
            while(digitalRead(TOUCH_IRQ) == LOW) delay(5);
        }
    }
}
