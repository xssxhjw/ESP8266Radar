#include "Radar.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
static const size_t RADAR_LOG_MAX_SIZE = 32768; // 32KB 最大日志大小

Radar::Radar(ConfigManager *config) {
    configMgr = config;
    radarSerial = new SoftwareSerial(D5, D6);
    bufferIndex = 0;
    mp3 = nullptr;
    file = nullptr;
    out = nullptr;
}

void Radar::begin() {
    // Serial.println("开始初始化雷达系统...");
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
    leftLightPinState = false; // 初始化左灯状态变量
    pinMode(RIGHT_LIGHT_PIN, OUTPUT);
    digitalWrite(RIGHT_LIGHT_PIN, LOW);
    rightLightPinState = false; // 初始化右灯状态变量
    delay(50);
    // 根据配置决定是否播放启动音效
    if (cfg.audioEnabled && cfg.startAudio) {
        playAudio("/start.mp3");
    }
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
        leftLightPinState = true;
        digitalWrite(LEFT_LIGHT_PIN, HIGH);
    }
    if (right) {
        rightLightOn = true;
        rightLightOnTime = now;
        rightLightLastBlinkTime = now;
        rightLightPinState = true;
        digitalWrite(RIGHT_LIGHT_PIN, HIGH);
    }
}

void Radar::playAudio(String audio) {
    const auto &cfg = configMgr->getConfig();
    if (!cfg.audioEnabled) {
        return;
    }
    if (mp3 == nullptr) {
        mp3 = new AudioGeneratorMP3();
    }
    if (mp3->isRunning()) {
        return;
    }
    file = new AudioFileSourceLittleFS(audio.c_str());
    out->SetGain(cfg.warningGain);
    mp3->begin(file, out);
}


void Radar::triggerAudioWarning(bool left, bool right, bool isDanger) {
    if (mp3 == nullptr) {
        mp3 = new AudioGeneratorMP3();
    }
    if (mp3->isRunning()) {
        return;
    }
    triggerLightWarning(left, right, isDanger);
    const auto &cfg = configMgr->getConfig();
    String path;
    if (isDanger) {
        path = "/danger.mp3";
    } else {
        if (cfg.lightAngle) {
            if (left && right) {
                path = "/rear.mp3"; // 正后方来车
            } else if (left) {
                path = "/left.mp3"; // 左后方来车
            } else if (right) {
                path = "/right.mp3"; // 右后方来车
            }
        } else {
            path = "/normal.mp3";
        }
    }
    playAudio(path);
}

void Radar::testFunction(String function) {
    String path;
    if (function == "left") {
        path = "/left.mp3";
        triggerLightWarning(true, false, true);
        playAudio(path);
    } else if (function == "right") {
        path = "/right.mp3";
        triggerLightWarning(false, true, true);
        playAudio(path);
    } else if (function == "rear") {
        path = "/rear.mp3";
        triggerLightWarning(true, true, true);
        playAudio(path);
    } else if (function == "isDanger") {
        path = "/danger.mp3";
        triggerLightWarning(true, true, true);
        playAudio(path);
    } else if (function == "normal") {
        path = "/normal.mp3";
        triggerLightWarning(true, true, false);
        playAudio(path);
    } else if (function == "start") {
        path = "/start.mp3";
        playAudio(path);
    }
}

void Radar::processTargets(uint8_t targetCount, uint8_t *data) {
    const auto &cfg = configMgr->getConfig();
    bool hasPreTarget = false;
    RadarTarget preTarget;
    const unsigned long now = millis();
    for (int i = 0; i < targetCount; i++) {
        RadarTarget target = {
            data[i * 5 + 2] == 0x01,
            data[i * 5 + 1],
            data[i * 5 + 3],
            (int8_t) (data[i * 5] - 0x80),
            now
        };
        //忽略不符目标
        if (!target.approaching || target.distance <= 0 || target.distance > cfg.detectionDistance || target.speed <= 0
            || target.speed < cfg.detectionSpeed || target.speed > 120 || target.angle <= -12 || target.angle >= 12) {
            continue;
        }
        // 与前一个目标比较，如果完全一致则忽略
        if (targetCount > 1 && hasPreTarget && target.distance == preTarget.distance && target.speed == preTarget.speed
            && target.angle == preTarget.angle) {
            if (cfg.logEnabled) {
                writeLog(String(millis()) + " [target] 与前一目标重复，忽略: dist=" + target.distance +
                         ", speed=" + target.speed + ", angle=" + (int) target.angle + ", ts=" + target.timestamp);
            }
            continue;
        }
        if (hasLastTarget && ((lastTarget.distance == target.distance && lastTarget.speed == target.speed && lastTarget.
                               angle == target.angle) || (now - lastTarget.timestamp) < 1000UL)) {
            if (cfg.logEnabled) {
                writeLog(String(millis()) + " [target] 与最近目标重复或时间过近，忽略: dist=" + target.distance +
                         ", speed=" + target.speed + ", angle=" + (int) target.angle + ", ts=" + target.timestamp);
            }
            continue;
        }
        hasPreTarget = true;
        preTarget = target;
        hasLastTarget = true;
        lastTarget = target;
        const bool isDanger = (target.distance <= cfg.dangerDistance) || (target.speed >= cfg.dangerSpeed);
        // 根据角度区分方向：左后方、右后方、正后方
        const int8_t centerAngle = (int8_t) cfg.centerAngle; // 中心阈值（±centerAngle° 视为正后方）
        bool left = false;
        bool right = false;
        if (!cfg.lightAngle) {// 左右同亮模式，不区分方向
            left = true;
            right = true;
        } else {
            if (target.angle <= -centerAngle) {
                left = true;
            } else if (target.angle >= centerAngle) {
                right = true;
            } else {// 正后方同时预警左右灯
                left = true;
                right = true;
            }
        }
        if (cfg.audioEnabled) {
            triggerAudioWarning(left, right, isDanger);
            if (cfg.logEnabled) {
                writeLog(
                    String(millis()) + " [warn] 触发预警: left=" + (left ? "1" : "0") + ", right=" + (right ? "1" : "0") +
                    ", danger=" + (isDanger ? "1" : "0") + ", angle=" + (int) target.angle + ", dist=" + target.distance
                    + ", speed=" + target.speed + ", ts=" + target.timestamp);
            }
        } else {
            if (cfg.logEnabled) {
                writeLog(
                    String(millis()) + " [light] 仅灯光预警: left=" + (left ? "1" : "0") + ", right=" + (right ? "1" : "0") +
                    ", danger=" + (isDanger ? "1" : "0") + ", ts=" + target.timestamp);
            }
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
            leftLightPinState = false;
            digitalWrite(LEFT_LIGHT_PIN, LOW);
        }
        if (now - rightLightOnTime >= durationMs) {
            rightLightOn = false;
            rightLightPinState = false;
            digitalWrite(RIGHT_LIGHT_PIN, LOW);
        }
    }
    if (!leftLightOn && !rightLightOn) {
        return;
    }
    // 闪烁模式的周期切换
    if (cfg.lightBlink) {
        if (leftLightOn && (now - leftLightLastBlinkTime >= blinkInterval)) {
            leftLightPinState = !leftLightPinState; // 使用状态变量翻转
            digitalWrite(LEFT_LIGHT_PIN, leftLightPinState ? HIGH : LOW);
            leftLightLastBlinkTime = now;
            if (cfg.logEnabled) {
                writeLog(String(millis()) + " [blink] 左灯切换 -> " + (leftLightPinState ? "HIGH" : "LOW") +
                         "，目标 " + (hasLastTarget
                                       ? (String("dist=") + lastTarget.distance + "，speed=" + lastTarget.speed +
                                          "，angle=" + (int) lastTarget.angle + "，ts=" + lastTarget.timestamp)
                                       : String("none")));
            }
        }
        if (rightLightOn && (now - rightLightLastBlinkTime >= blinkInterval)) {
            rightLightPinState = !rightLightPinState; // 使用状态变量翻转
            digitalWrite(RIGHT_LIGHT_PIN, rightLightPinState ? HIGH : LOW);
            rightLightLastBlinkTime = now;
            if (cfg.logEnabled) {
                writeLog(String(millis()) + " [blink] 右灯切换 -> " + (rightLightPinState ? "HIGH" : "LOW") +
                         "，目标 " + (hasLastTarget
                                       ? (String("dist=") + lastTarget.distance + "，speed=" + lastTarget.speed +
                                          "，angle=" + (int) lastTarget.angle + "，ts=" + lastTarget.timestamp)
                                       : String("none")));
            }
        }
    }
}

void Radar::warning() {
    if (configMgr->getConfig().audioEnabled) {
        if (mp3 != nullptr && mp3->isRunning() && !mp3->loop()) {
            mp3->stop();
            if (file != nullptr) {
                file->close();
                delete file;
            }
            file = nullptr;
            leftLightOn = false;
            rightLightOn = false;
            leftLightPinState = false;
            rightLightPinState = false;
            digitalWrite(LEFT_LIGHT_PIN, LOW);
            digitalWrite(RIGHT_LIGHT_PIN, LOW);
            if (configMgr->getConfig().logEnabled) {
                unsigned long __now = millis();
                writeLog(String(__now) + " [audio] 结束播放，灯光复位，目标 " +
                         (hasLastTarget
                              ? (String("dist=") + lastTarget.distance + "，speed=" + lastTarget.speed +
                                 "，angle=" + (int) lastTarget.angle + "，ts=" + lastTarget.timestamp)
                              : String("none")));
            }
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
            yield();
        }
    }
    updateLightBehavior();
}

void Radar::writeLog(const String &line) {
    const auto &cfg = configMgr->getConfig();
    if (!cfg.logEnabled) return;
    if (!LittleFS.begin()) return;
    // 检查大小，超过上限则旋转
    if (LittleFS.exists("/radar.log")) {
        File rf = LittleFS.open("/radar.log", "r");
        if (rf) {
            size_t sz = rf.size();
            rf.close();
            if (sz >= RADAR_LOG_MAX_SIZE) {
                LittleFS.remove("/radar.log");
                File nf = LittleFS.open("/radar.log", "w");
                if (nf) {
                    nf.println("=== LOG ROTATED ===");
                    nf.close();
                }
            }
        }
    }
    File f = LittleFS.open("/radar.log", "a");
    if (!f) {
        f = LittleFS.open("/radar.log", "w");
    }
    if (f) {
        f.println(line);
        f.close();
    }
}
