#include "web_server_service.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include "esp_ota_ops.h"
#include "esp_netif.h"
#include <sys/param.h>
#include "lcd_service.h"

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;













static esp_ota_handle_t update_handle = 0;
static const esp_partition_t *update_partition = NULL;




static esp_err_t update_post_handler(httpd_req_t *req)
{
    char buf[1024];
    int ret, remaining = req->content_len;

    if (update_partition == NULL) {
        update_partition = esp_ota_get_next_update_partition(NULL);
        if (update_partition == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
            lcd_print("No OTA Partition");
            return ESP_FAIL;
        }

        esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "esp_ota_begin failed");
            lcd_print("Ota Begin Failed");
            return ESP_FAIL;
        }
    }

    while (remaining > 0) {
        /* Baca data POST (ini akan skip boundary/header multipart otomatis kalau pakai httpd_req_recv yang benar) */
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            lcd_print("Receive Failed");
            return ESP_FAIL;
        }

        esp_err_t err = esp_ota_write(update_handle, buf, ret);
        if (err != ESP_OK) {
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "esp_ota_write failed");
            lcd_print("Ota Write Failed");
            return ESP_FAIL;
        }

        remaining -= ret;
    }

    esp_err_t err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "esp_ota_end failed");
        lcd_print("Ota End Failed");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "esp_ota_set_boot failed");
        lcd_print("Set Boot Failed");
        return ESP_FAIL;
    }

    httpd_resp_send(req, "Update success! Rebooting...", HTTPD_RESP_USE_STRLEN);
    lcd_print("Rebooting...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
}






/* Mount LittleFS partition "www" ke /www */
static esp_err_t mount_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/www",
        .partition_label = "www",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount LittleFS (corrupted?)");
            lcd_print("Fail Mount corupt");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition 'www'");
            lcd_print("Failed find part");
        } else {
            ESP_LOGE(TAG, "Failed to mount LittleFS (%s)", esp_err_to_name(ret));
            lcd_print("Failed to Mount");
            
        }
        return ret;
    }

    size_t total = 0, used = 0;
    esp_littlefs_info("www", &total, &used);
    ESP_LOGI(TAG, "LittleFS mounted: /www (%d KB total, %d KB used)", total / 1024, used / 1024);
    
    return ESP_OK;
}

/* Tentukan MIME type berdasarkan ekstensi file */
static const char* get_content_type(const char* filepath)
{
    const char* ext = strrchr(filepath, '.');
    if (!ext) return "application/octet-stream";

    ext++; // skip dot
    if (strcmp(ext, "html") == 0) return "text/html";
    if (strcmp(ext, "css")  == 0) return "text/css";
    if (strcmp(ext, "js")   == 0) return "application/javascript";
    if (strcmp(ext, "json") == 0) return "application/json";
    if (strcmp(ext, "png")  == 0) return "image/png";
    if (strcmp(ext, "jpg")  == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, "gif")  == 0) return "image/gif";
    if (strcmp(ext, "svg")  == 0) return "image/svg+xml";
    if (strcmp(ext, "ico")  == 0) return "image/x-icon";
    if (strcmp(ext, "woff") == 0) return "font/woff";
    if (strcmp(ext, "woff2")== 0) return "font/woff2";
    if (strcmp(ext, "ttf")  == 0) return "font/ttf";

    return "application/octet-stream";
}

/* Handler utama untuk semua static files + fallback SPA */
static esp_err_t static_file_handler(httpd_req_t *req)
{
    char filepath[256];
    struct stat file_stat;

    // Sanitize URI: hilangkan query string dan pastikan tidak ada path traversal
    char uri[128];
    strncpy(uri, req->uri, sizeof(uri) - 1);
    uri[sizeof(uri) - 1] = '\0';

    // Hapus query string jika ada
    char *query = strchr(uri, '?');
    if (query) *query = '\0';

    ESP_LOGI(TAG, "Request URI: %s", uri);
    

    // Root → index.html
    if (strcmp(uri, "/") == 0 || uri[0] == '\0') {
        strcpy(filepath, "/www/index.html");
    } else {
        // Cegah path traversal (../)
        if (strstr(uri, "..") != NULL || strstr(uri, "//") != NULL) {
            ESP_LOGW(TAG, "Possible path traversal attempt: %s", uri);
            
            httpd_resp_send_404(req);
            return ESP_OK;
        }
        snprintf(filepath, sizeof(filepath), "/www%s", uri);
    }

    // Jika file tidak ada atau bukan file reguler → fallback ke index.html (penting untuk SPA!)
    if (stat(filepath, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        ESP_LOGI(TAG, "File not found: %s → fallback to /www/index.html", filepath);
        strcpy(filepath, "/www/index.html");

        if (stat(filepath, &file_stat) != 0) {
            ESP_LOGE(TAG, "index.html not found!");
            httpd_resp_send_404(req);
            return ESP_OK;
        }
    }

    FILE *fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Set content type
    const char *content_type = get_content_type(filepath);
    httpd_resp_set_type(req, content_type);

    // Optional: tambah header caching untuk static assets (kecuali index.html)
    if (strstr(filepath, "index.html") == NULL) {
        httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
    }

    // Streaming file dalam chunk
    char buf[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, sizeof(buf), fd)) > 0) {
        if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK) {
            ESP_LOGE(TAG, "File sending failed!");
            fclose(fd);
            return ESP_FAIL;
        }
    }

    fclose(fd);

    // Akhiri response
    httpd_resp_send_chunk(req, NULL, 0);
    ESP_LOGI(TAG, "Served: %s (%s)", filepath, content_type);
    return ESP_OK;
}

esp_err_t web_server_service_start(void)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }

    if (mount_littlefs() != ESP_OK) {
        return ESP_FAIL;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_open_sockets = 7;
    config.max_uri_handlers = 8;
    config.max_resp_headers = 8;
    config.lru_purge_enable = true;
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;  // <--- BARIS INI YANG HILANG!

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register wildcard handler untuk semua GET request
    httpd_uri_t wildcard_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = static_file_handler,
        .user_ctx = NULL
    };




    httpd_uri_t update_post = {
	    .uri       = "/update",
	    .method    = HTTP_POST,
	    .handler   = update_post_handler,
	    .user_ctx  = NULL
    };
    
    
    httpd_register_uri_handler(server, &update_post);

    if (httpd_register_uri_handler(server, &wildcard_uri) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register wildcard handler");
        httpd_stop(server);
        server = NULL;
        return ESP_FAIL;
    }
	
    esp_netif_t *netif = esp_netif_get_default_netif();
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);


    //ESP_LOGI(TAG, "Web server started → http://",IPSTR, IP2STR(&ip_info.ip),"/");
    ESP_LOGI(TAG, "Web server started → http://" IPSTR "/", IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "Serving static files from LittleFS:/www");
    return ESP_OK;
}

/* Optional: stop server (jika perlu) */
void web_server_service_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        esp_vfs_littlefs_unregister("www");
        ESP_LOGI(TAG, "Web server stopped");
    }
}
