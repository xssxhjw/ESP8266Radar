#include "Radar.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

Radar::Radar(ConfigManager *config) {
    configMgr = config;
    radarSerial = new SoftwareSerial(D5, D6);
    bufferIndex = 0;
    mp3 = nullptr;
    file = nullptr;
    out = nullptr;
    lastBlinkTime = 0;
    isLEDOn = false;
}

void Radar::begin() {
    Serial.println("开始初始化雷达系统...");
    delay(100);
    radarSerial->begin(115200);
    out = new AudioOutputI2S();
    mp3 = new AudioGeneratorMP3();
    delay(200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    delay(50);
    Serial.println("雷达系统初始化完成");
}

void Radar::ledBlink(const bool isDanger) {
    isLEDOn = true;
    digitalWrite(LED_PIN, HIGH);
    if (configMgr->getConfig().ledBlinkMode) {
        blinkInterval = isDanger ? configMgr->getConfig().dangerBlinkInterval : configMgr->getConfig().normalBlinkInterval;
        lastBlinkTime = millis();
    }
}

void Radar::ledWarning() {
    if (!isLEDOn || !configMgr->getConfig().ledBlinkMode) {
        return;
    }
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= blinkInterval) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastBlinkTime = currentTime;
    }
}

void Radar::audioWarning(String audio) {
    if (mp3 != nullptr && mp3->isRunning()) {
        mp3->stop();
    }
    if (file != nullptr) {
        file->close();
        delete file;
        file = nullptr;
    }
    file = new AudioFileSourceLittleFS(audio.c_str());
    out->SetGain(configMgr->getConfig().warningGain);
    mp3->begin(file, out);
}

void Radar::warningByDistanceAndSpeed(const uint8_t distance, const uint8_t speed) {
    String audioFile;
    bool isDanger = false;
    if (distance <= configMgr->getConfig().dangerDistance || speed >= configMgr->getConfig().dangerSpeed) {
        audioFile = "/danger.mp3";
        isDanger = true;
    } else {
        audioFile = "/normal.mp3";
    }
    if (mp3 != nullptr && mp3->isRunning()) {
        return;
    }
    ledBlink(isDanger);
    audioWarning(audioFile);
}

void Radar::processTargets(uint8_t targetCount, uint8_t *data) {
    for (int i = 0; i < targetCount; i++) {
        const bool approaching = data[i * 5 + 2] == 0x01;
        const uint8_t distance = data[i * 5 + 1];
        const uint8_t speed = data[i * 5 + 3];
        if (!approaching || distance <= 0 || distance > configMgr->getConfig().detectionDistance || speed <= 0 || speed < configMgr->getConfig().detectionSpeed || speed > 120) {
            continue;
        }
        warningByDistanceAndSpeed(distance, speed);
    }
}

bool Radar::parseRadarData() {
    // 检查缓冲区是否有足够的数据
    if (bufferIndex < 10) {
        return false;
    }
    // 查找帧头
    for (int i = 0; i <= bufferIndex - 10; i++) {
        if (buffer[i] == 0xF4 && buffer[i + 1] == 0xF3 && buffer[i + 2] == 0xF2 && buffer[i + 3] == 0xF1) {
            // 找到帧头，检查是否有完整的帧
            uint16_t dataLength = buffer[i + 4] | (buffer[i + 5] << 8);
            // 检查是否有完整的帧（帧头4字节 + 长度2字节 + 数据长度 + 帧尾4字节）
            if (i + 10 + dataLength <= bufferIndex) {
                // 检查帧尾
                int footerIndex = i + 6 + dataLength;
                if (buffer[footerIndex] == 0xF8 && buffer[footerIndex + 1] == 0xF7 &&
                    buffer[footerIndex + 2] == 0xF6 && buffer[footerIndex + 3] == 0xF5) {
                    // 帧完整，处理数据
                    uint8_t targetCount = buffer[i + 6]; // 目标数量
                    // uint8_t alarmInfo = buffer[i + 7]; // 报警信息 (未使用)
                    // 处理目标信息
                    if (targetCount > 0) {
                        processTargets(targetCount, &buffer[i + 8]);
                    }
                    // 移动缓冲区指针，丢弃已处理的数据
                    int processedBytes = footerIndex + 4 - i;
                    memmove(buffer, buffer + i + processedBytes, bufferIndex - i - processedBytes);
                    bufferIndex -= processedBytes;
                    return true;
                }
            }
        }
    }
    return false;
}

void Radar::warning() {
    if (mp3 != nullptr && mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
            delete file;
            file = nullptr;
            isLEDOn = false;
            digitalWrite(LED_PIN, LOW);
        }
    } else {
        isLEDOn = false;
        digitalWrite(LED_PIN, LOW);
    }
    // 读取雷达数据
    while (radarSerial->available()) {
        buffer[bufferIndex] = radarSerial->read();
        bufferIndex++;
        // 防止缓冲区溢出
        if ((unsigned int) bufferIndex >= sizeof(buffer)) {
            bufferIndex = 0;
        }
        // 尝试解析数据
        if (parseRadarData()) {
            // 数据解析成功，重置缓冲区
            bufferIndex = 0;
        }
    }
    ledWarning();
}
