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
}

bool ConfigManager::loadConfig() {
    // Serial.println();
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
    DynamicJsonDocument doc(512);
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
    return true;
}

bool ConfigManager::saveConfig() {
    if (!LittleFS.begin()) {
        return false;
    }
    DynamicJsonDocument doc(512);
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
    File file = LittleFS.open(configFilePath, "w");
    if (!file) {
        return false;
    }
    serializeJson(doc, file);
    file.close();
    return true;
}

bool ConfigManager::updateConfig(const String &jsonString) {
    StaticJsonDocument<512> doc;
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
    return saveConfig();
}

const RadarConfig &ConfigManager::getConfig() const {
    return config;
}

String ConfigManager::getConfigJson() const {
    DynamicJsonDocument doc(512);
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
    String result;
    serializeJson(doc, result);
    return result;
}
