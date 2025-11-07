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

    // 全局最近一次目标
    bool hasLastTarget = false;
    RadarTarget lastTarget;

    bool parseRadarData();

    void processTargets(uint8_t targetCount, uint8_t *data);

    void triggerAudioWarning(bool left, bool right,bool isDanger);

    void triggerLightWarning(bool left, bool right, bool isDanger);

    void updateLightBehavior();
public:
    Radar(ConfigManager *configMgr);

    void begin();

    void warning();

    void testWarning(bool left, bool right);

    // 新增：普通/危险预警的通用测试触发（方向不区分，左右同时）
    void testGenericWarning(bool isDanger);
};

#endif // RADAR_PLAYER_H
