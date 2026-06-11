#ifdef _WIN32

#include "../mc_server.h"
#include "win_platform.h"
#include <stdio.h>
#include <signal.h>
#include <conio.h>  // _kbhit / _getch

SerialClass Serial;

static const uint16_t MC_PORT = 25565;
static MinecraftServer server(MC_PORT);
static volatile bool running = true;

// 三个状态循环切换
static const int STATE_COUNT = 3;
static int current_state = 0;

struct ServerStatus {
  int online;
  int max_players;
  const char* description;
};

static const ServerStatus states[STATE_COUNT] = {
  {  0, 100, "This is a Ghost Server"      },
  { 99, 100, "This is a Ghost Server"      },
  { 99, 100, "A Lively and Vibrant Server" },
};

static void applyState(int s) {
  server.setStatus(states[s].online, states[s].max_players, states[s].description);
  printf("State %d: %d/%d \"%s\"\n",
    s, states[s].online, states[s].max_players, states[s].description);
}

void signal_handler(int sig) {
  (void)sig;
  printf("\nShutting down...\n");
  running = false;
}

int main() {
  signal(SIGINT, signal_handler);
  printf("ESP32MC Episode 1 - Status Query (Windows)\n");
  printf("Port: %d\n", MC_PORT);
  printf("Press any key to cycle server status\n\n");

  applyState(current_state);

  if (!server.begin(nullptr, nullptr)) {
    printf("Server begin failed!\n");
    return 1;
  }

  printf("Server running. Connect with Minecraft to localhost:%d\n\n", MC_PORT);

  while (running) {
    // 按任意键切换状态
    if (_kbhit()) {
      _getch();
      current_state = (current_state + 1) % STATE_COUNT;
      applyState(current_state);
    }

    server.poll();
    Sleep(1);
  }

  return 0;
}

#endif
