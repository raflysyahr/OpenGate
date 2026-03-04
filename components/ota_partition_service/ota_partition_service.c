#include "ota_partition_service.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA_PARTITION";

void ota_partition_check_running(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Boot from partition: %s (0x%" PRIx32 ")", running->label, running->address);

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "First boot after OTA - checking stability...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);

            esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "New firmware validated & made permanent");
            } else {
                ESP_LOGE(TAG, "Validation failed: %s", esp_err_to_name(err));
            }
        }
    }
}
