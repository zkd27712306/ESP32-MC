# ESP32MC Server

在[ESP32-MC](https://github.com/GYGKHD/ESP32-MC)基础上进行优化，解决很多bug（合成时崩溃等），新增部分功能（!give命令）。

## 开源许可与来源
- 原项目作者：GYGKHD及所有参与代码修改的贡献者
- 原项目许可证：GPL-3.0
- 本项目许可证：GPL-3.0
- 派生项目新增与修改部分由 [zkd27712306] 于 2026 年完成

本派生项目主要修改包括：
- 网络模式从 STA 改为纯 AP 热点模式，移除 WiFi 配网模块，开箱即用
- 世界生成：每次启动生成随机世界种子
- 游戏功能：新增下界合金装备、护甲系统、弓箭系统、无限水桶、火把放置
- 生物 AI：新增苦力怕爆炸、骷髅射箭、僵尸群攻
- 稳定性：增强客户端连接健壮性，修复内存泄漏和光照问题

## 项目定位

一个跑在 ESP32S3 上的极简 Minecraft Java 服务器。

这个项目目前主要面向 Arduino ESP32S3 环境，协议版本是 `26.1.2 / 775`。整体思路是尽量用直接、可追踪的实现，把 Minecraft Java 的基础联机和生存逻辑压到一块资源很紧的芯片上。

它像一个能在 ESP32 上自己跑起来的实验性小型生存服，优先考虑的是：

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
- 护甲系统（护甲值/韧性/减伤）
- 弓箭射击系统
- 无限水桶
- AP 热点模式，开箱即用

当前默认配置比较小，适合先跑通和继续调试：

- 最大玩家数：`5`
- 视距：`2`
- 默认端口：`25565`

这些值和多数开关定义都在 [`ESP32-MC-main/src/game_types.h`](ESP32-MC-main/src/game_types.h)。

## 运行方式

### 在 ESP32S3 上运行

默认入口是 [`ESP32-MC-main/src/code.ino`](ESP32-MC-main/src/code.ino)。

大致流程：

1. 用 Visual Studio Code打开 ESP32-MC-main/src/ 目录
2. 安装 PlatformIO，并在 platformio.ini 中设置开发板型号（如 esp32-s3-devkitc-1）
3. 编译并烧录
4. 设备启动后会打开一个名为ESP32-MC的wifi
5. 服务器开始监听 `25565`
6. Minecraft Java 客户端连接到设备192.168.4.1:25565即可

启动时串口会输出网络状态、IP 地址和启动信息，方便排查。

或者直接下载[ESP32-MC Releases](https://github.com/zkd27712306/ESP32-MC/releases)，并使用烧录工具（如ESPWebTool）把程序直接烧录到你的开发板。

### WiFi 配置

当前可用的稳定方式是热点连接，相关实现见 [`ESP32-MC-main/src/code.ino`](ESP32-MC-main/src/code.ino) 。

基本用法：

1. 通过无线网卡进行连接，wifi和密码都是ESP32-MC

## 目录结构

当前主要代码都在 `ESP32-MC-main/src/` 目录下：

- [`ESP32-MC-main/src/code.ino`](ESP32-MC-main/src/code.ino)：Arduino 入口，初始化串口、WiFi、LED 和主循环
- [`ESP32-MC-main/src/mc_server.cpp`](ESP32-MC-main/src/mc_server.cpp)：服务器主体，连接管理、协议状态机、主要游戏逻辑
- [`ESP32-MC-main/src/packet_codec.cpp`](ESP32-MC-main/src/packet_codec.cpp)：Minecraft 数据包编解码
- [`ESP32-MC-main/src/network_layer.cpp`](ESP32-MC-main/src/network_layer.cpp)：ESP32 网络层封装
- [`ESP32-MC-main/src/procedures.cpp`](ESP32-MC-main/src/procedures.cpp)：玩家行为、方块交互、Mob 和 Tick 相关逻辑
- [`ESP32-MC-main/src/terrain.cpp`](ESP32-MC-main/src/terrain.cpp)：地形、区块和基础结构生成
- [`ESP32-MC-main/src/crafting.cpp`](ESP32-MC-main/src/crafting.cpp)：合成和熔炉逻辑
- [`ESP32-MC-main/src/game_state.cpp`](ESP32-MC-main/src/game_state.cpp)：全局游戏状态
- [`ESP32-MC-main/src/game_types.h`](ESP32-MC-main/src/game_types.h)：主要常量、开关和数据结构
- [`ESP32-MC-main/src/registries.cpp`](ESP32-MC-main/src/registries.cpp)：协议注册表和相关大体积数据
- [`ESP32-MC-main/src/wifi_config.cpp`](ESP32-MC-main/src/wifi_config.cpp)：WiFi 保存和串口配网逻辑（当前未使用）

## 开发说明

- 当前主线代码以 `ESP32-MC-main/src` 为准。
- `registries.cpp / registries.h` 体积较大，主要是协议相关的静态数据
- 这个项目的很多设计是为了节省资源和简化调试，不一定追求常见服务端那种完整抽象
