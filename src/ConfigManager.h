#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#include <Arduino.h>

struct RadarConfig {
    float warningGain;
    int detectionDistance;
    int detectionSpeed;
    int dangerDistance;
    int dangerSpeed;
    bool lightBlink;  // true: 闪烁模式, false: 常亮模式
    int blinkDuration;  // 秒，闪烁时长
    int normalBlinkInterval;
    int dangerBlinkInterval;
    bool lightAngle;    // true: 指示左右，false: 左右同亮
    int centerAngle;
    bool audioEnabled;
    bool audioI2S;
    bool startAudio;    // 是否播放启动音效
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
