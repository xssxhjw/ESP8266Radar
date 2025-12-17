#ifndef RADAR_PLAYER_H
#define RADAR_PLAYER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioOutputI2SNoDAC.h>
#include "ConfigManager.h"

#define LEFT_LIGHT_PIN D1
#define RIGHT_LIGHT_PIN D2
#define REAR_LIGHT_PIN D0

struct RadarTarget {
    bool approaching;
    uint8_t distance;
    uint8_t speed;
    int8_t angle;
    unsigned long timestamp;
};

class Radar {
private:
    SoftwareSerial *radarSerial;
    ConfigManager *configMgr;
    AudioGeneratorMP3 *mp3;
    AudioFileSourceLittleFS *file;
    AudioOutput *out;
    uint8_t buffer[128];
    int bufferIndex;

    unsigned long leftLightLastBlinkTime;
    unsigned long rightLightLastBlinkTime;
    unsigned long blinkInterval;
    unsigned long leftLightOnTime;
    unsigned long rightLightOnTime;
    bool leftLightOn = false;
    bool rightLightOn = false;
    
    // 状态变量记录当前输出电平，避免使用 digitalRead
    bool leftLightPinState = false;
    bool rightLightPinState = false;

    // 音频播放开始时间，用于最大时长兜底
    unsigned long audioStartTime = 0;
    // 当前音频允许的最大播放时长（毫秒），根据文件名和配置决定
    unsigned long currentMaxAudioMs = 0;

    // 全局最近一次目标
    bool hasLastTarget = false;
    RadarTarget lastTarget;

    bool parseRadarData();

    void processTargets(uint8_t targetCount, uint8_t *data);

    void triggerAudioWarning(bool left, bool right,bool isDanger);

    void triggerLightWarning(bool left, bool right, bool isDanger);

    void updateLightBehavior();

    void writeLog(const String &line);

    void stopAudioAndResetLights();

    unsigned long getMaxAudioMsForPath(const String &path);
public:
    Radar(ConfigManager *configMgr);

    void begin();

    void warning();

    void testFunction(String function);

    bool playAudio(String audio);
};

#endif // RADAR_PLAYER_H
