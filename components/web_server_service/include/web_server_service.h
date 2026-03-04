#ifndef WEB_SERVER_SERVICE_H
#define WEB_SERVER_SERVICE_H

#include "esp_err.h"

/**
 * Start web server untuk menyajikan static files dari LittleFS partition "www"
 * Folder root: /www di LittleFS → diakses via http://<ip>/
 */
esp_err_t web_server_service_start(void);

#endif // WEB_SERVER_SERVICE_H
