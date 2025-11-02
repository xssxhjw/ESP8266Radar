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

    bool parseRadarData();

    void processTargets(uint8_t targetCount, uint8_t *data);

    void triggerAudioWarning(bool left, bool right,bool isDanger);

    void triggerLightWarning(bool left, bool right, bool isDanger);

    void updateLightBehavior();
public:
    Radar(ConfigManager *configMgr);

    void begin();

    void warning();
};

#endif // RADAR_PLAYER_H
