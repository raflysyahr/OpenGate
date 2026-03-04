idf_component_register(SRCS "ota-portal.c"
                    INCLUDE_DIRS "."
		    REQUIRES web_server_service ntp_service wifi_service mqtt_service led_service ota_partition_service lcd_service hd44780 pcf8574 i2cdev bluetooth_service bt
)
