#pragma once
#include <stdint.h>
#include "ws2811.h"

#define GPIO_PIN 18

int setup_strip();
void cleanup_strip();
void set_led_color(int, uint8_t, uint8_t, uint8_t);
void set_led_32int_color(int, uint32_t);
void set_strip_color(uint8_t, uint8_t, uint8_t);
void set_strip_32int_color(uint32_t);
void set_brightness(uint8_t);
int change_led_count(int);
int get_led_count();
ws2811_t *get_ledstring();
void show_strip();