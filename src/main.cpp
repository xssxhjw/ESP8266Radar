#include <Arduino.h>
#include <OneButton.h>
#include "Radar.h"
#include "WebServerManager.h"

#define BTN_PIN 13

OneButton btn(BTN_PIN);

ConfigManager configMgr;

WebServerManager webServer(&configMgr);

Radar radar(&configMgr);

bool configMode = false;

void setup() {
    Serial.begin(115200);
    configMgr.loadConfig();
    webServer.setRadar(&radar);
    webServer.begin();
    delay(500);
    radar.begin();
    btn.attachLongPressStart([] {
        ESP.restart();
    });
    btn.attachMultiClick([] {

    });
}

void loop() {
    radar.warning();
    webServer.loop();
    btn.tick();
}
