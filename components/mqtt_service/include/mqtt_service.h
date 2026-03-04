#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include "esp_err.h"


typedef enum {
    LED_COLOR_OFF = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_YELLOW,
    LED_COLOR_CYAN,
    LED_COLOR_MAGENTA,
    LED_COLOR_WHITE,
    LED_COLOR_ORANGE,
    LED_COLOR_PURPLE
} led_color_t;

/**
 * Start MQTT service yang menangani:
 * - OTA via topic "ota/update"
 * - Remote logging semua ESP_LOGx() ke topic "esp32s3/log/general"
 * 
 * Harus dipanggil setelah WiFi connected dan NTP synced (opsional tapi direkomendasikan)
 */
esp_err_t mqtt_service_start(void);

#endif // MQTT_SERVICE_H
