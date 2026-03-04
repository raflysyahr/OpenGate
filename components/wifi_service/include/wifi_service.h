#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include "esp_wifi.h"

#define WIFI_SSID               "Zaidan_4g"
#define WIFI_PASS               "08042003"
#define ESP_WIFI_SSID_AP        "ESP32-S3"
#define ESP_WIFI_PASS_AP        "esp32-s3-rafly"
#define ESP_MAX_STA_CONN        4

void wifi_service_init(void);
bool wifi_service_is_connected_and_got_ip(void);

#endif // WIFI_SERVICE_H
