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
}

void Radar::begin() {
    Serial.println("开始初始化雷达系统...");
    delay(100);
    radarSerial->begin(115200);
    const auto &cfg = configMgr->getConfig();
    if (cfg.audioEnabled) {
        if (cfg.audioI2S) {
            out = new AudioOutputI2S();
        } else {
            out = new AudioOutputI2SNoDAC();
        }
    }
    delay(200);
    pinMode(LEFT_LIGHT_PIN, OUTPUT);
    digitalWrite(LEFT_LIGHT_PIN, LOW);
    delay(50);
    Serial.println("雷达系统初始化完成");
}

void Radar::triggerLightWarning(bool left, bool right, bool isDanger) {
    const auto &cfg = configMgr->getConfig();
    if (cfg.lightBlink) {
        blinkInterval = (unsigned long) (isDanger ? cfg.dangerBlinkInterval : cfg.normalBlinkInterval);
    }
    const unsigned long now = millis();
    if (left) {
        leftLightOn = true;
        leftLightOnTime = now;
        leftLightLastBlinkTime = now;
        digitalWrite(LEFT_LIGHT_PIN, HIGH);
    }
    if (right) {
        rightLightOn = true;
        rightLightOnTime = now;
        rightLightLastBlinkTime = now;
        digitalWrite(RIGHT_LIGHT_PIN, HIGH);
    }
}

void Radar::triggerAudioWarning(bool left, bool right, bool isDanger) {
    if (mp3 == nullptr) {
        mp3 = new AudioGeneratorMP3();
    }
    if (mp3->isRunning()) {
        return;
    }
    triggerLightWarning(left, right, isDanger);
    String audio = isDanger ? "/danger.mp3" : "/normal.mp3";
    if (file != nullptr) {
        file->close();
        delete file;
        file = nullptr;
    }
    file = new AudioFileSourceLittleFS(audio.c_str());
    out->SetGain(configMgr->getConfig().warningGain);
    mp3->begin(file, out);
}

void Radar::processTargets(uint8_t targetCount, uint8_t *data) {
    const auto &cfg = configMgr->getConfig();
    for (int i = 0; i < targetCount; i++) {
        const bool approaching = data[i * 5 + 2] == 0x01;
        const uint8_t distance = data[i * 5 + 1];
        const uint8_t speed = data[i * 5 + 3];
        const int8_t angle = data[i * 5] - 0x80; // 探测角度 水平±15°
        if (!approaching || distance <= 0 || distance > configMgr->getConfig().detectionDistance || speed <= 0 || speed
            < configMgr->getConfig().detectionSpeed || speed > 120 || angle <= -15 || angle >= 15) {
            continue;
        }
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.print(", Speed: ");
        Serial.print(speed);
        Serial.print(", Angle: ");
        Serial.print(angle);
        Serial.println("");
        const bool isDanger = (distance <= cfg.dangerDistance) || (speed >= cfg.dangerSpeed);
        // 根据角度区分方向：左后方、右后方、正后方
        const int8_t centerAngle = (int8_t) cfg.centerAngle; // 中心阈值（±centerAngle° 视为正后方）
        bool left = false;
        bool right = false;
        if (!cfg.lightAngle) {
            // 左右同亮模式，不区分方向
            left = true;
            right = true;
        } else {
            if (angle <= -centerAngle) {
                left = true;
            } else if (angle >= centerAngle) {
                right = true;
            } else {
                // 正后方同时预警左右灯
                left = true;
                right = true;
            }
        }
        if (cfg.audioEnabled) {
            triggerAudioWarning(left, right, isDanger);
        } else {
            triggerLightWarning(left, right, isDanger);
        }
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

void Radar::updateLightBehavior() {
    if (!leftLightOn && !rightLightOn) {
        return;
    }
    const auto &cfg = configMgr->getConfig();
    const unsigned long durationMs = (unsigned long) cfg.blinkDuration * 1000UL;
    const unsigned long now = millis();
    if (!cfg.audioEnabled) {
        // 达到持续时长后，结束预警并熄灭
        if (now - leftLightOnTime >= durationMs) {
            leftLightOn = false;
            digitalWrite(LEFT_LIGHT_PIN, LOW);;
        }
        if (now - rightLightOnTime >= durationMs) {
            rightLightOn = false;
            digitalWrite(RIGHT_LIGHT_PIN, LOW);
        }
    }
    if (!leftLightOn && !rightLightOn) {
        return;
    }
    // 闪烁模式的周期切换
    if (cfg.lightBlink) {
        if (leftLightOn && (now - leftLightLastBlinkTime >= blinkInterval)) {
            digitalWrite(LEFT_LIGHT_PIN, !digitalRead(LEFT_LIGHT_PIN));
            leftLightLastBlinkTime = now;
        }
        if (rightLightOn && (now - rightLightLastBlinkTime >= blinkInterval)) {
            digitalWrite(RIGHT_LIGHT_PIN, !digitalRead(RIGHT_LIGHT_PIN));
            rightLightLastBlinkTime = now;
        }
    }
}

void Radar::warning() {
    if (configMgr->getConfig().audioEnabled) {
        if (mp3 != nullptr && mp3->isRunning() && !mp3->loop()) {
            mp3->stop();
            file = nullptr;
            leftLightOn = false;
            rightLightOn = false;
            digitalWrite(LEFT_LIGHT_PIN, LOW);
            digitalWrite(RIGHT_LIGHT_PIN, LOW);
        }
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
    updateLightBehavior();
}
