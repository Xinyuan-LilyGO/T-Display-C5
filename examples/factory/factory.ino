#include "Arduino.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7789.h"
#include "board_config.h"
#include "lvgl.h"
#include "WiFi.h"
#include "WiFiMulti.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time.h"
#include "axp2602.h"
#include "esp_sleep.h"
#include "driver/ledc.h"
#include "LilyGo_Button.h"

#define LCD_SPI_CLOCK 40000000 // 40MHz SPI时钟
#define LCD_PWM_FREQ 5000      // PWM频率5kHz
#define LCD_PWM_RES 8          // PWM分辨率8位(0-255)
#define LCD_PWM_MAX 255        // 最大亮度值

// ---------- Modern Color Palette ----------
#define COLOR_BG lv_color_hex(0x0D1117)     // Dark background (GitHub dark)
#define COLOR_CARD lv_color_hex(0x161B22)   // Card background
#define COLOR_GREEN lv_color_hex(0x00E676)  // Battery high / success
#define COLOR_CYAN lv_color_hex(0x00BCD4)   // Voltage accent
#define COLOR_PURPLE lv_color_hex(0x7C4DFF) // Current accent
#define COLOR_ORANGE lv_color_hex(0xFF9100) // Temperature accent
#define COLOR_YELLOW lv_color_hex(0xFFD600) // Battery medium
#define COLOR_RED lv_color_hex(0xFF1744)    // Battery low / error
#define COLOR_WHITE lv_color_hex(0xFFFFFF)  // Primary text
#define COLOR_GRAY lv_color_hex(0x8B949E)   // Secondary text
#define COLOR_DIM lv_color_hex(0x30363D)    // Border / divider

#define WIFI_SSID_1 "xinyuandianzi"
#define WIFI_PASSWORD_1 "AA15994823428"
#define WIFI_SSID_2 "LilyGo-AABB"
#define WIFI_PASSWORD_2 "xinyuandianzi"

esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;

AXP2602 axp;
LilyGo_Button btn_0;
LilyGo_Button btn_boot;

bool wifi_connected = false;
IPAddress ip;
lv_timer_t *timer;

// ---------- Battery Data Cache ----------
float batt_voltage = 0;
float batt_current = 0;
int8_t batt_temp = 0;
uint8_t batt_soc = 0;
uint16_t batt_tte = 0;
uint16_t batt_ttf = 0;
uint8_t batt_soh = 0;

// ---------- UI Object Pointers ----------
// Top bar
lv_obj_t *time_label;
lv_obj_t *date_label;
lv_obj_t *batt_bar;
lv_obj_t *batt_percent_label;
lv_obj_t *wifi_ssid_label;
lv_obj_t *wifi_ip_label;
// Metric cards
lv_obj_t *volt_label;
lv_obj_t *current_label;
lv_obj_t *temp_label;
lv_obj_t *soh_card_label;
lv_obj_t *touch_x_label;
lv_obj_t *touch_y_label;
// Bottom
lv_obj_t *soc_bar;
lv_obj_t *soc_percent_label;
lv_obj_t *tte_label;
lv_obj_t *ttf_label;

#define DEBOUNCE_TIME 300
unsigned long lastButtonPressTime = 0; // 记录上次按键时间

void lv_timer_cb(lv_timer_t *timer);
lv_obj_t *create_metric_card(lv_obj_t *parent, const char *unit, lv_color_t accent, int x, int y);

// ===================================================================
//  Button Interrupt Handler
// ===================================================================
volatile bool IO23_buttonPressedFlag = false;
volatile bool Boot_buttonPressedFlag = false;
void IRAM_ATTR button_isr_handler(void *arg)
{
    int num = (int)arg;
    unsigned long currentTime = millis();
    // Serial.println("Button pressed, GPIO: " + String(num));
    if (currentTime - lastButtonPressTime > DEBOUNCE_TIME)
    {
        if (num == BUTTON_PIN)
        {
            IO23_buttonPressedFlag = true;
        }
        else if (num == BUTTON_BOOT)
        {
            Boot_buttonPressedFlag = true;
        }
    }
}

#if (CONFIG_TOUCH_CST816S == 1)
#include "CST816S.h"
CST816S touch(IIC_SDA_PIN, IIC_SCL_PIN, -1, TP_INT); // sda, scl, rst, irq
void touch_xy_update(void *data);

// ===================================================================
//  Touch Initialization
// ===================================================================

volatile bool touchEventFlag = false;
void IRAM_ATTR onTouchInterrupt()
{
    touchEventFlag = true;
}

void touch_init(void)
{
    pinMode(TP_RST, OUTPUT);
    digitalWrite(TP_RST, LOW);
    delay(50);
    digitalWrite(TP_RST, HIGH);
    delay(100);

    touch.begin(RISING);
    touch.setRotation(3);
    touch.attachUserInterrupt(onTouchInterrupt);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static int doubleClickCount = 0; // Counter for double clicks

    if (touchEventFlag)
    {
        touchEventFlag = false;
        if (touch.available())
        {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = touch.data.x;
            data->point.y = touch.data.y;
            Serial.printf("Touch at (%d, %d)\n", data->point.x, data->point.y);
            lv_async_call(touch_xy_update, (void *)&touch.data);
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void touch_xy_update(void *data)
{
    int x = ((data_struct *)data)->x;
    int y = ((data_struct *)data)->y;
    // lcd home btn
    if (x == 360 && y == 85)
    {
        lv_obj_set_style_text_color(touch_x_label, COLOR_RED, 0);
        lv_obj_set_style_text_color(touch_y_label, COLOR_RED, 0);
    }
    else
    {
        lv_obj_set_style_text_color(touch_x_label, COLOR_GRAY, 0);
        lv_obj_set_style_text_color(touch_y_label, COLOR_GRAY, 0);
    }

    // Serial.printf("Touch at (%d, %d)\n", x, y);
    lv_label_set_text_fmt(touch_x_label, "X: %d", x);
    lv_label_set_text_fmt(touch_y_label, "Y: %d", y);
}

#endif
// ===================================================================
//  LCD Hardware Initialization
// ===================================================================
void lcd_init(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = LCD_CS,
        .dc_gpio_num = LCD_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_SPI_CLOCK,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    pinMode(LCD_RST, OUTPUT);
    digitalWrite(LCD_RST, HIGH);
    delay(25);
    digitalWrite(LCD_RST, LOW);
    delay(25);
    digitalWrite(LCD_RST, HIGH);
    delay(125);

    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 35));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    Serial.println("LCD initialized successfully!");
}

void lcd_pwm_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)LCD_PWM_RES,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LCD_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_conf = {
        .gpio_num = LCD_BLK_POWER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_conf);
}

void set_lcd_brightness(uint8_t brightness)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void fade_lcd_on(void)
{
    for (int i = 0; i <= LCD_PWM_MAX; i++)
    {
        set_lcd_brightness(i);
        delay(2);
    }
}

void fade_lcd_off(void)
{
    for (int i = LCD_PWM_MAX; i >= 0; i--)
    {
        set_lcd_brightness(i);
        delay(2);
    }
}

// ===================================================================
//  LVGL Display Driver
// ===================================================================
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
    lv_display_t *display = lv_display_create(LCD_HEIGHT, LCD_WIDTH);
    lv_display_set_rotation(display, rotation);
    lv_display_set_user_data(display, panel_handle);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    size_t draw_buffer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(lv_color_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    assert(buf1);
    void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    assert(buf2);
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // lv_display_set_buffers(display, draw_buf, NULL,  sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, display_flush_cb);

#if (CONFIG_TOUCH_CST816S == 1)
    lv_indev_t *tp = lv_indev_create();
    lv_indev_set_type(tp, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(tp, my_touchpad_read);
#endif
}

// ===================================================================
//  Battery gauge color
// ===================================================================

lv_color_t soc_color(uint8_t soc)
{
    if (soc >= 60)
        return COLOR_GREEN;
    if (soc >= 20)
        return COLOR_YELLOW;
    return COLOR_RED;
}

// ===================================================================
//  Create a stylish metric card
// ===================================================================

lv_obj_t *create_metric_card(lv_obj_t *parent, const char *unit, lv_color_t accent, int x, int y)
{
    // Card container
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 73, 54);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    // Top accent bar
    lv_obj_t *accent_bar = lv_obj_create(card);
    lv_obj_set_size(accent_bar, 73, 3);
    lv_obj_set_pos(accent_bar, 0, 0);
    lv_obj_set_style_bg_color(accent_bar, accent, 0);
    lv_obj_set_style_border_width(accent_bar, 0, 0);
    lv_obj_set_style_radius(accent_bar, 0, 0);
    lv_obj_set_style_shadow_width(accent_bar, 0, 0);
    lv_obj_set_style_pad_all(accent_bar, 0, 0);

    // Value label (big number)
    lv_obj_t *val = lv_label_create(card);
    lv_obj_set_pos(val, 6, 8);
    lv_obj_set_size(val, 65, 24);
    lv_label_set_text(val, "--");
    lv_obj_set_style_text_color(val, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_LEFT, 0);

    // Unit label (small, below value)
    lv_obj_t *unit_label = lv_label_create(card);
    lv_obj_set_pos(unit_label, 6, 33);
    lv_obj_set_size(unit_label, 65, 16);
    lv_label_set_text(unit_label, unit);
    lv_obj_set_style_text_color(unit_label, accent, 0);
    lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(unit_label, LV_TEXT_ALIGN_LEFT, 0);

    return val;
}

// ===================================================================
//  UI Initialization - Modern Dashboard Design
// ===================================================================

void lvgl_ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);

    // --- Battery bar (left) ---
    batt_bar = lv_bar_create(scr);
    lv_obj_set_size(batt_bar, 6, 22);
    lv_obj_set_pos(batt_bar, 8, 4);
    lv_obj_set_style_radius(batt_bar, 2, 0);
    lv_obj_set_style_bg_color(batt_bar, COLOR_DIM, 0);
    lv_obj_set_style_bg_color(batt_bar, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(batt_bar, 2, LV_PART_INDICATOR);
    lv_bar_set_range(batt_bar, 0, 100);
    lv_bar_set_value(batt_bar, 75, LV_ANIM_OFF);
    lv_bar_set_mode(batt_bar, LV_BAR_MODE_NORMAL);

    // Battery terminal (small dot at top)
    lv_obj_t *batt_term = lv_obj_create(scr);
    lv_obj_set_size(batt_term, 4, 2);
    lv_obj_set_pos(batt_term, 9, 2);
    lv_obj_set_style_bg_color(batt_term, COLOR_DIM, 0);
    lv_obj_set_style_border_width(batt_term, 0, 0);
    lv_obj_set_style_radius(batt_term, 1, 0);
    lv_obj_set_style_shadow_width(batt_term, 0, 0);
    lv_obj_set_style_pad_all(batt_term, 0, 0);

    // Battery percentage text (next to bar)
    batt_percent_label = lv_label_create(scr);
    lv_obj_set_pos(batt_percent_label, 18, 5);
    lv_obj_set_size(batt_percent_label, 42, 20);
    lv_label_set_text(batt_percent_label, "75%");
    lv_obj_set_style_text_color(batt_percent_label, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(batt_percent_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(batt_percent_label, LV_TEXT_ALIGN_LEFT, 0);

    // --- Time (center) ---
    time_label = lv_label_create(scr);
    lv_obj_set_pos(time_label, 100, 0);
    lv_obj_set_size(time_label, 100, 22);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_color(time_label, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);

    // lv_obj_t *sep = lv_obj_create(scr);
    // lv_obj_set_size(sep, 304, 1);
    // lv_obj_set_pos(sep, 8, 31);
    // lv_obj_set_style_bg_color(sep, COLOR_GRAY, 0);
    // lv_obj_set_style_border_width(sep, 0, 0);
    // lv_obj_set_style_radius(sep, 0, 0);
    // lv_obj_set_style_shadow_width(sep, 0, 0);
    // lv_obj_set_style_pad_all(sep, 0, 0);

    // --- Date (center, below time) ---
    date_label = lv_label_create(scr);
    lv_obj_set_pos(date_label, 100, 24);
    lv_obj_set_size(date_label, 100, 14);
    lv_label_set_text(date_label, "0000-00-00");
    lv_obj_set_style_text_color(date_label, COLOR_GRAY, 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, 0);

    // --- WiFi info (right) ---
    wifi_ssid_label = lv_label_create(scr);
    lv_obj_align(wifi_ssid_label, LV_ALIGN_TOP_RIGHT, -4, 2);
    lv_obj_set_size(wifi_ssid_label, 80, 16);
    lv_label_set_text(wifi_ssid_label, "WiFi");
    lv_obj_set_style_text_color(wifi_ssid_label, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(wifi_ssid_label, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(wifi_ssid_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    // WiFi icon dot
    lv_obj_t *wifi_dot = lv_obj_create(scr);
    lv_obj_set_size(wifi_dot, 6, 6);
    lv_obj_align_to(wifi_dot, wifi_ssid_label, LV_ALIGN_OUT_LEFT_MID, -4, 0);
    lv_obj_set_style_bg_color(wifi_dot, COLOR_GREEN, 0);
    lv_obj_set_style_border_width(wifi_dot, 0, 0);
    lv_obj_set_style_radius(wifi_dot, 3, 0);
    lv_obj_set_style_shadow_width(wifi_dot, 0, 0);
    lv_obj_set_style_pad_all(wifi_dot, 0, 0);

    wifi_ip_label = lv_label_create(scr);
    lv_obj_align_to(wifi_ip_label, wifi_ssid_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    lv_obj_set_size(wifi_ip_label, 80, 14);
    lv_label_set_text(wifi_ip_label, "0.0.0.0");
    lv_obj_set_style_text_color(wifi_ip_label, COLOR_GRAY, 0);
    lv_obj_set_style_text_font(wifi_ip_label, &lv_font_montserrat_12, 0);

    int card_y = 40;
    int card_w = 73;
    int card_gap = 4;
    int total_cards_w = 4 * card_w + 3 * card_gap;
    int card_start_x = (320 - total_cards_w) / 2;

    volt_label = create_metric_card(scr, "VOLTS", COLOR_CYAN,
                                    card_start_x, card_y);
    current_label = create_metric_card(scr, "CURRENT", COLOR_PURPLE,
                                       card_start_x + card_w + card_gap, card_y);
    temp_label = create_metric_card(scr, "TEMP", COLOR_ORANGE,
                                    card_start_x + 2 * (card_w + card_gap), card_y);
    soh_card_label = create_metric_card(scr, "HEALTH", COLOR_GREEN,
                                        card_start_x + 3 * (card_w + card_gap), card_y);

    touch_x_label = lv_label_create(scr);
    // lv_obj_set_pos(touch_x_label, 100, 100);
    lv_obj_set_size(touch_x_label, 50, 16);
    lv_obj_align(touch_x_label, LV_ALIGN_TOP_MID, -80, 100);
    lv_label_set_text(touch_x_label, "X: 0");
    lv_obj_set_style_text_color(touch_x_label, COLOR_GRAY, 0);
    lv_obj_set_style_text_font(touch_x_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(touch_x_label, LV_TEXT_ALIGN_CENTER, 0);

    touch_y_label = lv_label_create(scr);
    // lv_obj_set_pos(touch_y_label, 220, 100);
    lv_obj_set_size(touch_y_label, 50, 16);
    lv_obj_align(touch_y_label, LV_ALIGN_TOP_MID, 80, 100);
    lv_label_set_text(touch_y_label, "Y: 0");
    lv_obj_set_style_text_color(touch_y_label, COLOR_GRAY, 0);
    lv_obj_set_style_text_font(touch_y_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(touch_y_label, LV_TEXT_ALIGN_CENTER, 0);

    // --- SOC progress bar ---
    soc_bar = lv_bar_create(scr);
    lv_obj_set_size(soc_bar, 300, 16);
    lv_obj_align(soc_bar, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_radius(soc_bar, 8, 0);
    lv_obj_set_style_bg_color(soc_bar, COLOR_DIM, 0);
    lv_obj_set_style_bg_color(soc_bar, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(soc_bar, 8, LV_PART_INDICATOR);
    lv_bar_set_range(soc_bar, 0, 100);
    lv_bar_set_value(soc_bar, 75, LV_ANIM_OFF);

    // SOC percentage overlay on bar
    soc_percent_label = lv_label_create(scr);
    lv_obj_align(soc_percent_label, LV_ALIGN_BOTTOM_MID, 0, -31);
    lv_obj_set_size(soc_percent_label, 100, 14);
    lv_label_set_text(soc_percent_label, "SOC 75%");
    lv_obj_set_style_text_color(soc_percent_label, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(soc_percent_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(soc_percent_label, LV_TEXT_ALIGN_CENTER, 0);

    // --- TTE / TTF (bottom two corners) ---
    tte_label = lv_label_create(scr);
    lv_obj_align(tte_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(tte_label, 100, 14);
    lv_label_set_text(tte_label, "Rem: -- min");
    lv_obj_set_style_text_color(tte_label, COLOR_GRAY, 0);
    lv_obj_set_style_text_font(tte_label, &lv_font_montserrat_12, 0);

    ttf_label = lv_label_create(scr);
    lv_obj_align(ttf_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_size(ttf_label, 100, 14);
    lv_label_set_text(ttf_label, "Full: -- min");
    lv_obj_set_style_text_color(ttf_label, COLOR_GRAY, 0);
    lv_obj_set_style_text_font(ttf_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(ttf_label, LV_TEXT_ALIGN_RIGHT, 0);

    // -> Start the 1-second timer
    timer = lv_timer_create(lv_timer_cb, 1000, NULL);
    lv_timer_resume(timer);
}

// ===================================================================
//  Timer Callback - Update all live data
// ===================================================================

void lv_timer_cb(lv_timer_t *timer)
{
    // 1. Read battery gauge
    batt_voltage = axp.getBatteryVoltage();
    batt_current = axp.getBatteryCurrent();
    batt_temp = axp.getTemperature();
    batt_soc = axp.getSOC();
    batt_tte = axp.getTimeToEmpty();
    batt_ttf = axp.getTimeToFull();
    batt_soh = axp.getSOH();

    // 2. Update time / date
    time_t now = time(nullptr);
    if (now > 1000000000ul)
    {
        struct tm *timeinfo = localtime(&now);
        lv_label_set_text_fmt(time_label, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
        lv_label_set_text_fmt(date_label, "%04d-%02d-%02d",
                              timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    }
    else
    {
        lv_label_set_text(time_label, "00:00");
        lv_label_set_text(date_label, "0000-00-00");
    }

    // 3. Update WiFi status
    if (wifi_connected)
    {
        lv_label_set_text_fmt(wifi_ssid_label,"%s", WiFi.SSID().c_str());
        lv_label_set_text_fmt(wifi_ip_label, "%s", ip.toString().c_str());
    }
    else
    {
        lv_label_set_text(wifi_ssid_label, "WiFi Disconnected");
        lv_label_set_text(wifi_ip_label, "0.0.0.0");
    }

    // 4. Update battery gauge UI
    uint8_t soc = batt_soc;
    if (soc > 100)
        soc = 0; // handle invalid reads

    // Battery bar
    lv_bar_set_value(batt_bar, soc, LV_ANIM_OFF);
    lv_color_t bcol = soc_color(soc);
    lv_obj_set_style_bg_color(batt_bar, bcol, LV_PART_INDICATOR);
    lv_label_set_text_fmt(batt_percent_label, "%d%%", soc);
    lv_obj_set_style_text_color(batt_percent_label, bcol, 0);

    // 5. Update metric cards
    lv_label_set_text_fmt(volt_label, "%.2f", batt_voltage);
    lv_label_set_text_fmt(current_label, "%d", (int)(batt_current * 1000));
    lv_label_set_text_fmt(temp_label, "%d", batt_temp);
    lv_label_set_text_fmt(soh_card_label, "%d%%", batt_soh);

    // 6. Update SOC bar
    lv_bar_set_value(soc_bar, soc, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(soc_bar, bcol, LV_PART_INDICATOR);
    lv_label_set_text_fmt(soc_percent_label, "SOC %d%%", soc);

    // 7. Update TTE / TTF
    if (batt_tte < 1440)
        lv_label_set_text_fmt(tte_label, "Rem: %dh%02dmin", batt_tte / 60, batt_tte % 60);
    else
        lv_label_set_text(tte_label, "Rem: --min");

    if (batt_ttf < 1440)
        lv_label_set_text_fmt(ttf_label, "Full: %dh%02dmin", batt_ttf / 60, batt_ttf % 60);
    else
        lv_label_set_text(ttf_label, "Full: --min");
}

// ===================================================================
//  WiFi Task
// ===================================================================
void wifi_task(void *pvParameters)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("WiFi Task started...");
    WiFi.mode(WIFI_STA);

    WiFiMulti wifiMulti;
    wifiMulti.addAP(WIFI_SSID_1, WIFI_PASSWORD_1);
    wifiMulti.addAP(WIFI_SSID_2, WIFI_PASSWORD_2);

    int timeout = 10;
    while (wifiMulti.run() != WL_CONNECTED && timeout > 0)
    {
        delay(1000);
        Serial.print("Connecting to WiFi... ");
        Serial.println(timeout);
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        wifi_connected = true;
        ip = WiFi.localIP();
        Serial.print("WiFi connected! SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP address: ");
        Serial.println(ip);

        configTime(8 * 3600, 0, "ntp.aliyun.com", "cn.ntp.org.cn", "pool.ntp.org");
        Serial.println("Waiting for NTP time sync...");

        int retry = 0;
        while (time(nullptr) < 1000000000ul && retry < 10)
        {
            delay(1000);
            retry++;
            Serial.print(".");
        }

        time_t now = time(nullptr);
        if (now > 1000000000ul)
        {
            struct tm *timeinfo = localtime(&now);
            Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                          timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                          timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        }
        else
        {
            Serial.println("Failed to sync NTP time.");
        }
    }
    else
    {
        Serial.println("WiFi connection failed.");
    }

    vTaskDelete(NULL);
}

void wifi_init(void)
{
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        1,
        NULL,
        0);
}

void Button_0_Callback(ButtonState event)
{
    static bool lcd_on = true;
    switch (event)
    {
    case BTN_CLICK_EVENT:
        Serial.println("Button 0 Clicked!");
        lcd_on = !lcd_on;
        if (lcd_on)
        {
            fade_lcd_on(); // 渐变亮屏
        }
        else
        {
            fade_lcd_off(); // 渐变息屏
        }
        break;
    }
}

void Button_boot_Callback(ButtonState event)
{
    switch (event)
    {
    case BTN_LONG_PRESSED_EVENT:
        Serial.println("Button boot long pressed!");
        WiFi.disconnect();
        fade_lcd_off(); // 渐变息屏

        ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(panel_handle, true));

        esp_deep_sleep_enable_gpio_wakeup(
            (1ULL << GPIO_NUM_0),    // bit mask
            ESP_GPIO_WAKEUP_GPIO_LOW // 低电平唤醒
        );
        Serial.println("Sleeping...");
        esp_deep_sleep_start();
        break;
    }
}

// ===================================================================
//  Main
// ===================================================================
void setup()
{
    delay(1000);
    Serial.begin(115200);
    Serial.println("Initializing T-Display-C5...");
    pinMode(IIC_SDA_PIN, OUTPUT);
    pinMode(IIC_SCL_PIN, OUTPUT);
    Wire.begin(IIC_SDA_PIN, IIC_SCL_PIN);
    Wire.setClock(400000);

    // Init AXP2602 battery gauge
    if (axp.begin() == false)
    {
        Serial.println("AXP2602 init failed – check I2C wiring");
    }
    else
    {
        Serial.println("AXP2602 init success");
        axp.setLowSOCThreshold(10);
        axp.setOverTempThreshold(60);
    }

    btn_0.init(BUTTON_PIN, 50, nullptr);
    btn_0.setEventCallback(Button_0_Callback);
    btn_boot.init(BUTTON_BOOT, 50, nullptr);
    btn_boot.setEventCallback(Button_boot_Callback);

    lcd_pwm_init();
    lcd_init();
#if (CONFIG_TOUCH_CST816S == 1)
    touch_init();
#endif
    set_lcd_brightness(0);
    lvgl_init();
    lvgl_ui_init();

    delay(50);
    fade_lcd_on();

    wifi_init();
}

void loop()
{
    btn_0.update();
    btn_boot.update();
    lv_timer_handler(); /* let the GUI do its work */

    delay(5); /* let this time pass */
}
