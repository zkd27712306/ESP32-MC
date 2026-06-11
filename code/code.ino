/**
 * ESP32MC - Minecraft Java Server on ESP32
 * 协议版本 26.1.2 / 775
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include "mc_server.h"
#include "wifi_config.h"
#include "packet_codec.h"

static const uint16_t MC_PORT = 25565;
static const uint8_t kWebCfgButtonPin = 0;      // IO0 / BOOT 键
static const uint8_t kLedPin = 9;               // IO9 活动灯

// ---- 简易 LED 驱动 ----
static bool s_led_enabled = false;
static uint32_t s_led_off_at_ms = 0;
static const uint32_t kLedPulseMs = 35;

static void ledBegin() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, HIGH);  // 熄灭（高电平灭）
}

static void ledSetEnabled(bool en) {
  s_led_enabled = en;
  if (!en) {
    s_led_off_at_ms = 0;
    digitalWrite(kLedPin, HIGH);  // 熄灭
  }
}

// 在 loop 里调用，处理脉冲超时熄灭
static void ledTick() {
  if (s_led_off_at_ms && (int32_t)(millis() - s_led_off_at_ms) >= 0) {
    s_led_off_at_ms = 0;
    if (s_led_enabled) digitalWrite(kLedPin, HIGH);  // 熄灭
  }
}

// 发一次短脉冲（有数据收发时调用）
static void ledPulse() {
  if (!s_led_enabled) return;
  digitalWrite(kLedPin, LOW);  // 点亮
  s_led_off_at_ms = millis() + kLedPulseMs;
}
// ---- LED 驱动结束 ----

static MinecraftServer server(MC_PORT);
static bool server_started = false;

static const char *resetReasonString(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN:   return "unknown";
    case ESP_RST_POWERON:   return "power_on";
    case ESP_RST_EXT:       return "external";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "int_wdt";
    case ESP_RST_TASK_WDT:  return "task_wdt";
    case ESP_RST_WDT:       return "other_wdt";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_SDIO:      return "sdio";
    default:                return "unmapped";
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi connected, IP: ");
      Serial.println(WiFi.localIP());
      ledSetEnabled(true);
      if (!server_started) {
        char ssid[33] = {}, pass[65] = {};
        wifiCfgLoadBootCredentials(ssid, sizeof(ssid), pass, sizeof(pass));
        server.begin(ssid, pass);
        server_started = true;
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected...");
      ledSetEnabled(false);
      // 不立即 reconnect，交给 network_layer.poll() 的重试逻辑处理
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.println("\nESP32MC starting...");
  Serial.printf("Reset reason: %d (%s)\n", (int)reset_reason, resetReasonString(reset_reason));
  Serial.printf("Free heap on boot: %u bytes\n", ESP.getFreeHeap());

  ledBegin();

  // 有数据包收发时触发 LED 脉冲
  g_packet_activity_cb = []() { ledPulse(); };

  wifiCfgInit(kWebCfgButtonPin, 3000, 180000);

  char wifi_ssid[33] = {};
  char wifi_pass[65] = {};
  wifiCfgLoadBootCredentials(wifi_ssid, sizeof(wifi_ssid), wifi_pass, sizeof(wifi_pass));

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);

  if (wifi_ssid[0]) {
    Serial.printf("Connecting to saved WiFi: %s\n", wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_pass);
  } else {
    Serial.println("WiFi not configured. Hold BOOT to enter config mode.");
  }
}

void loop() {
  wifiCfgPoll();
  ledTick();
  if (server_started) {
    server.poll();
  }
  vTaskDelay(1);
}
