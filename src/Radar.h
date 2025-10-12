#ifndef RADAR_PLAYER_H
#define RADAR_PLAYER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include "ConfigManager.h"

#define LED_PIN D1

class Radar {
private:
    SoftwareSerial *radarSerial;
    ConfigManager *configMgr;
    AudioGeneratorMP3 *mp3;
    AudioFileSourceLittleFS *file;
    AudioOutputI2S *out;
    uint8_t buffer[128];
    int bufferIndex;
    unsigned long lastBlinkTime;
    unsigned long blinkInterval;
    unsigned long blinkStartTime;
    bool isLEDOn = false;

    bool parseRadarData();

    void processTargets(uint8_t targetCount, uint8_t *data);

    void warningByDistanceAndSpeed(uint8_t distance, uint8_t speed);

    void ledWarning();

    void ledBlink(bool isDanger);

    void audioWarning(String audio);
public:
    Radar(ConfigManager *configMgr);

    void begin();

    void warning();
};

#endif // RADAR_PLAYER_H
