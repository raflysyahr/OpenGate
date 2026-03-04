#include "esp_err.h"

esp_err_t i2c_master_init(void);
void lcd_init (void);   // initialize lcd

void lcd_send_cmd (char cmd);  // send command to the lcd

void lcd_send_data (char data);  // send data to the lcd

void lcd_print (const char *str);  // send string to the lcd

void lcd_put_cur(int row, int col);  // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_clear_screen (void);
void lcd_print_xy(uint8_t col, uint8_t row, const char *msg);