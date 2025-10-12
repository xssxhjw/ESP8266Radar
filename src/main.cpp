#include <Arduino.h>
#include "Radar.h"
#include "WebServerManager.h"

ConfigManager configMgr;

WebServerManager webServer(&configMgr);

Radar radar(&configMgr);

void setup() {
    Serial.begin(115200);
    configMgr.loadConfig();
    webServer.begin();
    delay(500);
    radar.begin();
}

void loop() {
    radar.warning();
    yield();
}
