#include "mc_server.h"
#include <string.h>
#include <stdio.h>

// ======================== 驱动层 (平台相关) ========================
// ESP32: lwIP socket API + FreeRTOS
// Windows: Winsock2 API
#ifdef _WIN32
#include "win_platform.h"
#include "win_network_layer.h"
#else
#include <errno.h>
#include <fcntl.h>
#include <lwip/sockets.h>   // [驱动] ESP32 lwIP TCP socket
#endif
// ======================== 驱动层结束 ========================

#define STATE_NONE   0
#define STATE_STATUS 1

namespace {
  const char* VERSION_NAME = "26.1.2";
  const int PROTOCOL_VERSION = 775;
}

// ============================================================
// 构造 / 初始化 (纯逻辑, 无驱动)
// ============================================================

MinecraftServer::MinecraftServer(uint16_t port) : network_(port), online_(0), max_players_(100) {
  for (uint8_t i = 0; i < kMaxClients; ++i) {
    clients_[i].fd = -1;
    clients_[i].used = false;
    clients_[i].state = STATE_NONE;
  }
  strncpy(description_, "This is a Ghost Server", sizeof(description_) - 1);
}

bool MinecraftServer::begin(const char* ssid, const char* password) {
  return network_.begin(ssid, password);  // [驱动] NetworkLayer 内部处理 WiFi/Winsock
}

void MinecraftServer::setStatus(int online, int max_players, const char* description) {
  online_ = online;
  max_players_ = max_players;
  strncpy(description_, description, sizeof(description_) - 1);
  description_[sizeof(description_) - 1] = '\0';
}

// ============================================================
// 主循环 (纯逻辑, 无驱动)
// ============================================================

void MinecraftServer::poll() {
  network_.poll();                  // [驱动] 网络层轮询
  if (!network_.connected()) return;

  acceptClient_();

  for (uint8_t i = 0; i < kMaxClients; ++i) {
    if (!clients_[i].used) continue;
    serviceClient_(i);
  }
}

// ============================================================
// 连接管理 (含驱动: socket accept/close)
// ============================================================

// [驱动] ESP32 需要保持 WiFiClient 引用, 防止析构时关闭底层 fd
#ifndef _WIN32
static WiFiClient kept_clients[1];
#endif

bool MinecraftServer::acceptClient_() {
  // --- 驱动层: 获取新连接 fd ---
#ifdef _WIN32
  SOCKET new_sock = network_.acceptClient();
  if (new_sock == INVALID_SOCKET) return false;
  int new_fd = (int)new_sock;
#else
  WiFiClient client = network_.accept();  // [驱动] WiFi TCP accept
  if (!client) return false;

  int new_fd = client.fd();
  if (new_fd < 0) { client.stop(); return false; }

  // [驱动] 设置非阻塞模式 (lwIP fcntl)
  int flags = fcntl(new_fd, F_GETFL, 0);
  if (flags >= 0) fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
#endif
  // --- 驱动层结束 ---

  // 纯逻辑: 分配客户端槽位
  for (uint8_t i = 0; i < kMaxClients; ++i) {
    if (clients_[i].used) continue;
    clients_[i].fd = new_fd;
    clients_[i].state = STATE_NONE;
    clients_[i].used = true;
#ifndef _WIN32
    kept_clients[i] = client;  // [驱动] 保持引用
#endif
    Serial.println("Client connected");
    return true;
  }

  // 满了, 关闭连接
#ifdef _WIN32
  closesocket(new_sock);  // [驱动] Winsock close
#else
  client.stop();          // [驱动] WiFiClient close
#endif
  return false;
}

void MinecraftServer::serviceClient_(uint8_t slot_index) {
  ClientSlot& slot = clients_[slot_index];
  if (slot.fd < 0) { closeClient_(slot_index); return; }

  // --- 驱动层: peek 检测连接是否还活着 ---
  uint8_t peek_buf[2];
#ifdef _WIN32
  int peek_n = recv((SOCKET)slot.fd, (char*)peek_buf, 2, MSG_PEEK);
  if (peek_n == 0) { closeClient_(slot_index); return; }
  if (peek_n < 0) {
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK) return;  // [驱动] 无数据, 正常
    closeClient_(slot_index); return;
  }
#else
  int peek_n = recv(slot.fd, peek_buf, 2, MSG_PEEK);
  if (peek_n == 0) { closeClient_(slot_index); return; }
  if (peek_n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return;  // [驱动] 无数据, 正常
    closeClient_(slot_index); return;
  }
#endif
  // --- 驱动层结束 ---

  // 纯逻辑: 读包并分发
  PacketCodec codec(slot.fd);
  int32_t packet_len = 0, packet_id = 0;
  if (!codec.readVarInt(packet_len) || !codec.readVarInt(packet_id)) {
    closeClient_(slot_index); return;
  }

  int32_t payload_len = packet_len - codec.sizeVarInt((uint32_t)packet_id);
  bool ok = false;

  switch (slot.state) {
    case STATE_NONE:   ok = handleHandshake_(slot, codec); break;
    case STATE_STATUS: ok = handleStatus_(slot, codec, packet_id, payload_len); break;
    default: ok = false; break;
  }

  if (!ok) closeClient_(slot_index);
}

void MinecraftServer::closeClient_(uint8_t slot_index) {
  ClientSlot& slot = clients_[slot_index];
  if (!slot.used) return;

  // --- 驱动层: 关闭 socket ---
#ifdef _WIN32
  if (slot.fd >= 0) closesocket((SOCKET)slot.fd);  // [驱动] Winsock close
#else
  kept_clients[slot_index].stop();                  // [驱动] WiFiClient close
  kept_clients[slot_index] = WiFiClient();
#endif
  // --- 驱动层结束 ---

  // 纯逻辑: 重置槽位
  slot.fd = -1;
  slot.state = STATE_NONE;
  slot.used = false;

  Serial.println("Client disconnected");
}

// ============================================================
// Handshake (纯协议逻辑, 无驱动)
// ============================================================

bool MinecraftServer::handleHandshake_(ClientSlot& slot, PacketCodec& codec) {
  int32_t protocol_version = 0;
  char server_address[256];
  uint16_t server_port = 0;
  int32_t next_state = 0;

  if (!codec.readVarInt(protocol_version)) return false;
  if (!codec.readString(server_address, sizeof(server_address))) return false;
  if (!codec.readUint16(server_port)) return false;
  if (!codec.readVarInt(next_state)) return false;

  Serial.printf("Handshake: protocol=%d, next_state=%d\n", protocol_version, next_state);

  if (next_state == 1) {
    slot.state = STATE_STATUS;
    return true;
  }

  return false;
}

// ============================================================
// Status (纯协议逻辑, 无驱动)
// ============================================================

bool MinecraftServer::handleStatus_(ClientSlot& slot, PacketCodec& codec, int32_t packet_id, int32_t packet_len) {
  if (packet_id == 0x00) {
    // Status Request -> 回复 Status Response
    char json[256];
    int len = snprintf(json, sizeof(json),
      "{\"version\":{\"name\":\"%s\",\"protocol\":%d},"
      "\"players\":{\"max\":%d,\"online\":%d},"
      "\"description\":{\"text\":\"%s\"}}",
      VERSION_NAME, PROTOCOL_VERSION, max_players_, online_, description_);

    uint32_t json_len = (uint32_t)len;
    uint32_t pkt_len = 1 + codec.sizeVarInt(json_len) + json_len;

    bool ok = codec.writeVarInt(pkt_len) && codec.writeVarInt(0x00) &&
              codec.writeVarInt(json_len) && codec.writeExact((const uint8_t*)json, json_len);

    Serial.println("Sent Status Response");
    return ok;
  }

  if (packet_id == 0x01) {
    // Ping -> 原样返回 Pong
    uint8_t payload[8];
    if (!codec.readExact(payload, 8)) return false;
    bool ok = codec.writeVarInt(9) && codec.writeVarInt(0x01) && codec.writeExact(payload, 8);
    Serial.println("Ping -> Pong");
    closeClient_((uint8_t)(&slot - clients_));
    return ok;
  }

  return codec.skipBytes((size_t)packet_len);
}
