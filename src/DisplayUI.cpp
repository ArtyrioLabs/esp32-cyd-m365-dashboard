#include "DisplayUI.h"
#include <math.h>

DisplayUI::DisplayUI(TFT_eSPI& display) 
    : tft(display), lastSpeed(-1), lastBattery(255), lastOdometer(0), lastTrip(0),
      lastPower(-1), lastMode(255), lastHeadlight(false), lastError(255),
      lastVoltage(-1), lastCurrent(-1), lastTempESC(-100), lastTempBMS(-100),
      lastRideTime(0), lastConnected(false), firstDraw(true), needsClear(false),
      currentScreen(MAIN_SCREEN), historyIndex(0), lastHistoryUpdate(0) {
    for(int i = 0; i < HISTORY_SIZE; i++) {
        powerHistory[i] = 0;
        currentHistory[i] = 0;
    }
}

void DisplayUI::begin() {
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(COLOR_BG);
    tft.setSwapBytes(true);
}

void DisplayUI::showStatus(const char* message) {
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, 160, 120, 4);
    firstDraw = true;
    needsClear = true;
}

void DisplayUI::setScreen(Screen screen) {
    if (currentScreen != screen) {
        currentScreen = screen;
        needsClear = true;
        firstDraw = true;
    }
}

void DisplayUI::update(const ScooterData& data) {
    if (needsClear) {
        tft.fillScreen(COLOR_BG);
        needsClear = false;
        firstDraw = true;
    }
    
    updateHistory(data);
    
    switch (currentScreen) {
        case MAIN_SCREEN:
            drawMainScreen(data);
            break;
        case STATS_SCREEN:
            drawStatsScreen(data);
            break;
        case BATTERY_SCREEN:
            drawBatteryScreen(data);
            break;
    }
    
    firstDraw = false;
}

void DisplayUI::handleTouch(uint16_t x, uint16_t y) {
    currentScreen = (Screen)((currentScreen + 1) % 3);
    needsClear = true;
    firstDraw = true;
}

void DisplayUI::updateHistory(const ScooterData& data) {
    unsigned long now = millis();
    if (now - lastHistoryUpdate >= 1000) {
        powerHistory[historyIndex] = data.power;
        currentHistory[historyIndex] = fabs(data.current);
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        lastHistoryUpdate = now;
    }
}

void DisplayUI::formatTime(uint32_t seconds, char* buf) {
    uint32_t h = seconds / 3600;
    uint32_t m = (seconds % 3600) / 60;
    uint32_t s = seconds % 60;
    if (h > 0) {
        sprintf(buf, "%lu:%02lu:%02lu", h, m, s);
    } else {
        sprintf(buf, "%02lu:%02lu", m, s);
    }
}

// ========== MAIN SCREEN ==========
void DisplayUI::drawMainScreen(const ScooterData& data) {
    char buf[32];
    
    // Top status bar
    if (firstDraw || data.connected != lastConnected) {
        tft.fillRect(0, 0, 40, 28, COLOR_BG);
        drawBleStatus(8, 6, data.connected);
        lastConnected = data.connected;
    }
    
    if (firstDraw || data.mode != lastMode) {
        tft.fillRect(120, 0, 60, 28, COLOR_BG);
        drawModeIcon(125, 4, data.mode);
        lastMode = data.mode;
    }
    
    if (firstDraw || data.errorCode != lastError) {
        tft.fillRect(185, 0, 35, 28, COLOR_BG);
        if (data.errorCode > 0) {
            drawErrorIcon(190, 6, data.errorCode);
        }
        lastError = data.errorCode;
    }
    
    if (firstDraw || abs((int)data.batteryLevel - (int)lastBattery) >= 1) {
        tft.fillRect(235, 0, 85, 28, COLOR_BG);
        drawBatteryBar(240, 6, 45, 16, (int)data.batteryLevel);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        sprintf(buf, "%d%%", (int)data.batteryLevel);
        tft.drawString(buf, 290, 6, 2);
        lastBattery = (uint8_t)data.batteryLevel;
    }
    
    if (firstDraw) {
        tft.drawFastHLine(0, 30, 320, COLOR_DARKGRAY);
    }
    
    // Speedometer
    int speedInt = (int)data.speed;
    static int lastSpeedInt = -1;
    
    if (firstDraw || speedInt != lastSpeedInt) {
        tft.fillRect(80, 40, 160, 100, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        sprintf(buf, "%d", speedInt);
        tft.drawString(buf, 160, 85, 8);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("km/h", 160, 130, 2);
        lastSpeedInt = speedInt;
    }
    
    // Left column: V, A, W
    tft.setTextDatum(TL_DATUM);
    
    if (firstDraw || abs(data.voltage - lastVoltage) > 0.2f) {
        tft.fillRect(0, 32, 78, 38, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("V", 3, 33, 1);
        tft.setTextColor(COLOR_CYAN, COLOR_BG);
        sprintf(buf, "%.1f", data.voltage);
        tft.drawString(buf, 3, 44, 4);
        lastVoltage = data.voltage;
    }
    
    if (firstDraw || abs(data.current - lastCurrent) > 0.3f) {
        tft.fillRect(0, 72, 78, 38, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("A", 3, 73, 1);
        uint16_t col = data.current < 0 ? COLOR_GREEN : COLOR_YELLOW;
        tft.setTextColor(col, COLOR_BG);
        sprintf(buf, "%.1f", fabs(data.current));
        tft.drawString(buf, 3, 84, 4);
        lastCurrent = data.current;
    }
    
    if (firstDraw || abs(data.power - lastPower) > 5.0f) {
        tft.fillRect(0, 112, 78, 38, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("W", 3, 113, 1);
        uint16_t col = data.power < 0 ? COLOR_GREEN : COLOR_ORANGE;
        tft.setTextColor(col, COLOR_BG);
        sprintf(buf, "%d", abs((int)data.power));
        tft.drawString(buf, 3, 124, 4);
        lastPower = data.power;
    }
    
    // Right column: temperatures
    tft.setTextDatum(TR_DATUM);
    
    if (firstDraw || abs(data.tempESC - lastTempESC) > 1.0f) {
        tft.fillRect(245, 32, 75, 38, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("ESC", 317, 33, 1);
        uint16_t col = data.tempESC > 60 ? COLOR_RED : (data.tempESC > 45 ? COLOR_YELLOW : COLOR_GREEN);
        tft.setTextColor(col, COLOR_BG);
        sprintf(buf, "%.0fC", data.tempESC);
        tft.drawString(buf, 317, 44, 4);
        lastTempESC = data.tempESC;
    }
    
    float avgBMS = (data.tempBMS1 + data.tempBMS2) / 2.0f;
    if (firstDraw || abs(avgBMS - lastTempBMS) > 1.0f) {
        tft.fillRect(245, 72, 75, 38, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("BAT", 317, 73, 1);
        uint16_t col = avgBMS > 50 ? COLOR_RED : (avgBMS > 40 ? COLOR_YELLOW : COLOR_GREEN);
        tft.setTextColor(col, COLOR_BG);
        sprintf(buf, "%.0fC", avgBMS);
        tft.drawString(buf, 317, 84, 4);
        lastTempBMS = avgBMS;
    }
    
    // Bottom panel
    if (firstDraw) {
        tft.drawFastHLine(0, 155, 320, COLOR_DARKGRAY);
    }
    
    static uint32_t lastOdoDisplay = 0xFFFFFFFF;
    if (firstDraw || data.odometer != lastOdoDisplay) {
        tft.fillRect(0, 160, 105, 40, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("ODO", 5, 160, 1);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        sprintf(buf, "%.1f", data.odometer / 1000.0f);
        tft.drawString(buf, 5, 173, 4);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("km", 80, 190, 1);
        lastOdoDisplay = data.odometer;
    }
    
    static uint32_t lastTripDisplay = 0xFFFFFFFF;
    if (firstDraw || data.tripDistance != lastTripDisplay) {
        tft.fillRect(108, 160, 100, 40, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("TRIP", 113, 160, 1);
        tft.setTextColor(COLOR_ORANGE, COLOR_BG);
        sprintf(buf, "%.2f", data.tripDistance / 1000.0f);
        tft.drawString(buf, 113, 173, 4);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("km", 183, 190, 1);
        lastTripDisplay = data.tripDistance;
    }
    
    static uint32_t lastTimeDisplay = 0xFFFFFFFF;
    if (firstDraw || data.rideTime != lastTimeDisplay) {
        tft.fillRect(213, 160, 107, 40, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("TIME", 218, 160, 1);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        formatTime(data.rideTime, buf);
        tft.drawString(buf, 218, 173, 4);
        lastTimeDisplay = data.rideTime;
    }
    
    // Bottom row
    if (firstDraw) {
        tft.drawFastHLine(0, 205, 320, COLOR_DARKGRAY);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("RANGE", 5, 215, 1);
        tft.drawString("AVG", 115, 215, 1);
        tft.drawString("MAX", 215, 215, 1);
    }
    
    static float lastRange = -999;
    if (firstDraw || data.remainingRange != lastRange) {
        tft.fillRect(45, 212, 60, 20, COLOR_BG);
        tft.setTextColor(COLOR_GREEN, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        sprintf(buf, "%.1fkm", data.remainingRange);
        tft.drawString(buf, 45, 215, 2);
        lastRange = data.remainingRange;
    }
    
    float avgSpeed = 0;
    if (data.tripDistance > 0 && data.rideTime > 0) {
        avgSpeed = (data.tripDistance / 1000.0f) / (data.rideTime / 3600.0f);
    }
    static float lastAvgSpeed = -999;
    if (firstDraw || avgSpeed != lastAvgSpeed) {
        tft.fillRect(145, 212, 50, 20, COLOR_BG);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        sprintf(buf, "%.1f", avgSpeed);
        tft.drawString(buf, 145, 215, 2);
        lastAvgSpeed = avgSpeed;
    }
    
    static float maxSpeed = 0;
    if (data.speed > maxSpeed || firstDraw) {
        if (data.speed > maxSpeed) maxSpeed = data.speed;
        tft.fillRect(245, 212, 50, 20, COLOR_BG);
        tft.setTextColor(COLOR_ORANGE, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        sprintf(buf, "%.0f", maxSpeed);
        tft.drawString(buf, 245, 215, 2);
    }
}

// ========== STATS SCREEN ==========
void DisplayUI::drawStatsScreen(const ScooterData& data) {
    char buf[32];
    
    if (firstDraw) {
        tft.fillScreen(COLOR_BG);
        tft.setTextColor(COLOR_ORANGE, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("POWER & CURRENT", 160, 15, 4);
        tft.drawFastHLine(0, 35, 320, COLOR_DARKGRAY);
    }
    
    // Redraw graphs only when new data point added (every 1 sec)
    static unsigned long lastGraphDraw = 0;
    if (firstDraw || millis() - lastGraphDraw >= 1000) {
        drawGraph(10, 45, 300, 80, powerHistory, HISTORY_SIZE, COLOR_ORANGE, "POWER (W)");
        drawGraph(10, 135, 300, 80, currentHistory, HISTORY_SIZE, COLOR_YELLOW, "CURRENT (A)");
        lastGraphDraw = millis();
    }
    
    // Bottom stats - only update on change
    static int lastPwrStat = -999;
    static float lastCurStat = -999;
    static float lastVoltStat = -999;
    
    if (firstDraw || abs((int)data.power - lastPwrStat) > 2 || 
        fabs(data.current - lastCurStat) > 0.1f || fabs(data.voltage - lastVoltStat) > 0.2f) {
        
        tft.fillRect(0, 220, 320, 20, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        
        tft.setTextColor(COLOR_ORANGE, COLOR_BG);
        sprintf(buf, "PWR: %dW", abs((int)data.power));
        tft.drawString(buf, 20, 222, 2);
        
        tft.setTextColor(COLOR_YELLOW, COLOR_BG);
        sprintf(buf, "CUR: %.1fA", fabs(data.current));
        tft.drawString(buf, 130, 222, 2);
        
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        sprintf(buf, "V:%.1f", data.voltage);
        tft.drawString(buf, 250, 222, 2);
        
        lastPwrStat = (int)data.power;
        lastCurStat = data.current;
        lastVoltStat = data.voltage;
    }
}

// ========== BATTERY SCREEN ==========
void DisplayUI::drawBatteryScreen(const ScooterData& data) {
    char buf[32];
    
    if (firstDraw) {
        tft.fillScreen(COLOR_BG);
        tft.setTextColor(COLOR_GREEN, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("BATTERY", 160, 15, 4);
        tft.drawFastHLine(0, 32, 320, COLOR_DARKGRAY);
    }
    
    // Battery percentage - only redraw on change
    static int lastBattPercent = -1;
    if (firstDraw || (int)data.batteryLevel != lastBattPercent) {
        tft.fillRect(5, 38, 95, 85, COLOR_BG);
        tft.setTextColor(COLOR_WHITE, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        sprintf(buf, "%d", (int)data.batteryLevel);
        tft.drawString(buf, 55, 78, 7);
        lastBattPercent = (int)data.batteryLevel;
    }
    
    // Info panel - only redraw on change
    int infoX = 110;
    int infoY = 38;
    int lineH = 18;
    
    static float lastVoltDisp = -999;
    if (firstDraw || abs(data.voltage - lastVoltDisp) > 0.02f) {
        tft.fillRect(infoX, infoY, 205, lineH, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("Voltage", infoX, infoY + 2, 2);
        tft.setTextColor(COLOR_CYAN, COLOR_BG);
        tft.setTextDatum(TR_DATUM);
        sprintf(buf, "%.2fV", data.voltage);
        tft.drawString(buf, 315, infoY + 2, 2);
        lastVoltDisp = data.voltage;
    }
    
    infoY += lineH;
    static float lastCurrDisp = -999;
    if (firstDraw || abs(data.current - lastCurrDisp) > 0.05f) {
        tft.fillRect(infoX, infoY, 205, lineH, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("Current", infoX, infoY + 2, 2);
        tft.setTextColor(COLOR_YELLOW, COLOR_BG);
        tft.setTextDatum(TR_DATUM);
        sprintf(buf, "%.2fA", data.current);
        tft.drawString(buf, 315, infoY + 2, 2);
        lastCurrDisp = data.current;
    }
    
    infoY += lineH;
    static int lastPwrDisp = -999;
    if (firstDraw || abs((int)data.power - lastPwrDisp) > 2) {
        tft.fillRect(infoX, infoY, 205, lineH, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("Power", infoX, infoY + 2, 2);
        tft.setTextColor(COLOR_ORANGE, COLOR_BG);
        tft.setTextDatum(TR_DATUM);
        sprintf(buf, "%dW", (int)data.power);
        tft.drawString(buf, 315, infoY + 2, 2);
        lastPwrDisp = (int)data.power;
    }
    
    infoY += lineH;
    static int lastCapDisp = -1;
    if (firstDraw || data.capacityRemain != lastCapDisp) {
        tft.fillRect(infoX, infoY, 205, lineH, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("Capacity", infoX, infoY + 2, 2);
        tft.setTextColor(COLOR_GREEN, COLOR_BG);
        tft.setTextDatum(TR_DATUM);
        sprintf(buf, "%dmAh", data.capacityRemain);
        tft.drawString(buf, 315, infoY + 2, 2);
        lastCapDisp = data.capacityRemain;
    }
    
    infoY += lineH;
    float avgBMS = (data.tempBMS1 + data.tempBMS2) / 2.0f;
    static float lastTempDisp = -999;
    if (firstDraw || abs(data.tempESC - lastTempDisp) > 0.5f) {
        tft.fillRect(infoX, infoY, 205, lineH, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.drawString("Temp", infoX, infoY + 2, 2);
        uint16_t tempCol = (data.tempESC > 60 || avgBMS > 50) ? COLOR_RED : COLOR_ORANGE;
        tft.setTextColor(tempCol, COLOR_BG);
        tft.setTextDatum(TR_DATUM);
        sprintf(buf, "%.0f/%.0fC", data.tempESC, avgBMS);
        tft.drawString(buf, 315, infoY + 2, 2);
        lastTempDisp = data.tempESC;
    }
    
    // Cell voltages - only redraw if changed
    static float lastCells[10] = {0};
    bool cellsChanged = firstDraw;
    for (int i = 0; i < 10 && !cellsChanged; i++) {
        if (abs(data.cellVoltages[i] - lastCells[i]) > 0.005f) cellsChanged = true;
    }
    
    if (cellsChanged) {
        tft.drawFastHLine(0, 130, 320, COLOR_DARKGRAY);
        
        float minV = 5.0f, maxV = 0.0f;
        for (int i = 0; i < data.cellCount; i++) {
            if (data.cellVoltages[i] > 0) {
                if (data.cellVoltages[i] < minV) minV = data.cellVoltages[i];
                if (data.cellVoltages[i] > maxV) maxV = data.cellVoltages[i];
            }
        }
        
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("CELLS", 5, 138, 1);
        
        int startY = 152;
        int cellsPerRow = 5;
        
        for (int i = 0; i < data.cellCount && i < 10; i++) {
            int row = i / cellsPerRow;
            int col = i % cellsPerRow;
            int x = 5 + col * 63;
            int y = startY + row * 22;
            
            float v = data.cellVoltages[i];
            
            uint16_t cellColor;
            if (v <= 0) {
                cellColor = COLOR_DARKGRAY;
            } else if (v == minV && minV < maxV - 0.02f) {
                cellColor = COLOR_RED;
            } else if (v == maxV && maxV > minV + 0.02f) {
                cellColor = COLOR_CYAN;
            } else {
                cellColor = COLOR_GREEN;
            }
            
            tft.fillRect(x, y, 60, 18, COLOR_BG);
            tft.setTextColor(cellColor, COLOR_BG);
            tft.setTextDatum(TL_DATUM);
            if (v > 0) {
                sprintf(buf, "%.2fV", v);
            } else {
                sprintf(buf, "---");
            }
            tft.drawString(buf, x, y + 2, 2);
            lastCells[i] = data.cellVoltages[i];
        }
    }
    
    // Balance info - only redraw on change
    static float lastImbalance = -999;
    float imbalance = (data.maxCellVoltage - data.minCellVoltage) * 1000;
    if (firstDraw || abs(imbalance - lastImbalance) > 1) {
        tft.drawFastHLine(0, 200, 320, COLOR_DARKGRAY);
        tft.fillRect(0, 208, 150, 25, COLOR_BG);
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("BAL:", 10, 212, 2);
        tft.setTextColor(imbalance > 50 ? COLOR_RED : COLOR_GREEN, COLOR_BG);
        sprintf(buf, "%.0fmV", imbalance);
        tft.drawString(buf, 50, 212, 2);
        lastImbalance = imbalance;
    }
    
    if (firstDraw) {
        tft.setTextColor(COLOR_GRAY, COLOR_BG);
        tft.setTextDatum(TR_DATUM);
        tft.drawString("TAP TO RETURN", 310, 218, 1);
    }
}

// ========== UI COMPONENTS ==========
void DisplayUI::drawBatteryBar(int x, int y, int w, int h, int percent) {
    uint16_t col = percent > 50 ? COLOR_GREEN : (percent > 20 ? COLOR_YELLOW : COLOR_RED);
    
    tft.drawRect(x, y, w, h, col);
    tft.fillRect(x + w, y + h/4, 3, h/2, col);
    
    int innerW = w - 4;
    int innerH = h - 4;
    int segments = 5;
    int segW = (innerW - (segments - 1)) / segments;
    int filled = (percent * segments + 50) / 100;
    
    for (int i = 0; i < segments; i++) {
        int sx = x + 2 + i * (segW + 1);
        if (i < filled) {
            tft.fillRect(sx, y + 2, segW, innerH, col);
        } else {
            tft.fillRect(sx, y + 2, segW, innerH, COLOR_BG);
        }
    }
}

void DisplayUI::drawModeIcon(int x, int y, uint8_t mode) {
    const char* str;
    uint16_t col;
    
    switch (mode) {
        case 0: str = "ECO"; col = COLOR_GREEN; break;
        case 1: str = "D"; col = COLOR_CYAN; break;
        case 2: str = "S"; col = COLOR_RED; break;
        default: str = "-"; col = COLOR_GRAY;
    }
    
    tft.drawRoundRect(x, y, 50, 20, 5, col);
    tft.setTextColor(col);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(str, x + 25, y + 10, 2);
}

void DisplayUI::drawBleStatus(int x, int y, bool connected) {
    uint16_t col = connected ? COLOR_CYAN : COLOR_DARKGRAY;
    
    // Bluetooth rune icon
    tft.drawLine(x + 6, y, x + 6, y + 14, col);
    tft.drawLine(x + 6, y, x + 12, y + 4, col);
    tft.drawLine(x + 12, y + 4, x + 6, y + 7, col);
    tft.drawLine(x + 6, y + 14, x + 12, y + 10, col);
    tft.drawLine(x + 12, y + 10, x + 6, y + 7, col);
    tft.drawLine(x, y + 3, x + 6, y + 7, col);
    tft.drawLine(x, y + 11, x + 6, y + 7, col);
    
    tft.setTextColor(col, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(connected ? "BT" : "--", x + 15, y + 2, 1);
}

void DisplayUI::drawHeadlight(int x, int y, bool on) {
    uint16_t col = on ? COLOR_YELLOW : COLOR_DARKGRAY;
    
    tft.drawCircle(x + 7, y + 7, 6, col);
    if (on) {
        tft.fillCircle(x + 7, y + 7, 4, col);
        tft.drawLine(x + 15, y + 7, x + 20, y + 7, col);
        tft.drawLine(x + 13, y + 2, x + 17, y - 2, col);
        tft.drawLine(x + 13, y + 12, x + 17, y + 16, col);
    }
}

void DisplayUI::drawErrorIcon(int x, int y, uint8_t error) {
    tft.fillTriangle(x + 10, y, x, y + 16, x + 20, y + 16, COLOR_RED);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("!", x + 10, y + 10, 2);
}

void DisplayUI::drawGraph(int x, int y, int w, int h, float* data, int count, uint16_t color, const char* label) {
    tft.fillRect(x, y, w, h, COLOR_BG);
    tft.drawRect(x, y, w, h, COLOR_DARKGRAY);
    
    float minVal = 999999.0f;
    float maxVal = 0.0f;
    bool hasData = false;
    
    for (int i = 0; i < count; i++) {
        if (data[i] > 0) {
            hasData = true;
            if (data[i] < minVal) minVal = data[i];
            if (data[i] > maxVal) maxVal = data[i];
        }
    }
    
    bool isPowerGraph = (color == COLOR_ORANGE);
    
    if (!hasData) {
        minVal = 0;
        maxVal = isPowerGraph ? 100.0f : 2.0f;
    } else {
        maxVal = maxVal * 1.2f;
        minVal = 0;
        
        if (isPowerGraph) {
            // Power: round to nice values 50, 100, 200, 500, 1000W
            if (maxVal < 50) maxVal = 50;
            else if (maxVal < 100) maxVal = 100;
            else if (maxVal < 200) maxVal = 200;
            else if (maxVal < 500) maxVal = 500;
            else if (maxVal < 1000) maxVal = 1000;
            else maxVal = ceil(maxVal / 500) * 500;
        } else {
            // Current: round to nice values 0.5, 1, 2, 5, 10, 15, 20A
            if (maxVal < 0.5f) maxVal = 0.5f;
            else if (maxVal < 1.0f) maxVal = 1.0f;
            else if (maxVal < 2.0f) maxVal = 2.0f;
            else if (maxVal < 5.0f) maxVal = 5.0f;
            else if (maxVal < 10.0f) maxVal = 10.0f;
            else if (maxVal < 15.0f) maxVal = 15.0f;
            else if (maxVal < 20.0f) maxVal = 20.0f;
            else maxVal = ceil(maxVal / 5) * 5;
        }
    }
    
    float range = maxVal - minVal;
    if (range < 0.01f) range = 1.0f;
    
    tft.setTextColor(color);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(label, x + 5, y + 3, 1);
    
    char buf[16];
    tft.setTextColor(COLOR_GRAY);
    tft.setTextDatum(TR_DATUM);
    if (isPowerGraph) {
        sprintf(buf, "%.0f", maxVal);
    } else {
        if (maxVal < 10) sprintf(buf, "%.1f", maxVal);
        else sprintf(buf, "%.0f", maxVal);
    }
    tft.drawString(buf, x + w - 3, y + 3, 1);
    
    int graphH = h - 20;
    int graphY = y + 15;
    float stepX = (float)(w - 10) / count;
    
    int prevX = -1, prevY = -1;
    
    for (int i = 0; i < count; i++) {
        int idx = (historyIndex + i) % count;
        float val = data[idx];
        if (val < minVal) val = minVal;
        
        int py = graphY + graphH - (int)((val - minVal) / range * graphH);
        int px = x + 5 + (int)(i * stepX);
        
        if (py < graphY) py = graphY;
        if (py > graphY + graphH) py = graphY + graphH;
        
        if (prevX >= 0 && val > 0) {
            tft.drawLine(prevX, prevY, px, py, color);
        }
        
        prevX = px;
        prevY = py;
    }
}
