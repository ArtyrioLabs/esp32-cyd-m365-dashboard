#include "M365BLE.h"

M365BLE* M365BLE::instance = nullptr;

class M365ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("[BLE] Connected");
    }
    
    void onDisconnect(NimBLEClient* pClient, int reason) {
        Serial.printf("[BLE] Disconnected: %d\n", reason);
    }
};

class M365ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* device) override {
        String name = device->getName().c_str();
        
        if (name.startsWith("MIScooter") || 
            name.startsWith("Mi Electric") ||
            name.indexOf("M365") >= 0 ||
            name.indexOf("Scooter") >= 0) {
            
            Serial.printf("[BLE] Found: %s\n", name.c_str());
            NimBLEDevice::getScan()->stop();
            
            if (M365BLE::instance) {
                M365BLE::scanCallback(device);
            }
        }
    }
};

static M365ScanCallbacks scanCallbacks;
static M365ClientCallbacks clientCallbacks;

M365BLE::M365BLE() {
    state = BLEState::DISCONNECTED;
    rssi = 0;
    pClient = nullptr;
    pService = nullptr;
    pTxChar = nullptr;
    pRxChar = nullptr;
    foundDevice = nullptr;
    lastRequest = 0;
    lastPoll = 0;
    connectionStartTime = 0;
    pollIndex = 0;
    rxIndex = 0;
    instance = this;
}

void M365BLE::begin() {
    Serial.println("[BLE] Init");
    
    NimBLEDevice::init("M365Dashboard");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(false, false, false);
    
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(&clientCallbacks, false);
    pClient->setConnectionParams(12, 12, 0, 400);
    pClient->setConnectTimeout(10);
    
    startScan();
}

bool M365BLE::startScan() {
    if (state == BLEState::SCANNING) return true;
    
    state = BLEState::SCANNING;
    foundDevice = nullptr;
    
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCallbacks, false);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    
    Serial.println("[BLE] Scanning...");
    pScan->start(30, nullptr, false);
    
    return true;
}

void M365BLE::scanCallback(NimBLEAdvertisedDevice* device) {
    if (instance) {
        instance->foundDevice = device;
        instance->state = BLEState::CONNECTING;
    }
}

bool M365BLE::connect() {
    if (!foundDevice) return false;
    
    state = BLEState::CONNECTING;
    Serial.printf("[BLE] Connecting to %s\n", foundDevice->getAddress().toString().c_str());
    
    if (!pClient->connect(foundDevice)) {
        Serial.println("[BLE] Connect failed");
        state = BLEState::DISCONNECTED;
        return false;
    }
    
    rssi = pClient->getRssi();
    
    pService = pClient->getService(M365_SERVICE_UUID);
    if (!pService) {
        Serial.println("[BLE] Service not found");
        pClient->disconnect();
        state = BLEState::DISCONNECTED;
        return false;
    }
    
    pTxChar = pService->getCharacteristic(M365_TX_CHAR_UUID);
    pRxChar = pService->getCharacteristic(M365_RX_CHAR_UUID);
    
    if (!pTxChar || !pRxChar) {
        Serial.println("[BLE] Characteristics not found");
        pClient->disconnect();
        state = BLEState::DISCONNECTED;
        return false;
    }
    
    if (pRxChar->canNotify()) {
        pRxChar->subscribe(true, notifyCallback, true);
    }
    
    state = BLEState::CONNECTED;
    scooterData.connected = true;
    scooterData.rssi = rssi;
    connectionStartTime = millis();
    
    Serial.println("[BLE] Ready");
    return true;
}

void M365BLE::disconnect() {
    if (pClient && pClient->isConnected()) {
        pClient->disconnect();
    }
    state = BLEState::DISCONNECTED;
    scooterData.connected = false;
    foundDevice = nullptr;
}

void M365BLE::notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    if (instance) {
        instance->processResponse(pData, length);
    }
}

void M365BLE::processResponse(uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (rxIndex == 0 && data[i] != 0x55) continue;
        if (rxIndex == 1 && data[i] != 0xAA) { rxIndex = 0; continue; }
        
        rxBuffer[rxIndex++] = data[i];
        
        if (rxIndex >= 4) {
            uint8_t pktLen = rxBuffer[2];
            if (rxIndex >= pktLen + 4 + 2) {
                uint8_t addr = rxBuffer[3];
                uint8_t reg = (pktLen > 1) ? rxBuffer[5] : 0;
                
                if (addr == 0x23) {
                    parseESCResponse(reg, &rxBuffer[6], pktLen - 2);
                } else if (addr == 0x25) {
                    parseBMSResponse(reg, &rxBuffer[6], pktLen - 2);
                }
                
                rxIndex = 0;
            }
        }
        
        if (rxIndex >= sizeof(rxBuffer)) rxIndex = 0;
    }
}

void M365BLE::parseESCResponse(uint8_t reg, uint8_t* data, uint8_t len) {
    switch (reg) {
        case REG_ESC_SPEED:
            if (len >= 2) {
                int16_t raw = data[0] | (data[1] << 8);
                scooterData.speed = abs(raw) / 1000.0f;
            }
            break;
            
        case REG_ESC_BATTERY:
            scooterData.batteryLevel = data[0];
            break;
            
        case REG_ESC_MODE:
            {
                uint8_t rawMode = data[0] & 0x03;
                if (rawMode == 0) scooterData.mode = 1;
                else if (rawMode == 1) scooterData.mode = 0;
                else scooterData.mode = 2;
            }
            break;
            
        case REG_ESC_ODOMETER:
            if (len >= 4) {
                scooterData.odometer = data[0] | (data[1] << 8) | 
                                       (data[2] << 16) | (data[3] << 24);
            }
            break;
            
        case REG_ESC_TRIP:
            if (len >= 2) {
                scooterData.tripDistance = data[0] | (data[1] << 8);
            }
            break;
            
        case REG_ESC_RANGE:
            if (len >= 2) {
                uint16_t rawRange = data[0] | (data[1] << 8);
                scooterData.remainingRange = rawRange / 100.0f;
            }
            break;
            
        case REG_ESC_AVERAGE:
            if (len >= 2) {
                int16_t rawAvg = data[0] | (data[1] << 8);
                scooterData.averageSpeed = rawAvg / 1000.0f;
            }
            break;
            
        case REG_ESC_UPTIME:
            if (len >= 4) {
                scooterData.rideTime = data[0] | (data[1] << 8) | 
                                       (data[2] << 16) | (data[3] << 24);
            }
            break;
            
        case REG_ESC_FRAME_TEMP:
            if (len >= 2) {
                int16_t raw = data[0] | (data[1] << 8);
                scooterData.tempESC = raw / 10.0f;
                scooterData.temperature = scooterData.tempESC;
            }
            break;
            
        case REG_ESC_ERROR:
            scooterData.errorCode = data[0];
            break;
            
        case 0x21:
            if (len >= 1) {
                scooterData.headlight = (data[0] & 0x01) != 0;
            }
            break;
    }
}

void M365BLE::parseBMSResponse(uint8_t reg, uint8_t* data, uint8_t len) {
    if (reg == REG_BMS_CURRENT && len >= 4) {
        int16_t rawCurr = data[0] | (data[1] << 8);
        scooterData.current = rawCurr / 100.0f;
        
        uint16_t rawVolt = data[2] | (data[3] << 8);
        scooterData.voltage = rawVolt / 100.0f;
        
        scooterData.power = scooterData.voltage * abs(scooterData.current);
        return;
    }
    
    switch (reg) {
        case REG_BMS_VOLTAGE:
            if (len >= 2) {
                uint16_t raw = data[0] | (data[1] << 8);
                scooterData.voltage = raw / 100.0f;
            }
            break;
            
        case REG_BMS_CURRENT:
            if (len >= 2) {
                int16_t raw = data[0] | (data[1] << 8);
                scooterData.current = raw / 100.0f;
                scooterData.power = scooterData.voltage * scooterData.current;
            }
            break;
            
        case REG_BMS_TEMP:
            if (len >= 2) {
                scooterData.tempBMS1 = (float)data[0] - 20.0f;
                scooterData.tempBMS2 = (float)data[1] - 20.0f;
            }
            break;
            
        case REG_BMS_CELLS:
            for (int i = 0; i < 10 && (i * 2 + 1) < len; i++) {
                uint16_t mv = data[i * 2] | (data[i * 2 + 1] << 8);
                scooterData.cellVoltages[i] = mv / 1000.0f;
            }
            
            scooterData.minCellVoltage = 5.0f;
            scooterData.maxCellVoltage = 0.0f;
            for (int i = 0; i < 10; i++) {
                if (scooterData.cellVoltages[i] > 0.1f) {
                    if (scooterData.cellVoltages[i] < scooterData.minCellVoltage)
                        scooterData.minCellVoltage = scooterData.cellVoltages[i];
                    if (scooterData.cellVoltages[i] > scooterData.maxCellVoltage)
                        scooterData.maxCellVoltage = scooterData.cellVoltages[i];
                }
            }
            scooterData.cellImbalance = scooterData.maxCellVoltage - scooterData.minCellVoltage;
            break;
            
        case REG_BMS_CAPACITY:
            if (len >= 2) {
                scooterData.capacityRemain = data[0] | (data[1] << 8);
            }
            break;
            
        case REG_BMS_FULL_CAP:
            if (len >= 2) {
                scooterData.capacityFull = data[0] | (data[1] << 8);
            }
            break;
    }
}

uint16_t M365BLE::calculateChecksum(uint8_t* data, uint8_t len) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (sum ^ 0xFFFF);
}

bool M365BLE::sendCommand(uint8_t addr, uint8_t cmd, uint8_t reg, uint8_t len) {
    if (!pTxChar || !pClient->isConnected()) return false;
    
    uint8_t packet[9];
    packet[0] = 0x55;
    packet[1] = 0xAA;
    packet[2] = 0x03;
    packet[3] = addr;
    packet[4] = cmd;
    packet[5] = reg;
    packet[6] = len;
    
    uint16_t crc = calculateChecksum(&packet[2], 5);
    packet[7] = crc & 0xFF;
    packet[8] = (crc >> 8) & 0xFF;
    
    return pTxChar->writeValue(packet, 9, false);
}

bool M365BLE::requestESCData() {
    if (state != BLEState::CONNECTED && state != BLEState::AUTHENTICATED) return false;
    return sendCommand(ADDR_ESC, CMD_READ, REG_ESC_BATTERY, 0x14);
}

bool M365BLE::requestBMSData() {
    if (state != BLEState::CONNECTED && state != BLEState::AUTHENTICATED) return false;
    return sendCommand(ADDR_BMS, CMD_READ, REG_BMS_CURRENT, 0x04);
}

bool M365BLE::requestCellVoltages() {
    if (state != BLEState::CONNECTED && state != BLEState::AUTHENTICATED) return false;
    return sendCommand(ADDR_BMS, CMD_READ, REG_BMS_CELLS, 0x14);
}

void M365BLE::update() {
    unsigned long now = millis();
    
    switch (state) {
        case BLEState::DISCONNECTED:
            if (now - lastRequest > 5000) {
                startScan();
                lastRequest = now;
            }
            break;
            
        case BLEState::SCANNING:
            if (foundDevice) {
                connect();
            }
            break;
            
        case BLEState::CONNECTING:
            if (foundDevice && !pClient->isConnected()) {
                connect();
            }
            break;
            
        case BLEState::CONNECTED:
        case BLEState::AUTHENTICATED:
            if (!pClient->isConnected()) {
                state = BLEState::DISCONNECTED;
                scooterData.connected = false;
                break;
            }
            
            scooterData.rideTime = (now - connectionStartTime) / 1000;
            
            if (scooterData.remainingRange <= 0) {
                scooterData.remainingRange = scooterData.batteryLevel * 0.45f;
            }
            
            if (now - lastRequest > 5000) {
                rssi = pClient->getRssi();
                scooterData.rssi = rssi;
                lastRequest = now;
            }
            
            if (now - lastPoll > 100) {
                lastPoll = now;
                
                switch (pollIndex % 16) {
                    case 0:
                    case 4:
                    case 8:
                    case 12:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_SPEED, 2);
                        break;
                    case 1:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_BATTERY, 1);
                        break;
                    case 2:
                        requestBMSData();
                        break;
                    case 3:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_MODE, 1);
                        break;
                    case 5:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_ODOMETER, 4);
                        break;
                    case 6:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_TRIP, 2);
                        break;
                    case 7:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_FRAME_TEMP, 2);
                        break;
                    case 9:
                        sendCommand(ADDR_BMS, CMD_READ, REG_BMS_TEMP, 2);
                        break;
                    case 10:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_RANGE, 2);
                        break;
                    case 11:
                        sendCommand(ADDR_BMS, CMD_READ, REG_BMS_CAPACITY, 2);
                        break;
                    case 13:
                        requestCellVoltages();
                        break;
                    case 14:
                        sendCommand(ADDR_ESC, CMD_READ, REG_ESC_AVERAGE, 2);
                        break;
                    case 15:
                        sendCommand(ADDR_BMS, CMD_READ, REG_BMS_FULL_CAP, 2);
                        break;
                }
                pollIndex++;
            }
            break;
    }
}

const char* M365BLE::getStateName() const {
    switch (state) {
        case BLEState::DISCONNECTED: return "DISCONNECTED";
        case BLEState::SCANNING:     return "SCANNING...";
        case BLEState::CONNECTING:   return "CONNECTING...";
        case BLEState::CONNECTED:    return "CONNECTED";
        case BLEState::AUTHENTICATED: return "READY";
        default: return "UNKNOWN";
    }
}
