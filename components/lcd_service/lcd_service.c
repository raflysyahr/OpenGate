#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "lcd_service.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define TAG "LCD"

/* ================= I2C CONFIG ================= */

#define I2C_MASTER_SCL_IO   GPIO_NUM_5
#define I2C_MASTER_SDA_IO   GPIO_NUM_4
#define I2C_MASTER_FREQ_HZ  400000

#define SLAVE_ADDRESS_LCD   (0x4E >> 1)

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t lcd_dev = NULL;

/* ================= LCD CONFIG ================= */

#define LCD_ROWS 2
#define LCD_COLS 16

static char lcd_buffer[LCD_ROWS][LCD_COLS + 1];
static uint8_t lcd_line_index = 0;

/* ========================================================= */

esp_err_t i2c_master_init(void)
{
    esp_err_t ret;

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&bus_config, &i2c_bus);
    if (ret != ESP_OK) return ret;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SLAVE_ADDRESS_LCD,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &lcd_dev);

    return ret;
}

/* ========================================================= */

static esp_err_t lcd_i2c_write(uint8_t *data, size_t len)
{
    return i2c_master_transmit(lcd_dev, data, len, -1);
}

/* ========================================================= */

void lcd_send_cmd(char cmd)
{
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;

    uint8_t data_t[4] = {
        data_u | 0x0C,
        data_u | 0x08,
        data_l | 0x0C,
        data_l | 0x08
    };

    if (lcd_i2c_write(data_t, 4) != ESP_OK)
        ESP_LOGE(TAG, "CMD write error");
}

void lcd_send_data(char data)
{
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;

    uint8_t data_t[4] = {
        data_u | 0x0D,
        data_u | 0x09,
        data_l | 0x0D,
        data_l | 0x09
    };

    if (lcd_i2c_write(data_t, 4) != ESP_OK)
        ESP_LOGE(TAG, "DATA write error");
}

/* ========================================================= */

void lcd_clear_screen(void)
{
    lcd_send_cmd(0x01);
    usleep(5000);
}

void lcd_put_cur(int row, int col)
{
    uint8_t addr = (row == 0) ? 0x80 : 0xC0;
    lcd_send_cmd(addr | col);
}

/* ========================================================= */

void lcd_init(void)
{
    usleep(50000);

    lcd_send_cmd(0x30);
    usleep(5000);
    lcd_send_cmd(0x30);
    usleep(200);
    lcd_send_cmd(0x30);
    usleep(10000);
    lcd_send_cmd(0x20);

    lcd_send_cmd(0x28);
    lcd_send_cmd(0x08);
    lcd_send_cmd(0x01);
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x0C);
}

/* ========================================================= */

static void lcd_render(void)
{
    lcd_clear_screen();

    for (uint8_t r = 0; r < LCD_ROWS; r++) {
        lcd_put_cur(r, 0);
        char *str = lcd_buffer[r];
        while (*str) lcd_send_data(*str++);
        usleep(4000);
    }
}

/* ========================================================= */

void lcd_print(const char *str)
{
    if (lcd_line_index < LCD_ROWS) {

        snprintf(lcd_buffer[lcd_line_index],
                 sizeof(lcd_buffer[lcd_line_index]),
                 "%s", str);

        lcd_put_cur(lcd_line_index, 0);

        while (*str)
            lcd_send_data(*str++);

        lcd_line_index++;

    } else {

        snprintf(lcd_buffer[0], sizeof(lcd_buffer[0]), "%s", lcd_buffer[1]);
        snprintf(lcd_buffer[1], sizeof(lcd_buffer[1]), "%s", str);

        lcd_render();
    }
}

void lcd_print_xy(uint8_t col, uint8_t row, const char *msg)
{
    lcd_put_cur(row, col);
    while (*msg)
        lcd_send_data(*msg++);
}
