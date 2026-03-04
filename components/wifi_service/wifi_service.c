#include "wifi_service.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_event.h"
#include <string.h>
#include "lwip/ip_addr.h"   // Ini yang mendefinisikan IP_ADDR4
			    //
static const char *TAG = "WIFI_SERVICE";

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{

	

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected from router, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

	esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (sta_netif == NULL) {
            ESP_LOGE(TAG, "Failed to get netif");
            return;
        }



       
        // ==================================

        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Connected to router!");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Client connected to AP: " MACSTR ", AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Client disconnected from AP: " MACSTR ", AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_service_init(void)
{
    ESP_LOGI(TAG, "Initializing AP+STA mode");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();



    esp_netif_dhcpc_stop(sta_netif);  // Stop DHCP (aman, kalau belum start tidak error)

    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 1, 100);        // IP yang kamu mau
    IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);          // Gateway (biasanya IP router)
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);   // Netmask

    ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));

    // Optional: Set DNS manual (biar bisa resolve hostname)
    esp_netif_dns_info_t dns_info;
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 192, 168, 1, 1);  // DNS primer (bisa router atau 8.8.8.8)
    dns_info.ip.type = IPADDR_TYPE_V4;
    ESP_ERROR_CHECK(esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info));

    // Optional: DNS sekunder (contoh Google DNS)
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8);
    ESP_ERROR_CHECK(esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_BACKUP, &dns_info));
    // ============================================







    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = ESP_WIFI_SSID_AP,
            .ssid_len = strlen(ESP_WIFI_SSID_AP),
            .password = ESP_WIFI_PASS_AP,
            .max_connection = ESP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 1,
        },
    };

    if (strlen(ESP_WIFI_PASS_AP) == 0) {
        wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi started in AP+STA mode");
    ESP_LOGI(TAG, "AP SSID: %s | Password: %s | IP AP: 192.168.4.1",
             ESP_WIFI_SSID_AP, ESP_WIFI_PASS_AP);
}

bool wifi_service_is_connected_and_got_ip(void)
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) != ESP_OK) {
        return false;
    }

    return (ip_info.ip.addr != 0);
}
