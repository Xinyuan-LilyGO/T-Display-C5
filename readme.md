# T-Display-C5

The LilyGO T-Display-C5 is a development board based on the **ESP32-C5**, featuring a 1.9-inch ST7789 LCD touchscreen, an AXP2602 power management chip, and a CST816S capacitive touch controller. It supports wake-up from deep sleep via button (IO0) and I2C (IO2, IO3).

## 🛠️ Hardware Specifications

| Item             | Spec                             |
| ---------------- | -------------------------------- |
| SoC              | ESP32-C5 (RISC-V 32-bit, 240MHz) |
| Flash            | 16MB (Quad QIO)                  |
| PSRAM            | 8MB (Quad QIO)                   |
| Display          | 1.9" ST7789, 170×320 RGB565, SPI |
| Touch (optional) | CST816S Capacitive Touch (I2C)   |
| BMU              | AXP2602 Power voltameter         |
| Wireless         | Wi-Fi + Bluetooth                |
| USB              | USB Type-C (USB-Serial-JTAG)     |

## 🔌 Pin Mapping

| Function            | Pin     | Description       |
| ------------------- | ------- | ----------------- |
| **LCD**             |         |                   |
| LCD_CS              | GPIO 26 | SPI Chip Select   |
| LCD_SCK             | GPIO 7  | SPI Clock         |
| LCD_MOSI            | GPIO 9  | SPI Data          |
| LCD_DC              | GPIO 8  | Data/Command      |
| LCD_RST             | GPIO 23 | Reset             |
| LCD_BLK             | GPIO 25 | Backlight PWM     |
| **I2C**             |         |                   |
| I2C_SDA             | GPIO 2  | I2C Data          |
| I2C_SCL             | GPIO 3  | I2C Clock         |
| **Touch (CST816S)** |         |                   |
| TP_INT              | GPIO 27 | Touch Interrupt   |
| TP_RST              | GPIO 24 | Touch Reset       |
| **Buttons**         |         |                   |
| BUTTON (GPIO0)      | GPIO 0  | User Button       |
| BOOT                | GPIO 28 | BOOT Button       |
| **BMU**             |         |                   |
| AXP2602_INT         | GPIO 10 | AXP2602 Interrupt |
>- In DeepSleep mode, the ESP32C5 can be woken up by IO0-IO6 pins; for detailed information, please refer to the datasheet.

## 📦 Software Requirements

| Component     | Version / Notes                              |
| ------------- | -------------------------------------------- |
| ESP-IDF       | ≥ v5.3 (ESP32-C5 support required)           |
| Arduino ESP32 | Based on ESP-IDF v5.3+                       |
| LVGL          | v9.2.0 (included)                            |
| PlatformIO    | Use `develop` branch of platform-espressif32 |

## 📝 Examples

| Example                             | Description                                                                                         |
| ----------------------------------- | --------------------------------------------------------------------------------------------------- |
| 🏭 [factory](examples/factory)       | Factory demo firmware — LVGL dashboard with battery info, WiFi, NTP, touch, buttons, and deep sleep |
| 🖥️ [lcd](examples/lcd)               | LCD basic test — initialize ST7789 and fill screen with red                                         |
| 🎨 [lvgl](examples/lvgl)             | LVGL GUI example — demonstrates button widgets                                                      |
| 🔗 [spi_test](examples/spi_test)     | SPI bus communication test                                                                          |
| 👆 [touch](examples/touch)           | Touch screen test — CST816S touch coordinate reading                                                |
| 🔋 [voltameter](examples/voltameter) | Voltmeter — AXP2602 battery voltage/current/temperature/SOC monitoring                              |
| 📡 [wifi_sat](examples/wifi_sat)     | Wi-Fi SoftAP — ESP32-C5 as a wireless access point                                                  |

## 🚀 Quick Start

### 🤖 PlatformIO IDE

1. Install [VS Code](https://code.visualstudio.com/) with the [PlatformIO extension](https://platformio.org/install/ide/install-vs-code)
2. Clone or download this repository
3. Open the project folder in VS Code
4. In `platformio.ini`, uncomment the `default_envs` line for your target example and comment out others:

```ini
[platformio]
default_envs = factory   ; change to your desired example name
```

5. Connect the board and click the **Upload** button (→)

### 📟 Arduino IDE

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to **File → Preferences**, add the following URL to "Additional Boards Manager URLs":
     `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Open **Tools → Board → Boards Manager**, search for and install **esp32** (v3.x+)
3. Select the board: **Tools → Board → ESP32C5 (or Generic ESP32C5)**
4. Copy the board configuration from `board/Lilygo-T-Display-C5.json` into Arduino's board definition, or manually configure:
   - Flash Size: 16MB
   - Partition Scheme: Default 16MB
   - PSRAM: Enabled (Quad QIO)
5. Open `examples/<example_name>/<example_name>.ino`
6. Copy `include/board_config.h` and the libraries from `lib/` into your Arduino libraries folder or alongside the sketch
7. Click **Upload**

> **Note**: The ESP32-C5 requires a recent Arduino ESP32 version. If you encounter issues, PlatformIO is recommended.

## 📁 Project Structure

```
T-Display-C5/
├── board/              # PlatformIO board definition
├── doc/                # Documentation & schematics
├── examples/           # Example sketches
│   ├── factory/
│   ├── lcd/
│   ├── lvgl/
│   ├── spi_test/
│   ├── touch/
│   ├── voltameter/
│   └── wifi_sat/
├── include/            # Header files (board_config.h)
├── lib/                # Libraries
│   ├── AXP2602/        # Power management library
│   ├── Button/         # Button library
│   ├── CST816S/        # Touch library
│   ├── esp_lcd_st7789/ # ST7789 LCD driver
│   └── lvgl-9.2.0/    # LVGL graphics library
├── firmware/           # Build output (.bin)
├── platformio.ini      # PlatformIO configuration
└── extra_script.py     # Post-build script
```
