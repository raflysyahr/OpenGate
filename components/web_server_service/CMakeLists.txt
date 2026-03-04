idf_component_register(SRCS "web_server_service.c"
                    INCLUDE_DIRS "include"
		    REQUIRES esp_http_server littlefs app_update esp_netif lcd_service
	    )
