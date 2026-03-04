idf_component_register(SRCS "mqtt_service.c"
                    INCLUDE_DIRS "include"
		    REQUIRES mqtt esp_wifi nvs_flash esp_https_ota app_update led_service
		    EMBED_FILES "certs/server.pem"
	    )
