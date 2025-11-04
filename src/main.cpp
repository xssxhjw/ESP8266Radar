#include <Arduino.h>
#include <OneButton.h>
#include <ESP8266WiFi.h>
#include "Radar.h"
#include "WebServerManager.h"

#define BTN_PIN 13

OneButton btn(BTN_PIN);

ConfigManager configMgr;

WebServerManager webServer(&configMgr);

Radar radar(&configMgr);

bool configMode = false;

static void startConfigMode() {
    configMode = true;
    // 启动 AP 并启动配置网页服务（在 WebServerManager::begin 内部完成）
    webServer.begin();
    // Serial.println("配置模式已开启：请连接 AP 'Radar'，访问 192.168.4.1");
}

static void stopConfigMode() {
    // Serial.println("配置模式已关闭，WiFi关闭以节省资源");
    // 关闭 AP / WiFi，释放无线资源
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    configMode = false;
}

void setup() {
    Serial.begin(115200);
    // 默认关闭 WiFi 以节省资源，只有进入配置模式时才开启
    WiFi.mode(WIFI_OFF);
    configMgr.loadConfig();
    webServer.setRadar(&radar);
    delay(500);
    radar.begin();
    btn.attachLongPressStart([] {
        ESP.restart();
    });
    btn.attachClick([] {
        if (!configMode) {
            startConfigMode();
        } else {
            stopConfigMode();
        }
    });
}

void loop() {
    radar.warning();
    webServer.loop();
    btn.tick();
    yield();
}
