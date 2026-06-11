/**
 * ESP32MC - Episode 1
 * WiFi 联网 + Minecraft Status 查询
 * 按 BOOT 键 (GPIO0) 切换服务器状态
 */

#include "mc_server.h"

static const uint16_t MC_PORT   = 25565;
static const char* WIFI_SSID    = "QwenR";
static const char* WIFI_PASS    = "13792468";
static const int   BUTTON_PIN   = 0;  // ESP32 BOOT 按键

static MinecraftServer server(MC_PORT);

// 三个状态循环切换
static const int STATE_COUNT = 3;
static int current_state = 0;

struct ServerStatus {
  int online;
  int max_players;
  const char* description;
};

static const ServerStatus states[STATE_COUNT] = {
  {  0, 100, "This is a Ghost Server"       },
  { 99, 100, "This is a Ghost Server"       },
  { 99, 100, "A Lively and Vibrant Server"  },
};

static void applyState(int s) {
  server.setStatus(states[s].online, states[s].max_players, states[s].description);
  Serial.printf("State %d: %d/%d \"%s\"\n",
    s, states[s].online, states[s].max_players, states[s].description);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32MC Episode 1 - Status Query");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  applyState(current_state);

  if (!server.begin(WIFI_SSID, WIFI_PASS)) {
    Serial.println("WiFi begin failed");
  }
}

static bool last_btn = HIGH;

void loop() {
  // 检测按键下降沿
  bool btn = digitalRead(BUTTON_PIN);
  if (last_btn == HIGH && btn == LOW) {
    current_state = (current_state + 1) % STATE_COUNT;
    applyState(current_state);
  }
  last_btn = btn;

  server.poll();
  vTaskDelay(1);
}
