#include "ConfigManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>


void ConfigManager::setDefaultConfig() {
    Serial.println("加载配置文件失败，使用默认配置");
    config.warningGain = 3.0;
    config.detectionDistance = 100;
    config.detectionSpeed = 5;
    config.dangerDistance = 50;
    config.dangerSpeed = 25;
    config.ledBlinkMode = true;
    config.normalBlinkInterval = 800;
    config.dangerBlinkInterval = 120;
}

bool ConfigManager::loadConfig() {
    Serial.println();
    Serial.println("开始加载配置文件");
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
    config.ledBlinkMode = doc["ledBlinkMode"] | config.ledBlinkMode;
    config.normalBlinkInterval = doc["normalBlinkInterval"] | config.normalBlinkInterval;
    config.dangerBlinkInterval = doc["dangerBlinkInterval"] | config.dangerBlinkInterval;
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
    doc["ledBlinkMode"] = config.ledBlinkMode;
    doc["normalBlinkInterval"] = config.normalBlinkInterval;
    doc["dangerBlinkInterval"] = config.dangerBlinkInterval;
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
    config.ledBlinkMode = doc["ledBlinkMode"] | config.ledBlinkMode;
    config.normalBlinkInterval = doc["normalBlinkInterval"] | config.normalBlinkInterval;
    config.dangerBlinkInterval = doc["dangerBlinkInterval"] | config.dangerBlinkInterval;
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
    doc["ledBlinkMode"] = config.ledBlinkMode;
    doc["normalBlinkInterval"] = config.normalBlinkInterval;
    doc["dangerBlinkInterval"] = config.dangerBlinkInterval;
    String result;
    serializeJson(doc, result);
    return result;
}
