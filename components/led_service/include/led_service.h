#ifndef LED_SERVICE_H
#define LED_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

// Fungsi utama
void led_service_start(void);
void led_color_demo_start(void);
void led_set_identify_mode(bool enable);

// Fungsi warna (tambahkan semua yang kamu pakai)
void init_led_strip(void);           // <--- WAJIB ditambahkan
void led_set_red(void);
void led_set_green(void);
void led_set_blue(void);
void led_set_yellow(void);
void led_set_cyan(void);
void led_set_magenta(void);
void led_set_white(void);
void led_set_orange(void);
void led_set_purple(void);           // <--- WAJIB ditambahkan
void led_set_off(void);
void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

#endif // LED_SERVICE_H





