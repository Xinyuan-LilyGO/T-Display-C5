#include "Arduino.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7789.h"
#include "TCA6408.h"
#include "CST816S.h"
#include "board_config.h"
#include "lvgl.h"

// 引脚定义
#define LCD_PIN_CS 3   // 片选引脚
#define LCD_PIN_DC 8   // 数据/命令选择引脚
#define LCD_PIN_RST -1 // 复位引脚
#define LCD_PIN_MOSI 9 // SPI MOSI引脚
#define LCD_PIN_SCLK 7 // SPI时钟引脚
#define LCD_PIN_BL 10  // 背光引脚

// 屏幕参数
#define LCD_WIDTH 320
#define LCD_HEIGHT 170
#define LCD_SPI_CLOCK 20000000 // 40MHz SPI时钟

CST816S touch(IIC_SDA_PIN, IIC_SCL_PIN, -1, TP_INT); // sda, scl, rst, irq

// 全局变量
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;

#ifdef CONFIG_TOUCH_CST816S
volatile bool touchEventFlag = false;
void IRAM_ATTR onTouchInterrupt()
{
    touchEventFlag = true;
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (touchEventFlag)
    {
        touchEventFlag = false;
        if (touch.available())
        {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = touch.data.x;
            data->point.y = touch.data.y;
            Serial.printf("Touch at (%d, %d)\n", data->point.x, data->point.y);
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

void lcd_init(void)
{
    pinMode(LCD_PIN_BL, OUTPUT);
    digitalWrite(LCD_PIN_BL, HIGH);
    TCA6408_ioExpander.pinMode(IO_EXPANDER_LCD_BLK, OUTPUT);
    TCA6408_ioExpander.digitalWrite(IO_EXPANDER_LCD_BLK, HIGH);

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
    TCA6408_ioExpander.pinMode(IO_EXPANDER_LCD_RES, OUTPUT);
    TCA6408_ioExpander.digitalWrite(IO_EXPANDER_LCD_RES, HIGH);
    delay(25);
    TCA6408_ioExpander.digitalWrite(IO_EXPANDER_LCD_RES, LOW);
    delay(25);
    TCA6408_ioExpander.digitalWrite(IO_EXPANDER_LCD_RES, HIGH);
    delay(125);

    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // 设置屏幕方向和颜色
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 35));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    Serial.println("LCD initialized successfully!");
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    esp_lcd_panel_draw_bitmap(panle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
    lv_display_flush_ready(disp);
}

void lvgl_init(void)
{
    lv_init();
    lv_tick_set_cb(my_tick);

    lv_disp_rotation_t rotation = LV_DISPLAY_ROTATION_0;
    lv_display_t *display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_rotation(display, rotation);
    lv_display_set_user_data(display, panel_handle);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    size_t draw_buffer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(lv_color_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    assert(buf1);
    void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    assert(buf2);

    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, display_flush_cb);

#ifdef CONFIG_TOUCH_CST816S
    lv_indev_t *tp = lv_indev_create();
    lv_indev_set_type(tp, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(tp, my_touchpad_read);
#endif
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing ST7789 LCD...");

    Wire.begin(IIC_SDA_PIN, IIC_SCL_PIN);
    TCA6408_ioExpander.begin(&Wire);
    lcd_init();

#ifdef CONFIG_TOUCH_CST816S
    TCA6408_ioExpander.pinMode(IO_EXPANDER_3, OUTPUT);
    TCA6408_ioExpander.digitalWrite(IO_EXPANDER_3, LOW);
    delay(50);
    TCA6408_ioExpander.digitalWrite(IO_EXPANDER_3, HIGH);
    delay(100);

    touch.begin(RISING);
    touch.setRotation(3); 
    touch.attachUserInterrupt(onTouchInterrupt);
#endif

    lvgl_init();
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x00FF00), 0);

    lv_obj_t *button = lv_button_create(scr);
    lv_obj_set_size(button, 100, 50);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "button");
    lv_obj_center(label);

    lv_obj_t *button1 = lv_button_create(scr);
    lv_obj_set_size(button1, 100, 50);
    lv_obj_align(button1, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t *label1 = lv_label_create(button1);
    lv_label_set_text(label1, "button1");
    lv_obj_center(label1);
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}