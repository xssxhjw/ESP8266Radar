#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"

class WebServerManager {
private:
    AsyncWebServer server;
    ConfigManager* configManager;
    
public:
    WebServerManager(ConfigManager* configMgr);
    void begin();
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};

#endif