#ifndef M365_BLE_H
#define M365_BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "ScooterData.h"

#define M365_SERVICE_UUID   "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define M365_TX_CHAR_UUID   "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define M365_RX_CHAR_UUID   "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

#define ADDR_ESC    0x20
#define ADDR_BMS    0x22
#define ADDR_BLE    0x21

#define CMD_READ    0x01
#define CMD_WRITE   0x03

// ESC registers
#define REG_ESC_ERROR       0x1B
#define REG_ESC_ALARM       0x1C
#define REG_ESC_STATUS      0x1D
#define REG_ESC_MODE        0x1F
#define REG_ESC_BATTERY     0x22
#define REG_ESC_RANGE       0x25
#define REG_ESC_SPEED       0x26
#define REG_ESC_ODOMETER    0x29
#define REG_ESC_TRIP        0x2F
#define REG_ESC_UPTIME      0x32
#define REG_ESC_FRAME_TEMP  0x3E
#define REG_ESC_AVERAGE     0x65

// BMS registers
#define REG_BMS_TEMP        0x35
#define REG_BMS_CAPACITY    0x31
#define REG_BMS_FULL_CAP    0x32
#define REG_BMS_CURRENT     0x33
#define REG_BMS_VOLTAGE     0x34
#define REG_BMS_HEALTH      0x3B
#define REG_BMS_CELLS       0x40

enum class BLEState {
    DISCONNECTED,
    SCANNING,
    CONNECTING,
    CONNECTED,
    AUTHENTICATED
};

class M365BLE {
public:
    M365BLE();
    
    void begin();
    void update();
    
    bool startScan();
    bool connect();
    void disconnect();
    
    bool requestESCData();
    bool requestBMSData();
    bool requestCellVoltages();
    
    ScooterData getData() const { return scooterData; }
    BLEState getState() const { return state; }
    bool isConnected() const { return state == BLEState::CONNECTED || state == BLEState::AUTHENTICATED; }
    int8_t getRSSI() const { return rssi; }
    const char* getStateName() const;
    
    static void notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);
    
private:
    ScooterData scooterData;
    BLEState state;
    int8_t rssi;
    
    NimBLEClient* pClient;
    NimBLERemoteService* pService;
    NimBLERemoteCharacteristic* pTxChar;
    NimBLERemoteCharacteristic* pRxChar;
    
    BLEAdvertisedDevice* foundDevice;
    
    unsigned long lastRequest;
    unsigned long lastPoll;
    unsigned long connectionStartTime;
    uint8_t pollIndex;
    
    uint8_t rxBuffer[256];
    uint8_t rxIndex;
    
    bool sendCommand(uint8_t addr, uint8_t cmd, uint8_t reg, uint8_t len);
    uint16_t calculateChecksum(uint8_t* data, uint8_t len);
    
    void processResponse(uint8_t* data, size_t len);
    void parseESCResponse(uint8_t reg, uint8_t* data, uint8_t len);
    void parseBMSResponse(uint8_t reg, uint8_t* data, uint8_t len);
    
public:
    static M365BLE* instance;
    static void scanCallback(NimBLEAdvertisedDevice* device);
};

#endif
