/**
 * ESP32MC - Minecraft Java Server on ESP32
 * 协议版本 26.1.2 / 775
 *
 * 框架文件 (驱动层, 无需修改):
 *   mc_server.h / .cpp   - 服务器主体, 协议处理
 *   network_layer.h/.cpp - WiFi + TCP 网络层
 *   packet_codec.h/.cpp  - 协议编解码
 *   win/                 - Windows 调试构建
 */

#include "mc_server.h"

static const uint16_t MC_PORT = 25565;
static const char* WIFI_SSID = "YOUR_SSID";
static const char* WIFI_PASS = "YOUR_PASSWORD";

static MinecraftServer server(MC_PORT);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32MC starting...");

  if (!server.begin(WIFI_SSID, WIFI_PASS)) {
    Serial.println("WiFi begin failed");
  }
}

void loop() {
  server.poll();
  vTaskDelay(1);  // [驱动] 让 WiFi/lwIP 任务有机会处理 ACK
}
