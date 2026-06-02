#include "Arduino.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7789.h"
#include "Wire.h"
// #include "TCA6408.h"
#include "board_config.h"

// 引脚定义
#define LCD_PIN_CS 3   // 片选引脚
#define LCD_PIN_DC 8   // 数据/命令选择引脚
#define LCD_PIN_RST 0  // 复位引脚
#define LCD_PIN_MOSI 9 // SPI MOSI引脚
#define LCD_PIN_SCLK 7 // SPI时钟引脚
#define LCD_PIN_BL 25  // 背光引脚

// 屏幕参数
#define LCD_WIDTH 170
#define LCD_HEIGHT 320
#define LCD_SPI_CLOCK 20000000 // 40MHz SPI时钟

// 全局变量
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;

void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing ST7789 LCD...");

    Wire.begin(IIC_SDA_PIN, IIC_SCL_PIN);
    pinMode(LCD_PIN_BL, OUTPUT);
    digitalWrite(LCD_PIN_BL, HIGH);
    
    spi_bus_config_t bus_config = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1, // 不使用MISO
        .sclk_io_num = LCD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8, // RGB565格式
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    // 2. 配置LCD面板IO
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_PIN_CS,
        .dc_gpio_num = LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_SPI_CLOCK,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    // 绑定SPI设备和LCD面板IO
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    // 3. 配置LCD面板
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };

    // 创建ST7789面板
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    pinMode(LCD_PIN_RST, OUTPUT);
    digitalWrite(LCD_PIN_RST, HIGH);
    delay(25);
    digitalWrite(LCD_PIN_RST, LOW);
    delay(25);
    digitalWrite(LCD_PIN_RST, HIGH);
    delay(125);

    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // 设置屏幕方向和颜色
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 35, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    Serial.println("LCD initialized successfully!");

    // 测试：填充整个屏幕为红色
    uint16_t *buf = (uint16_t *)heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * 2, MALLOC_CAP_DMA);
    if (buf)
    {
        for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++)
        {
            buf[i] = 0xF800; // 红色 RGB565
        }
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, buf);
        free(buf);
        Serial.println("Screen filled with red color");
    }
    else
    {
        Serial.println("Failed to allocate buffer for test");
    }
}

void loop()
{
    Serial.println("Looping...");
    delay(1000);
}