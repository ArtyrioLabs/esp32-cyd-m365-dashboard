#ifndef SCOOTER_DATA_H
#define SCOOTER_DATA_H

#include <Arduino.h>

struct ScooterData {
    // Core telemetry
    float speed;
    float averageSpeed;
    float batteryLevel;
    float voltage;
    float current;
    float power;
    uint32_t odometer;
    uint32_t tripDistance;
    uint32_t rideTime;
    float remainingRange;
    
    // Temperature
    float temperature;
    float tempESC;
    float tempBMS1;
    float tempBMS2;
    
    // Battery capacity
    uint16_t capacityRemain;
    uint16_t capacityFull;
    
    // Cell voltages (10S config)
    float cellVoltages[10];
    uint8_t cellCount;
    float minCellVoltage;
    float maxCellVoltage;
    float cellImbalance;
    
    // Status
    bool isCharging;
    bool isLocked;
    uint8_t errorCode;
    uint8_t mode;
    bool headlight;
    bool taillight;
    
    // Connection
    bool connected;
    int8_t rssi;
    
    ScooterData() {
        speed = 0;
        averageSpeed = 0;
        batteryLevel = 0;
        voltage = 0;
        current = 0;
        power = 0;
        odometer = 0;
        tripDistance = 0;
        rideTime = 0;
        remainingRange = 0;
        temperature = 0;
        tempESC = 0;
        tempBMS1 = 0;
        tempBMS2 = 0;
        capacityRemain = 0;
        capacityFull = 12800;
        cellCount = 10;
        for (int i = 0; i < 10; i++) cellVoltages[i] = 0;
        minCellVoltage = 0;
        maxCellVoltage = 0;
        cellImbalance = 0;
        isCharging = false;
        isLocked = false;
        errorCode = 0;
        mode = 255;
        headlight = false;
        taillight = false;
        connected = false;
        rssi = 0;
    }
};

#endif
