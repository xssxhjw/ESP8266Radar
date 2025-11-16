#include "ConfigManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>


void ConfigManager::setDefaultConfig() {
    //Serial.println("加载配置文件失败，使用默认配置");
    config.warningGain = 2.5;
    config.detectionDistance = 45;
    config.detectionSpeed = 10;
    config.dangerDistance = 15;
    config.dangerSpeed = 25;
    config.lightBlink = true;
    config.normalBlinkInterval = 800;
    config.dangerBlinkInterval = 120;
    config.lightAngle = true;
    config.centerAngle = 5;
    config.blinkDuration = 2;
    config.audioEnabled = true;
    config.audioI2S = true;
    config.startAudio = true;
    config.logEnabled = false;

    // 默认实际时长（同时作为最大播放时长） normal 2s，其它 1s
    config.audioDurationMsNormal = 2000;
    config.audioDurationMsDanger = 1000;
    config.audioDurationMsLeft = 1000;
    config.audioDurationMsRight = 1000;
    config.audioDurationMsRear = 1000;
    config.audioDurationMsStart = 1000;
}

bool ConfigManager::loadConfig() {
    // Serial.println("开始加载配置文件");
    if (!LittleFS.begin()) {
        setDefaultConfig();
        return true;
    }
    File file = LittleFS.open(configFilePath, "r");
    if (!file) {
        setDefaultConfig();
        return true;
    }
    String jsonString = file.readString();
    file.close();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonString);
    config.detectionDistance = doc["detectionDistance"] | config.detectionDistance;
    config.detectionSpeed = doc["detectionSpeed"] | config.detectionSpeed;
    config.dangerDistance = doc["dangerDistance"] | config.dangerDistance;
    config.dangerSpeed = doc["dangerSpeed"] | config.dangerSpeed;
    config.warningGain = doc["warningGain"] | config.warningGain;
    config.lightBlink = doc["lightBlink"] | config.lightBlink;
    config.normalBlinkInterval = doc["normalBlinkInterval"] | config.normalBlinkInterval;
    config.dangerBlinkInterval = doc["dangerBlinkInterval"] | config.dangerBlinkInterval;
    config.lightAngle = doc["lightAngle"] | config.lightAngle;
    config.centerAngle = doc["centerAngle"] | config.centerAngle;
    config.blinkDuration = doc["blinkDuration"] | config.blinkDuration;
    config.audioEnabled = doc["audioEnabled"] | config.audioEnabled;
    config.audioI2S = doc["audioI2S"] | config.audioI2S;
    config.startAudio = doc["startAudio"] | config.startAudio;
    config.logEnabled = doc["logEnabled"] | config.logEnabled;

    // 读取实际时长（同时作为最大播放时长）
    config.audioDurationMsNormal = doc["audioDurationMsNormal"] | config.audioDurationMsNormal;
    config.audioDurationMsDanger = doc["audioDurationMsDanger"] | config.audioDurationMsDanger;
    config.audioDurationMsLeft = doc["audioDurationMsLeft"] | config.audioDurationMsLeft;
    config.audioDurationMsRight = doc["audioDurationMsRight"] | config.audioDurationMsRight;
    config.audioDurationMsRear = doc["audioDurationMsRear"] | config.audioDurationMsRear;
    config.audioDurationMsStart = doc["audioDurationMsStart"] | config.audioDurationMsStart;
    return true;
}

bool ConfigManager::saveConfig() {
    if (!LittleFS.begin()) {
        return false;
    }
    DynamicJsonDocument doc(1024);
    doc["detectionDistance"] = config.detectionDistance;
    doc["detectionSpeed"] = config.detectionSpeed;
    doc["dangerDistance"] = config.dangerDistance;
    doc["dangerSpeed"] = config.dangerSpeed;
    doc["warningGain"] = config.warningGain;
    doc["lightBlink"] = config.lightBlink;
    doc["normalBlinkInterval"] = config.normalBlinkInterval;
    doc["dangerBlinkInterval"] = config.dangerBlinkInterval;
    doc["lightAngle"] = config.lightAngle;
    doc["centerAngle"] = config.centerAngle;
    doc["blinkDuration"] = config.blinkDuration;
    doc["audioEnabled"] = config.audioEnabled;
    doc["audioI2S"] = config.audioI2S;
    doc["startAudio"] = config.startAudio;
    doc["logEnabled"] = config.logEnabled;

    // 实际时长（同时作为最大播放时长）
    doc["audioDurationMsNormal"] = config.audioDurationMsNormal;
    doc["audioDurationMsDanger"] = config.audioDurationMsDanger;
    doc["audioDurationMsLeft"] = config.audioDurationMsLeft;
    doc["audioDurationMsRight"] = config.audioDurationMsRight;
    doc["audioDurationMsRear"] = config.audioDurationMsRear;
    doc["audioDurationMsStart"] = config.audioDurationMsStart;
    File file = LittleFS.open(configFilePath, "w");
    if (!file) {
        return false;
    }
    serializeJson(doc, file);
    file.close();
    return true;
}

bool ConfigManager::updateConfig(const String &jsonString) {
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, jsonString);
    config.detectionDistance = doc["detectionDistance"] | config.detectionDistance;
    config.detectionSpeed = doc["detectionSpeed"] | config.detectionSpeed;
    config.dangerDistance = doc["dangerDistance"] | config.dangerDistance;
    config.dangerSpeed = doc["dangerSpeed"] | config.dangerSpeed;
    config.warningGain = doc["warningGain"] | config.warningGain;
    config.lightBlink = doc["lightBlink"] | config.lightBlink;
    config.blinkDuration = doc["blinkDuration"] | config.blinkDuration;
    config.normalBlinkInterval = doc["normalBlinkInterval"] | config.normalBlinkInterval;
    config.dangerBlinkInterval = doc["dangerBlinkInterval"] | config.dangerBlinkInterval;
    config.audioEnabled = doc["audioEnabled"] | config.audioEnabled;
    config.audioI2S = doc["audioI2S"] | config.audioI2S;
    config.lightAngle = doc["lightAngle"] | config.lightAngle;
    config.centerAngle = doc["centerAngle"] | config.centerAngle;
    config.startAudio = doc["startAudio"] | config.startAudio;
    config.logEnabled = doc["logEnabled"] | config.logEnabled;

    // 更新实际音频时长（同时作为最大播放时长）
    config.audioDurationMsNormal = doc["audioDurationMsNormal"] | config.audioDurationMsNormal;
    config.audioDurationMsDanger = doc["audioDurationMsDanger"] | config.audioDurationMsDanger;
    config.audioDurationMsLeft = doc["audioDurationMsLeft"] | config.audioDurationMsLeft;
    config.audioDurationMsRight = doc["audioDurationMsRight"] | config.audioDurationMsRight;
    config.audioDurationMsRear = doc["audioDurationMsRear"] | config.audioDurationMsRear;
    config.audioDurationMsStart = doc["audioDurationMsStart"] | config.audioDurationMsStart;
    return saveConfig();
}

const RadarConfig &ConfigManager::getConfig() const {
    return config;
}

String ConfigManager::getConfigJson() const {
    DynamicJsonDocument doc(1024);
    doc["warningGain"] = config.warningGain;
    doc["detectionDistance"] = config.detectionDistance;
    doc["detectionSpeed"] = config.detectionSpeed;
    doc["dangerDistance"] = config.dangerDistance;
    doc["dangerSpeed"] = config.dangerSpeed;
    doc["lightBlink"] = config.lightBlink;
    doc["normalBlinkInterval"] = config.normalBlinkInterval;
    doc["dangerBlinkInterval"] = config.dangerBlinkInterval;
    doc["lightAngle"] = config.lightAngle;
    doc["centerAngle"] = config.centerAngle;
    doc["blinkDuration"] = config.blinkDuration;
    doc["audioEnabled"] = config.audioEnabled;
    doc["audioI2S"] = config.audioI2S;
    doc["startAudio"] = config.startAudio;
    doc["logEnabled"] = config.logEnabled;

    // 实际时长（同时作为最大播放时长）
    doc["audioDurationMsNormal"] = config.audioDurationMsNormal;
    doc["audioDurationMsDanger"] = config.audioDurationMsDanger;
    doc["audioDurationMsLeft"] = config.audioDurationMsLeft;
    doc["audioDurationMsRight"] = config.audioDurationMsRight;
    doc["audioDurationMsRear"] = config.audioDurationMsRear;
    doc["audioDurationMsStart"] = config.audioDurationMsStart;
    String result;
    serializeJson(doc, result);
    return result;
}
