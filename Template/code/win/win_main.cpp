#ifdef _WIN32

#include "../mc_server.h"
#include "win_platform.h"
#include <stdio.h>
#include <signal.h>

SerialClass Serial;

static const uint16_t MC_PORT = 25565;
static MinecraftServer server(MC_PORT);
static volatile bool running = true;

void signal_handler(int sig) {
  (void)sig;
  printf("\nShutting down...\n");
  running = false;
}

int main() {
  signal(SIGINT, signal_handler);
  printf("ESP32MC - Status Query (Windows)\n");
  printf("Port: %d\n", MC_PORT);

  if (!server.begin(nullptr, nullptr)) {
    printf("Server begin failed!\n");
    return 1;
  }

  printf("Server running. Connect with Minecraft to localhost:%d\n\n", MC_PORT);

  while (running) {
    server.poll();
    Sleep(1);
  }

  return 0;
}

#endif
