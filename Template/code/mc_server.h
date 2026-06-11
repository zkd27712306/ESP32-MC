#ifndef MC_SERVER_H
#define MC_SERVER_H

#ifdef _WIN32
#include "win_platform.h"
#include "win_network_layer.h"
#else
#include <WiFi.h>
#include "network_layer.h"
#endif

#include "packet_codec.h"
#include <stdint.h>

class MinecraftServer {
 public:
  explicit MinecraftServer(uint16_t port);

  bool begin(const char* ssid, const char* password);
  void poll();

  void setStatus(int online, int max_players, const char* description);

 private:
  struct ClientSlot {
    int fd;
    uint8_t state;
    bool used;
  };

  static const uint8_t kMaxClients = 1;

  bool acceptClient_();
  void serviceClient_(uint8_t slot_index);
  void closeClient_(uint8_t slot_index);

  NetworkLayer network_;
  ClientSlot clients_[kMaxClients];

  int online_;
  int max_players_;
  char description_[64];
};

#endif
