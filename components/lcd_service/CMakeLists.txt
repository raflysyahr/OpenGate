idf_component_register(SRCS "lcd_service.c"
                    INCLUDE_DIRS "include"
		    REQUIRES driver hd44780 pcf8574 i2cdev
	    )
