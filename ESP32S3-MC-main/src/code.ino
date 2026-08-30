/**
 * ESP32MC - Minecraft Java Server on ESP32
 * 协议版本 26.1.2 / 775
 * 纯 AP 热点模式，不连接外部 WiFi
 * 每次启动生成不同世界
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include "game_state.h"
#include "mc_server.h"

static const uint16_t MC_PORT = 25565;
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

void setup() {
    Serial.begin(115200);
    delay(500);

    esp_reset_reason_t reset_reason = esp_reset_reason();
    Serial.println("\n========================================");
    Serial.println("  ESP32-MC Server (AP Mode)");
    Serial.println("  Protocol: 26.1.2 / 775");
    Serial.println("========================================");
    Serial.printf("Reset reason: %d (%s)\n", (int)reset_reason, resetReasonString(reset_reason));
    Serial.printf("Free heap on boot: %u bytes\n", ESP.getFreeHeap());

    // ====== 生成随机世界种子 ======
    uint32_t seed = esp_random();
    seed ^= (uint32_t)micros() << 16;
    seed ^= (uint32_t)millis() << 8;
    seed ^= (uint32_t)esp_random() << 24;
    if (seed == 0) seed = 0xDEADBEEF;
    world_seed = seed;
    rng_seed = seed ^ 0x350B10FB;
    
    Serial.printf("World Seed: 0x%08X\n", world_seed);
    Serial.printf("RNG Seed: 0x%08X\n", rng_seed);

    // ====== 启动 AP 热点（纯热点模式） ======
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-MC", "ESP32-MC");
    
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP started, IP: ");
    Serial.println(ip);
    Serial.print("MAC Address: ");
    Serial.println(WiFi.softAPmacAddress());

    // ====== 启动服务器（传空字符串表示 AP 模式） ======
    if (!server.begin("", "")) {
        Serial.println("[ERROR] Server failed to start!");
    } else {
        Serial.println("[OK] Server started successfully!");
        Serial.print("[OK] Listening on port ");
        Serial.println(MC_PORT);
        Serial.print("[OK] Connect with Minecraft Java 26.1.2 at ");
        Serial.print(ip);
        Serial.println(":25565");
        server_started = true;
    }
    
    Serial.println("========================================");
    Serial.println("  Connect to WiFi: ESP32-MC");
    Serial.println("  Password: ESP32-MC");
    Serial.println("  Server IP: 192.168.4.1:25565");
    Serial.println("========================================");
}

void loop() {
    if (server_started) {
        server.poll();
    }
    vTaskDelay(1);
}