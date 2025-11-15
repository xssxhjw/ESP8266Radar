#include "WebServerManager.h"
#include "Radar.h"
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Updater.h>
#include <ArduinoJson.h>

WebServerManager::WebServerManager(ConfigManager *configMgr) : server(80), configManager(configMgr) {
    shouldRestart = false;
    rebootAtMillis = 0;
    radar = nullptr;
}


void WebServerManager::handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data,size_t len, bool final) {
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
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("Radar");
}


void WebServerManager::begin() {
    initWiFi();
    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = String("{") +
                      "\"version\":\"" + String(FIRMWARE_VERSION) + "\"," +
                      "\"buildTime\":\"" + String(BUILD_TIME) + "\"" +
                      "}";
        request->send(200, "application/json; charset=utf-8", json);
    });

    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String config = configManager->getConfigJson();
        request->send(200, "application/json; charset=utf-8", config);
    });

    // 日志查看与下载
    server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/radar.log")) {
            request->send(LittleFS, "/radar.log", "text/plain; charset=utf-8");
        } else {
            request->send(404, "text/plain; charset=utf-8", "日志不存在");
        }
    });
    server.on("/logs/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/radar.log")) {
            request->send(LittleFS, "/radar.log", "application/octet-stream", true);
        } else {
            request->send(404, "text/plain; charset=utf-8", "日志不存在");
        }
    });
    server.on("/logs/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!LittleFS.begin()) {
            request->send(500, "text/plain; charset=utf-8", "文件系统不可用");
            return;
        }
        if (LittleFS.exists("/radar.log")) {
            LittleFS.remove("/radar.log");
            request->send(200, "text/plain; charset=utf-8", "已清空日志");
        } else {
            request->send(200, "text/plain; charset=utf-8", "日志不存在");
        }
    });

    server.on("/testFunction", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (radar) {
            String function = "normal";
            if (request->hasParam("function")) {
                function = request->getParam("function")->value();
            }
            if (function == "reboot") {
                rebootAtMillis = millis() + 500;
                shouldRestart = true;
            } else {
                radar->testFunction(function);
            }
            request->send(200, "text/plain; charset=utf-8", "已触发功能");
        } else {
            request->send(500, "text/plain; charset=utf-8", "Radar 未初始化");
        }
    });

    server.on("/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
                  request->send(400, "text/plain; charset=utf-8", "请使用正确的请求格式");
              }, nullptr, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                  static String body = "";
                  if (index == 0) {
                      body = "";
                  }
                  for (size_t i = 0; i < len; i++) {
                      body += (char) data[i];
                  }
                  if (index + len == total) {
                      bool needReboot = false;
                      StaticJsonDocument<512> doc;
                      DeserializationError err = deserializeJson(doc, body);
                      if (!err) {
                          const auto &oldCfg = configManager->getConfig();
                          bool newAudioEnabled = doc["audioEnabled"].isNull()? oldCfg.audioEnabled: (bool) doc["audioEnabled"];
                          bool newAudioI2S = doc["audioI2S"].isNull() ? oldCfg.audioI2S : (bool) doc["audioI2S"];
                          needReboot = (newAudioEnabled != oldCfg.audioEnabled) || (newAudioI2S != oldCfg.audioI2S);
                      }
                      if (configManager->updateConfig(body)) {
                          if (needReboot) {
                              AsyncWebServerResponse *resp = request->beginResponse(200, "text/plain; charset=utf-8", "配置保存成功，设备将重启");
                              resp->addHeader("Connection", "close");
                              request->send(resp);
                              rebootAtMillis = millis() + 500;
                              shouldRestart = true;
                          } else {
                              request->send(200, "text/plain; charset=utf-8", "配置保存成功");
                          }
                      } else {
                          request->send(500, "text/plain; charset=utf-8", "配置保存失败");
                      }
                  }
              });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
                  request->send(200, "text/plain; charset=utf-8", "上传完成");
              }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,bool final) {
                  handleFileUpload(request, filename, index, data, len, final);
              });

    server.on("/update", HTTP_POST,
              [this](AsyncWebServerRequest *request) {
                  request->send(400, "text/plain; charset=utf-8", "请以二进制上传固件或文件系统镜像");
              }, nullptr,[this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                  static bool updateBegun = false;
                  static bool updateError = false;
                  static uint8_t updateTarget = U_FLASH; // 默认更新程序固件
                  if (index == 0) {
                      // Serial.printf("开始接收更新包，总大小: %u 字节\n", (unsigned) total);
                      Update.runAsync(true);
                      // 根据请求参数选择更新目标：程序(U_FLASH) 或 文件系统(U_FS)
                      String type = "flash";
                      if (request->hasParam("firmwareType")) {
                          type = request->getParam("firmwareType")->value();
                      } else {
                          if (total > 500000) {
                              type = "fs";
                          }
                      }
                      type.toLowerCase();
                      if (type == "fs") {
                          updateTarget = U_FS;
                      } else {
                          updateTarget = U_FLASH;
                      }
                      updateBegun = Update.begin(total, updateTarget);
                      updateError = !updateBegun;
                  }
                  if (!updateError) {
                      size_t written = Update.write(data, len);
                      if (written != len) {
                          updateError = true;
                          // Serial.printf("Update.write 失败，写入 %u/%u 字节\n", (unsigned) written, (unsigned) len);
                      }
                  }
                  if (index + len == total) {
                      if (!updateError && Update.end(true)) {
                          // Serial.println("更新成功，即将重启...");
                          AsyncWebServerResponse *resp = request->beginResponse(200,"text/plain; charset=utf-8", "更新成功，设备将重启");
                          resp->addHeader("Connection", "close");
                          request->send(resp);
                          // 在回调外延迟重启，避免在 SYS 上下文中 delay/restart 触发断言
                          rebootAtMillis = millis() + 500;
                          shouldRestart = true;
                      } else {
                          // Serial.printf("更新失败，错误码: %d\n", (int) Update.getError());
                          Update.end(false);
                          request->send(500, "text/plain; charset=utf-8", "更新失败，错误码:" + Update.getError());
                      }
                  }
              });
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain; charset=utf-8", "页面未找到");
    });
    server.begin();
}

void WebServerManager::loop() {
    if (shouldRestart && millis() >= rebootAtMillis) {
        shouldRestart = false;
        ESP.restart();
    }
}

void WebServerManager::setRadar(Radar *r) {
    radar = r;
}
