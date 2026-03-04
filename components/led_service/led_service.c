#include "led_service.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_service.h"

#define LED_STRIP_GPIO    48
#define LED_STRIP_LENGTH  1

static const char *TAG = "LED_SERVICE";
static led_strip_handle_t strip;
static int identify_update = 0;  // diperbaiki typo dari identitfy_update

// ===============================================
// Fungsi-fungsi warna (TANPA static agar sesuai dengan .h)
// ===============================================


void print_lcd(char* msg){

    //lcd_print(msg);
 
}

void led_set_red(void)
{
    led_strip_set_pixel(strip, 0, 255, 0, 0);     // Merah penuh
    led_strip_refresh(strip);
    print_lcd("COLOR: RED");
}

void led_set_green(void)
{
    led_strip_set_pixel(strip, 0, 0, 255, 0);     // Hijau penuh
    led_strip_refresh(strip);
    print_lcd("COLOR: GREEN");
}

void led_set_blue(void)
{
    led_strip_set_pixel(strip, 0, 0, 0, 255);     // Biru penuh
    led_strip_refresh(strip);
    print_lcd("COLOR: BLUE");
}

void led_set_yellow(void)
{
    led_strip_set_pixel(strip, 0, 255, 255, 0);   // Kuning
    led_strip_refresh(strip);
    print_lcd("COLOR: YELLOW");
}

void led_set_cyan(void)
{
    led_strip_set_pixel(strip, 0, 0, 255, 255);   // Cyan
    led_strip_refresh(strip);
    print_lcd("COLOR: CYAN");
}

void led_set_magenta(void)
{
    led_strip_set_pixel(strip, 0, 255, 0, 255);   // Magenta
    led_strip_refresh(strip);
    print_lcd("COLOR: MAGENTA");
}

void led_set_white(void)
{
    led_strip_set_pixel(strip, 0, 255, 255, 255); // Putih penuh
    led_strip_refresh(strip);
    print_lcd("COLOR: WHITE");
}

void led_set_orange(void)
{
    led_strip_set_pixel(strip, 0, 255, 100, 0);   // Oranye
    led_strip_refresh(strip);
    print_lcd("COLOR: ORANGE");
}

void led_set_purple(void)
{
    led_strip_set_pixel(strip, 0, 128, 0, 128);   // Ungu
    led_strip_refresh(strip);
    print_lcd("COLOR: PURPLE");
}

void led_set_off(void)
{
    led_strip_clear(strip);
    led_strip_refresh(strip);
    print_lcd("COLOR: OFF");
}

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(strip, 0, r, g, b);
    led_strip_refresh(strip);
    print_lcd("COLOR: RGB");
}

// ===============================================

void init_led_strip(void)  // TANPA static
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = LED_STRIP_LENGTH,
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));
    ESP_LOGI(TAG, "LED strip initialized");
}

static void blink_led_task(void *pvParameters)
{
    init_led_strip();

    while (1) {
        if (identify_update == 0) {
            led_set_green();        // Hijau saat normal
        } else {
            led_set_blue();         // Biru saat identify/update
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

        led_set_off();              // Matikan sebentar
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void color_demo_task(void *pvParameters)
{
    init_led_strip();

    while (1) {
        ESP_LOGI(TAG, "Menampilkan warna demo...");
        led_set_red();     vTaskDelay(pdMS_TO_TICKS(800));
        led_set_green();   vTaskDelay(pdMS_TO_TICKS(800));
        led_set_blue();    vTaskDelay(pdMS_TO_TICKS(800));
        led_set_yellow();  vTaskDelay(pdMS_TO_TICKS(800));
        led_set_cyan();    vTaskDelay(pdMS_TO_TICKS(800));
        led_set_magenta(); vTaskDelay(pdMS_TO_TICKS(800));
        led_set_white();   vTaskDelay(pdMS_TO_TICKS(800));
        led_set_orange();  vTaskDelay(pdMS_TO_TICKS(800));
        led_set_purple();  vTaskDelay(pdMS_TO_TICKS(800));
        led_set_off();     vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void led_service_start(void)
{
    ESP_LOGI(TAG, "LED service started");
    xTaskCreate(blink_led_task, "blink_led", 4096, NULL, 5, NULL);
}

void led_color_demo_start(void)
{
    ESP_LOGI(TAG, "LED color demo started");
    xTaskCreate(color_demo_task, "color_demo", 4096, NULL, 5, NULL);
}

void led_set_identify_mode(bool enable)
{
    identify_update = enable ? 1 : 0;
}
