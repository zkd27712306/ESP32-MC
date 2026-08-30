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
  ip_announced_ = false;

  // ====== 如果是 AP 模式（SSID 为空），直接启动服务器 ======
  if (ssid_ == nullptr || ssid_[0] == '\0') {
    Serial.println("AP mode: Starting server directly");
    startServer_();
    return true;
  }

  // ====== 否则连接 WiFi ======
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid_);

  if (!connectWiFi_(connect_timeout_ms)) {
    Serial.println("WiFi connect failed");
    return false;
  }

  startServer_();
  return true;
}

void NetworkLayer::poll() {
  // AP 模式下不需要处理 WiFi 重连
  if (ssid_ == nullptr || ssid_[0] == '\0') {
    if (!server_started_) {
      startServer_();
    }
    return;
  }

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
  // AP 模式下始终返回 true
  if (ssid_ == nullptr || ssid_[0] == '\0') {
    return true;
  }
  return WiFi.status() == WL_CONNECTED;
}

WiFiClient NetworkLayer::accept() {
  // AP 模式下直接接受连接
  if (ssid_ == nullptr || ssid_[0] == '\0') {
    if (!server_started_) {
      startServer_();
    }
    return server_.available();
  }

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

  Serial.println("Server listening on port 25565");
}

#endif // !_WIN32