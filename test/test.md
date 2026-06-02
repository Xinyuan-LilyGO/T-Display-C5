<h1 align = "center">T-Display-C5</h1>

## 调试问题记录
- 电量计的I2C引脚连接反了
- BOOT引脚接错
- 屏幕的CS引脚接到了拓展IO芯片，导致写其他库时，需要手动操作电平，修改为MCU直接控制
- 电量计的中断引脚接到拓展芯片

## Pin Map
| Screen pins | ESP32C5 pins |
| :---------: | :----------: |
|   LCD_SCK   |     IO7      |
|   LCD_DC    |     IO8      |
|  LCD_MOSI   |     IO9      |
|  BLK_POWER  |     IO10     |
|   TP_SCL    |     IO5      |
|   TP_SDA    |     IO4      |
|   TP_INT    |     IO6      |


