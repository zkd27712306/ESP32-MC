#ifndef _WIN32  // Windows 版使用 win_network_layer.cpp, 此文件仅 ESP32 编译

#include "network_layer.h"

NetworkLayer::NetworkLayer(uint16_t port)
    : server_(port),
      server_started_(false),
      ip_announced_(false),
      ssid_(nullptr),
      password_(nullptr),
      next_retry_at_ms_(0) {}

bool NetworkLayer::begin(const char* ssid, const char* password, uint32_t connect_timeout_ms) {
  ssid_ = ssid;
  password_ = password;
  next_retry_at_ms_ = 0;

  // 已由外部（WiFiEvent）完成连接，直接启动服务器
  if (WiFi.status() == WL_CONNECTED) {
    ip_announced_ = true;
    startServer_();
    return true;
  }

  ip_announced_ = false;
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid_ != nullptr ? ssid_ : "");

  if (!connectWiFi_(connect_timeout_ms)) {
    Serial.println("WiFi connect failed");
    return false;
  }

  startServer_();
  return true;
}

void NetworkLayer::poll() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!server_started_) {
      startServer_();
    }
    if (!ip_announced_) {
      Serial.print("WiFi connected, IP: ");
      Serial.println(WiFi.localIP());
      ip_announced_ = true;
    }
    return;
  }

  if (ssid_ == nullptr || ssid_[0] == '\0') {
    return;
  }

  if (ip_announced_) {
    Serial.println("WiFi disconnected, reconnecting...");
    ip_announced_ = false;
  }

  uint32_t now = millis();
  if (next_retry_at_ms_ != 0 && (int32_t)(now - next_retry_at_ms_) < 0) {
    return;
  }

  next_retry_at_ms_ = now + 5000;
  WiFi.begin(ssid_, password_ != nullptr ? password_ : "");
}

bool NetworkLayer::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

WiFiClient NetworkLayer::accept() {
  if (WiFi.status() != WL_CONNECTED) {
    return WiFiClient();
  }

  if (!server_started_) {
    startServer_();
  }

  return server_.available();
}

bool NetworkLayer::connectWiFi_(uint32_t connect_timeout_ms) {
  if (ssid_ == nullptr || ssid_[0] == '\0') {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_, password_ != nullptr ? password_ : "");

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= connect_timeout_ms) {
      Serial.println("WiFi connect timeout");
      return false;
    }
    delay(250);
  }

  ip_announced_ = true;
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void NetworkLayer::startServer_() {
  if (server_started_) {
    return;
  }

  server_.begin();
  server_.setNoDelay(true);
  server_started_ = true;

  // 尝试增大 TCP 发送缓冲区
  Serial.println("Server listening on port 25565");
}

#endif // !_WIN32
