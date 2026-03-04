#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <time.h>
#include "wifi_service.h"
#include "ntp_service.h"
#include "mqtt_service.h"
#include "led_service.h"
#include "ota_partition_service.h"
#include "web_server_service.h"
#include "lcd_service.h"
#include "bluetooth_service.h"

static const char *TAG = "MAIN";



void app_main(void)
{

    
    ESP_LOGI(TAG, "===============================================");
    ESP_LOGI(TAG, ">>> OTA PORTAL                              <<<");
    ESP_LOGI(TAG, "===============================================");




    ESP_ERROR_CHECK(i2c_master_init());
    lcd_init();


    
    lcd_print("SYSTEM READY");

    vTaskDelay(pdMS_TO_TICKS(2000));

    lcd_print("ESP32-S3");
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_print("BOOT OK");




    ESP_ERROR_CHECK(nvs_flash_init());
    ota_partition_check_running();
    ble_service_init();

    init_led_strip();
//    lcd_raw_test(&lcd);

    wifi_service_init();
    led_set_purple();
    web_server_service_start();

    // Tunggu koneksi WiFi + IP
    while (!wifi_service_is_connected_and_got_ip()) {
        lcd_print("Wifi connect...");
        ESP_LOGI(TAG, "Waiting for WiFi connection and IP...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    lcd_print("Wifi Connected");
    ESP_LOGI(TAG, "WiFi connected & got IP! Starting services...");

    ntp_service_init();

    int ntp_retry = 0;
    const int ntp_max_retry = 30;  // 30 x 1 detik = 30 detik timeout

    while (!ntp_service_is_synced() && ntp_retry < ntp_max_retry) {
        ESP_LOGI(TAG, "Waiting for NTP time synchronization... (%d/%d)", ntp_retry + 1, ntp_max_retry);
        lcd_print("Ntp SYNC....");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ntp_retry++;
    }



    if (ntp_service_is_synced()) {


	ESP_LOGI(TAG, "NTP synchronized successfully! Starting MQTT...");
    lcd_print("Ntp Synchronized");

        // Set timezone kalau perlu (contoh: WIB)
        setenv("TZ", "WIB-7", 1);
        tzset();

        mqtt_service_start();
        
        ESP_LOGI(TAG, "Time synchronized, MQTT and logging started");
        lcd_print("Time synchronized");
	led_set_green();
    }else{

	    ESP_LOGE(TAG, "NTP synchronization FAILED or TIMEOUT! MQTT not started.");
        lcd_print("Ntp Failed Sync");
        led_set_red();
    }

    // Loop utama kosong (semua berjalan di task terpisah)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
