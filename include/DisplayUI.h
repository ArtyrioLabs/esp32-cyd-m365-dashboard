#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <TFT_eSPI.h>
#include "ScooterData.h"

// Colors
#define COLOR_BG        0x0000
#define COLOR_ORANGE    0xFD20
#define COLOR_WHITE     0xFFFF
#define COLOR_GREEN     0x07E0
#define COLOR_YELLOW    0xFFE0
#define COLOR_RED       0xF800
#define COLOR_GRAY      0x7BEF
#define COLOR_DARKGRAY  0x4208
#define COLOR_CYAN      0x07FF
#define COLOR_BLUE      0x001F

#define HISTORY_SIZE 60

enum Screen {
    MAIN_SCREEN = 0,
    STATS_SCREEN = 1,
    BATTERY_SCREEN = 2
};

class DisplayUI {
public:
    DisplayUI(TFT_eSPI& display);
    void begin();
    void update(const ScooterData& data);
    void handleTouch(uint16_t x, uint16_t y);
    void showStatus(const char* message);
    void setScreen(Screen screen);
    
private:
    TFT_eSPI& tft;
    
    // Cached values
    float lastSpeed;
    uint8_t lastBattery;
    uint32_t lastOdometer;
    uint32_t lastTrip;
    float lastPower;
    uint8_t lastMode;
    bool lastHeadlight;
    uint8_t lastError;
    float lastVoltage;
    float lastCurrent;
    float lastTempESC;
    float lastTempBMS;
    uint32_t lastRideTime;
    bool lastConnected;
    bool firstDraw;
    bool needsClear;
    Screen currentScreen;
    
    // Graph history
    float powerHistory[HISTORY_SIZE];
    float currentHistory[HISTORY_SIZE];
    int historyIndex;
    unsigned long lastHistoryUpdate;
    
    void drawMainScreen(const ScooterData& data);
    void drawStatsScreen(const ScooterData& data);
    void drawBatteryScreen(const ScooterData& data);
    
    void drawBatteryBar(int x, int y, int w, int h, int percent);
    void drawModeIcon(int x, int y, uint8_t mode);
    void drawBleStatus(int x, int y, bool connected);
    void drawHeadlight(int x, int y, bool on);
    void drawErrorIcon(int x, int y, uint8_t error);
    void updateHistory(const ScooterData& data);
    void drawGraph(int x, int y, int w, int h, float* data, int count, uint16_t color, const char* label);
    void formatTime(uint32_t seconds, char* buf);
};

#endif
