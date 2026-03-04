#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "mqtt_service.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_http_client.h"
#include "led_service.h"






#define TOPIC_REBOOT "esp32s3/reboot"
#define TOPIC_BLINKING "esp32s3/blink"
#define TOPIC_OTA_UPDATE "ota/update"
// Konfigurasi
#define LOG_FILE_PATH       "/www/log.txt"     // Ganti jadi "/littlefs/log.txt" jika pakai LittleFS
#define MAX_LOG_LINE        512
#define MAX_LOG_FILE_SIZE   (100 * 1024)          // 100 KB, sesuaikan kebutuhan


static const char *TAG = "MQTT_SERVICE";


extern const uint8_t server_cert_pem_start[] asm("_binary_server_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_server_pem_end");

// Satu-satunya MQTT client (digunakan untuk OTA + Logging)
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static const char *LOG_TOPIC = "esp32s3/log/general";




static void reboot_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Reboot timer triggered - restarting now!");
    fflush(stdout);
    esp_restart();
}






// Fungsi OTA (sama seperti sebelumnya)
static void perform_ota(const char *url)
{
    ESP_LOGI(TAG, "Starting OTA update from: %s", url);

    esp_http_client_config_t http_config = {
	.url = url,
        .cert_pem = (const char *)server_cert_pem_start,
        .timeout_ms = 20000,
        .keep_alive_enable = true,
    };

                                                                               
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
        .partial_http_download = true,
        .max_http_request_size = 8192,                                         
    };

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK || ota_handle == NULL) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return;                                                                
    }

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) continue;
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
            esp_https_ota_abort(ota_handle);
            return;
        }
        break;
    }

    err = esp_https_ota_finish(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        return;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // 1 detik cukup aman
    










    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    err = esp_ota_set_boot_partition(update_partition);

    if (err == ESP_OK) {

	ESP_LOGI(TAG, "OTA success! Rebooting to new firmware in 3 second ...");


	TimerHandle_t reboot_timer = xTimerCreate(
		"ota_reboot",
		pdMS_TO_TICKS(3000),  // 3 detik delay
		pdFALSE,              // one-shot
		NULL,
		reboot_timer_callback
	);

//fflush(stdout);  // Pastikan semua log keluar sebelum reboot
//        vTaskDelay(3000 / portTICK_PERIOD_MS);
//        esp_restart();
	

    } else {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
    }

    

}






led_color_t led_color_from_string(const char* str)
{
    if (!str) return LED_COLOR_OFF;

    if (strcasecmp(str, "RED") == 0)        return LED_COLOR_RED;
    if (strcasecmp(str, "GREEN") == 0)      return LED_COLOR_GREEN;
    if (strcasecmp(str, "BLUE") == 0)       return LED_COLOR_BLUE;
    if (strcasecmp(str, "YELLOW") == 0)     return LED_COLOR_YELLOW;
    if (strcasecmp(str, "CYAN") == 0)       return LED_COLOR_CYAN;
    if (strcasecmp(str, "MAGENTA") == 0)    return LED_COLOR_MAGENTA;
    if (strcasecmp(str, "WHITE") == 0)      return LED_COLOR_WHITE;
    if (strcasecmp(str, "ORANGE") == 0)     return LED_COLOR_ORANGE;
    if (strcasecmp(str, "PURPLE") == 0)     return LED_COLOR_PURPLE;
    if (strcasecmp(str, "OFF") == 0)        return LED_COLOR_OFF;

    return LED_COLOR_OFF; // default kalau tidak cocok
}




// ========== Handle Topic ================


// Fungsi spesifik per topic
static void handle_reboot(const char* data, size_t data_len)
{
    ESP_LOGI(TAG, "Perintah REBOOT diterima!");
    
    // Opsional: cek payload jika mau konfirmasi (misal hanya reboot jika payload "yes")
    // char payload_str[32];
    // snprintf(payload_str, sizeof(payload_str), "%.*s", (int)data_len, data);
    // if (strcmp(payload_str, "yes") != 0) return;

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Biar log keluar dulu
    esp_restart();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void handle_blinking(const char* data, size_t data_len)
{
    char payload_str[32];
    snprintf(payload_str, sizeof(payload_str), "%.*s", (int)data_len, data);

//    ESP_LOGI(TAG, "Perintah blinking: %s", payload_str);
//

    /**
 * Set warna LED berdasarkan kode warna
 * @param color_code:
 *   0  = Off
 *   1  = Red
 *   2  = Green
 *   3  = Blue
 *   4  = Yellow
 *   5  = Cyan
 *   6  = Magenta
 *   7  = White
 *   8  = Orange
 *   9  = Purple
 *   lainnya = Off (default)
 */

    led_color_t color = led_color_from_string(payload_str);


    switch (color) {
        case LED_COLOR_RED:
            led_set_red();
            ESP_LOGI(TAG, "LED: Red");
            break;
        case LED_COLOR_GREEN:
            led_set_green();
            ESP_LOGI(TAG, "LED: Green");
            break;
        case LED_COLOR_BLUE:
            led_set_blue();
            ESP_LOGI(TAG, "LED: Blue");
            break;
        case LED_COLOR_YELLOW:
            led_set_yellow();
            ESP_LOGI(TAG, "LED: Yellow");
            break;
        case LED_COLOR_CYAN:
            led_set_cyan();
            ESP_LOGI(TAG, "LED: Cyan");
            break;
        case LED_COLOR_MAGENTA:
            led_set_magenta();
            ESP_LOGI(TAG, "LED: Magenta");
            break;
        case LED_COLOR_WHITE:
            led_set_white();
            ESP_LOGI(TAG, "LED: White");
            break;
        case LED_COLOR_ORANGE:
            led_set_orange();
            ESP_LOGI(TAG, "LED: Orange");
            break;
        case LED_COLOR_PURPLE:
            led_set_purple();
            ESP_LOGI(TAG, "LED: Purple");
            break;
        case LED_COLOR_OFF:
        default:
            led_set_off();
            ESP_LOGI(TAG, "LED: Off");
            break;
    }










}




// Fungsi dispatcher utama (dipanggil dari event handler)
static void handle_mqtt_message(const char* topic, size_t topic_len, const char* data, size_t data_len)
{
    // Buat string topic yang null-terminated untuk strcmp
    char topic_str[128];
    snprintf(topic_str, sizeof(topic_str), "%.*s", (int)topic_len, topic);

    ESP_LOGI(TAG, "Handling topic: %s", topic_str);

    if (strcmp(topic_str, TOPIC_REBOOT) == 0) {
        handle_reboot(data, data_len);
    }
    else if (strcmp(topic_str, TOPIC_BLINKING) == 0) {
        handle_blinking(data, data_len);
    }
    else if(strcmp(topic_str, TOPIC_OTA_UPDATE) == 0){
	ESP_LOGI(TAG, "OTA command received");
        char url[256] = {0};
        int data_len_2 = data_len < 256 ? data_len : 255;
        memcpy(url, data, data_len_2);
        url[data_len_2] = '\0';

        perform_ota(url);

    }
    else {
        ESP_LOGW(TAG, "Topic tidak dikenal: %s", topic_str);
    }
}



// ========== End Handle Topic ============













// Fungsi untuk cek dan rotasi log jika terlalu besar (opsional tapi direkomendasikan)
static void rotate_log_if_needed(void) {
    struct stat st;
    if (stat(LOG_FILE_PATH, &st) == 0) {
        if (st.st_size > MAX_LOG_FILE_SIZE) {
            // Hapus file lama, biar mulai baru (atau bisa rename jadi log_old.txt)
            remove(LOG_FILE_PATH);
            ESP_LOGW(TAG, "Log file terlalu besar (%ld bytes), dirotasi (dihapus)", st.st_size);
        }
    }
}









// Custom log function → kirim ke MQTT + serial
static int mqtt_log_vprintf(const char *fmt, va_list args)
{
    char buffer[512];

    char timestamp[16];  // Buffer untuk [HH:MM:SS]
    char full_msg[sizeof(buffer) + sizeof(timestamp)];  // Buffer gabungan

    // Dapatkan waktu saat ini
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", &timeinfo);

    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (len > 0) {
        if (len >= (int)sizeof(buffer)) len = sizeof(buffer) - 1;
        buffer[len] = '\0';

	int full_len = snprintf(full_msg, sizeof(full_msg), "%s%s", timestamp, buffer);

        // Kirim ke MQTT jika sudah connected
        if (mqtt_client && mqtt_connected) {
            esp_mqtt_client_publish(mqtt_client, LOG_TOPIC, full_msg, full_len, 1, 0);
        }

	rotate_log_if_needed();  // cek ukuran dulu

        FILE *f = fopen(LOG_FILE_PATH, "a");
        if (f) {
           fwrite(full_msg, 1, full_len, f);
           fflush(f);                    // pastikan ditulis
           fsync(fileno(f));             // lebih aman (force write ke flash)
           fclose(f);
        } else {
           ESP_LOGE(TAG, "Gagal buka file log untuk tulis: %s", LOG_FILE_PATH);
        }

        // Tetap tampilkan di serial console
        printf("%s", buffer);
	return full_len;
    }

    return len;
}







// Event handler MQTT (satu untuk semua)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected → OTA & Logging active");
        mqtt_connected = true;

	// ============== MQTT SUBSCRIBETION ==============
        esp_mqtt_client_subscribe(mqtt_client, "ota/update", 0);

	esp_mqtt_client_subscribe(mqtt_client, TOPIC_REBOOT, 0);
        ESP_LOGI(TAG, "Subscribed to topic: %s", TOPIC_REBOOT);

	esp_mqtt_client_subscribe(mqtt_client, TOPIC_BLINKING, 0);
        ESP_LOGI(TAG, "Subscribed to topic: %s", TOPIC_BLINKING);

        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected → Logging paused");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_DATA:

	handle_mqtt_message(event->topic, event->topic_len, event->data, event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT Error occurred");
        break;

    default:
        break;
    }
}

esp_err_t mqtt_service_start(void)
{
    static bool already_started = false;
    if (already_started) {
        ESP_LOGW(TAG, "MQTT service already running");
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = "192.168.1.2",
        .broker.address.port = 1883,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        return err;
    }

    // Ganti fungsi logging ESP-IDF agar semua log otomatis ke MQTT
    esp_log_set_vprintf(mqtt_log_vprintf);

    already_started = true;
    ESP_LOGI(TAG, "MQTT Service started: OTA + Remote Logging → %s", LOG_TOPIC);
    return ESP_OK;
}
