#include "WebServerManager.h"
#include <ESP8266WiFi.h>
#include <LittleFS.h>

WebServerManager::WebServerManager(ConfigManager *configMgr) : server(80), configManager(configMgr) {
}


void WebServerManager::handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data,
                                        size_t len, bool final) {
    static File uploadFile;
    if (!index) {
        String path = "/" + filename;
        uploadFile = LittleFS.open(path, "w");
        if (!uploadFile) {
            request->send(500, "text/plain; charset=utf-8", "文件创建失败");
            return;
        }
    }
    if (uploadFile) {
        uploadFile.write(data, len);
    }
    if (final) {
        if (uploadFile) {
            uploadFile.close();
        }
        request->send(200, "text/plain; charset=utf-8", "文件上传完成");
    }
}

void initWiFi() {
    Serial.println("开始初始化WiFi...");
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("Radar");
}


void WebServerManager::begin() {
    initWiFi();
    Serial.println("开始初始化WebServer...");
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String config = configManager->getConfigJson();
        request->send(200, "application/json; charset=utf-8", config);
    });

    server.on("/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
                  request->send(400, "text/plain; charset=utf-8", "请使用正确的请求格式");
              }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                  static String body = "";
                  if (index == 0) {
                      body = "";
                  }
                  for (size_t i = 0; i < len; i++) {
                      body += (char) data[i];
                  }
                  if (index + len == total) {
                      // 数据接收完成，保存配置
                      if (configManager->updateConfig(body)) {
                          request->send(200, "text/plain; charset=utf-8", "配置保存成功");
                      } else {
                          request->send(500, "text/plain; charset=utf-8", "配置保存失败");
                      }
                  }
              });


    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
                  request->send(200, "text/plain; charset=utf-8", "上传完成");
              }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
                        bool final) {
                  handleFileUpload(request, filename, index, data, len, final);
              });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain; charset=utf-8", "页面未找到");
    });

    server.begin();
}
