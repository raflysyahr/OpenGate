#include <stdbool.h>
#include "ntp_service.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>

static const char *TAG = "NTP_SERVICE";
static bool s_sntp_sync = false;

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "NTP: Time synchronized");
    s_sntp_sync = true;
}

void ntp_service_init(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");

    s_sntp_sync = false;


    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    ip_addr_t ntp_ip1, ntp_ip2;
    IP_ADDR4(&ntp_ip1, 162, 159, 200, 1);  // time.google.com
    IP_ADDR4(&ntp_ip2, 216, 239, 35, 0);

    //esp_sntp_setservername(0, "pool.ntp.org");
    //esp_sntp_setservername(1, "time.google.com");
    
    esp_sntp_setserver(0, &ntp_ip1);
    esp_sntp_setserver(1, &ntp_ip2);

    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    int retry = 0;
    const int retry_count = 10;
    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (s_sntp_sync && timeinfo.tm_year >= (2026 - 1900)) {
        ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        s_sntp_sync = true;
    } else {
        ESP_LOGE(TAG, "NTP synchronization failed!");
        s_sntp_sync = false;
    }
}

bool ntp_service_is_synced(void)
{
    return s_sntp_sync;
}
