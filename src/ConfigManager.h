#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#include <Arduino.h>

struct RadarConfig {
    float warningGain;
    int detectionDistance;
    int detectionSpeed;
    int dangerDistance;
    int dangerSpeed;
    bool ledBlinkMode;  // true: 闪烁模式, false: 常亮模式
    int normalBlinkInterval;
    int dangerBlinkInterval;
};

class ConfigManager {
private:
    RadarConfig config;
    const char *configFilePath = "/config.json";

    void setDefaultConfig();

public:
    bool loadConfig();

    bool saveConfig();

    bool updateConfig(const String &jsonString);

    const RadarConfig &getConfig() const;

    String getConfigJson() const;
};

#endif // CONFIG_MANAGER_H
