#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"
class Radar; // 前向声明

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0"
#endif
#ifndef BUILD_TIME
#define BUILD_TIME __DATE__ " " __TIME__
#endif

class WebServerManager {
private:
    AsyncWebServer server;
    ConfigManager* configManager;
    volatile bool shouldRestart;
    unsigned long rebootAtMillis;
    Radar* radar;
    
public:
    WebServerManager(ConfigManager* configMgr);
    void begin();
    void loop();
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void setRadar(Radar* r);
};

#endif