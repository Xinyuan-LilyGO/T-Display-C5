# T-Display-C5

LilyGO T-Display-C5 是一款基于 **ESP32-C5** 的开发板，配备 1.9 英寸 ST7789 LCD 触摸屏、AXP2602 电源管理芯片和 CST816S 电容触摸控制器。

## 🛠️ 硬件规格

| 项目 | 参数 |
|------|------|
| SoC | ESP32-C5 (RISC-V 32-bit, 240MHz) |
| Flash | 16MB (Quad QIO) |
| PSRAM | 8MB (Quad QIO) |
| 屏幕 | 1.9" ST7789, 170320 RGB565, SPI |
| 触摸(可选) | CST816S 电容触摸 (I2C) |
| PMU | AXP2602 电源管理 |
| 无线 | Wi-Fi + Bluetooth |
| USB | USB Type-C (USB-Serial-JTAG) |

## 🔌 引脚映射

| 功能 | 引脚 | 说明 |
|------|------|------|
| **LCD** | | |
| LCD_CS | GPIO 26 | SPI 片选 |
| LCD_SCK | GPIO 7 | SPI 时钟 |
| LCD_MOSI | GPIO 9 | SPI 数据 |
| LCD_DC | GPIO 8 | 数据/命令 |
| LCD_RST | GPIO 23 | 复位 |
| LCD_BLK | GPIO 25 | 背光 PWM |
| **I2C** | | |
| I2C_SDA | GPIO 2 | I2C 数据 |
| I2C_SCL | GPIO 3 | I2C 时钟 |
| **触摸 (CST816S)** | | |
| TP_INT | GPIO 27 | 触摸中断 |
| TP_RST | GPIO 24 | 触摸复位 |
| **按键** | | |
| BUTTON (GPIO0) | GPIO 0 | 用户按键 |
| BOOT | GPIO 28 | BOOT 按键 |
| **PMU** | | |
| AXP2602_INT | GPIO 10 | AXP2602 中断 |

## 📦 软件要求

| 组件 | 版本/说明 |
|------|-----------|
| ESP-IDF | v5.3 (需支持 ESP32-C5) |
| Arduino ESP32 | 基于 ESP-IDF v5.3+ |
| LVGL | v9.2.0 (内置) |
| PlatformIO | 需使用 develop 分支的 platform-espressif32 |

## 📝 例程

| 例程 | 说明 |
|------|------|
| 🏭 [factory](examples/factory) | 工厂演示固件 - LVGL 仪表盘，含电池信息、WiFi 连接、NTP 校时、触摸显示、按键控制、深睡眠 |
| 🖥️ [lcd](examples/lcd) | LCD 基本测试 - 初始化 ST7789 并填充红色 |
| 🎨 [lvgl](examples/lvgl) | LVGL GUI 示例 - 演示按钮控件 |
| 🔗 [spi_test](examples/spi_test) | SPI 总线通信测试 |
| 👆 [touch](examples/touch) | 触摸屏测试 - CST816S 触控坐标读取 |
| 🔋 [voltameter](examples/voltameter) | 电压计 - AXP2602 电池电压/电流/温度/SOC 检测 |
| 📡 [wifi_sat](examples/wifi_sat) | Wi-Fi 热点 - ESP32-C5 作为软 AP |

## 🚀 快速开始

### 🤖 PlatformIO IDE

1. 安装 [VS Code](https://code.visualstudio.com/) 和 [PlatformIO 扩展](https://platformio.org/install/ide/install-vs-code)
2. 克隆或下载本仓库
3. 打开项目文件夹
4. 在 `platformio.ini` 中取消注释所需例程的 `default_envs`，然后注释掉其他行：

```ini
[platformio]
default_envs = factory   ; 改为你想运行的例程名
```

5. 连接开发板，点击 **Upload** 按钮

### 📟 Arduino IDE

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)
2. 添加 ESP32 板支持：
   - 打开 **文件 > 首选项**，在附加开发板管理器网址中添加：
     `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - 打开 **工具 > 开发板 > 开发板管理器**，搜索并安装 **esp32** (v3.x+)
3. 选择开发板：**Tools > Board > ESP32C5 (或 Generic ESP32C5)**
4. 复制 `board/Lilygo-T-Display-C5.json` 中的配置到 Arduino 板定义中，或手动配置：
   - Flash Size: 16MB
   - Partition Scheme: Default 16MB
   - PSRAM: Enabled (Quad QIO)
5. 打开 `examples/<example_name>/<example_name>.ino`
6. 将 `include/board_config.h` 和 `lib/` 下的库复制到 Arduino 的库目录或项目旁
7. 点击 **Upload**

> **注意**：ESP32-C5 在 Arduino 中需要较新的支持版本。如遇问题，建议优先使用 PlatformIO。

## 📁 项目结构

```
T-Display-C5/
  board/              # PlatformIO 板定义
  doc/                # 文档和原理图
  examples/           # 例程
  include/            # 头文件 (board_config.h)
  lib/                # 库
  firmware/           # 编译输出 (.bin)
  platformio.ini      # PlatformIO 配置
  extra_script.py     # 编译后脚本
```
