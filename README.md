# ESP32MC Server

一个跑在 ESP32S3 上的极简 Minecraft Java 服务器。

这个项目目前主要面向 Arduino ESP32S3 环境，协议版本是 `26.1.2 / 775`。整体思路是尽量用直接、可追踪的实现，把 Minecraft Java 的基础联机和生存逻辑压到一块资源很紧的芯片上。

代码思路参考了 [bareiron](https://github.com/p2r3/bareiron)，但这个仓库已经做了适配和重构，当前主线代码以 `code/` 目录为准。

## 项目定位

这不是完整原版服，也不是为了兼容插件生态。

它更像一个能在 ESP32 上自己跑起来的实验性小型生存服，优先考虑的是：

- 在 ESP32S3 上能稳定跑起来
- 代码结构尽量直接，方便继续改
- 出问题时容易定位

暂时不优先考虑的是：

- 完整原版特性
- 高并发
- 插件兼容
- 过度包装的工程结构

## 当前能力

现在已经有的内容包括：

- 玩家登录、出生、移动、聊天
- 基础区块生成、地形和生物群系
- 方块放置、破坏、简单流体
- 背包、基础合成、熔炉相关逻辑
- 基础 Mob 刷新和部分行为
- WiFi 连接和持久化保存
- 串口配网流程
- Windows 本地调试版

当前默认配置比较小，适合先跑通和继续调试：

- 最大玩家数：`5`
- 视距：`2`
- 默认端口：`25565`

这些值和多数开关定义都在 [`code/game_types.h`](code/game_types.h)。

## 运行方式

### 在 ESP32S3 上运行

默认入口是 [`code/code.ino`](code/code.ino)。

大致流程：

1. 用 Arduino IDE 或兼容的 ESP32 开发环境打开 `code/` 目录
2. 安装并选择 ESP32S3 对应开发板
3. 编译并烧录
4. 设备启动后会尝试连接已保存的 WiFi
5. 成功联网后，服务器开始监听 `25565`
6. Minecraft Java 客户端连接到设备 IP 即可

启动时串口会输出网络状态、IP 地址和启动信息，方便排查。

### WiFi 配置

当前可用的稳定方式是串口 AT 配网，相关实现见 [`code/wifi_config.cpp`](code/wifi_config.cpp)。

基本用法：

1. 打开串口监视器
2. 发送 `#WIFI_UART_CFG#` 进入配网模式
3. 发送 `AT+HELP` 查看命令
4. 常用命令包括：
- `AT+WSCAN`
- `AT+WSET="<ssid>","<password>"`
- `AT+WSAVE`
- `AT+WINFO?`
- `AT+WEXIT`

代码里也保留了 BOOT 键触发的配置模式入口，但当前 README 只把串口流程作为正式使用方式来说明。

### 在 Windows 上本地调试

仓库提供了一个 Windows 调试入口，方便在桌面环境先跑协议和逻辑：

- 构建脚本：[`code/win/build_win.bat`](code/win/build_win.bat)
- 入口文件：[`code/win/win_main.cpp`](code/win/win_main.cpp)

这个脚本会在首次运行时自动准备 `w64devkit`，然后编译并启动本地版本服务器。

## 目录结构

当前主要代码都在 `code/` 目录下：

- [`code/code.ino`](code/code.ino)：Arduino 入口，初始化串口、WiFi、LED 和主循环
- [`code/mc_server.cpp`](code/mc_server.cpp)：服务器主体，连接管理、协议状态机、主要游戏逻辑
- [`code/packet_codec.cpp`](code/packet_codec.cpp)：Minecraft 数据包编解码
- [`code/network_layer.cpp`](code/network_layer.cpp)：ESP32 网络层封装
- [`code/procedures.cpp`](code/procedures.cpp)：玩家行为、方块交互、Mob 和 Tick 相关逻辑
- [`code/terrain.cpp`](code/terrain.cpp)：地形、区块和基础结构生成
- [`code/crafting.cpp`](code/crafting.cpp)：合成和熔炉逻辑
- [`code/game_state.cpp`](code/game_state.cpp)：全局游戏状态
- [`code/game_types.h`](code/game_types.h)：主要常量、开关和数据结构
- [`code/registries.cpp`](code/registries.cpp)：协议注册表和相关大体积数据
- [`code/wifi_config.cpp`](code/wifi_config.cpp)：WiFi 保存和串口配网逻辑

另外还有一些辅助目录：

- [`code/win/`](code/win/)：Windows 调试版
- `toolchain/`：本地工具链文件
- `Template/`：最初工程模板
- `EP1/`：EP1 视频功能实现

## 开发说明

- 当前主线代码以 `code/` 为准，旧 README 里提到的 `main.c`、`packets.c` 等文件属于更早的版本
- 仓库里不只有源码，也包含了一些构建产物、工具链和调试文件
- `registries.cpp / registries.h` 体积较大，主要是协议相关的静态数据
- 这个项目的很多设计是为了节省资源和简化调试，不一定追求常见服务端那种完整抽象

## 致谢

- 代码思路参考：[bareiron](https://github.com/p2r3/bareiron)
