#include "ws2811.h"
#ifndef LED_TEST_H
#define LED_TEST_H

extern ws2811_t ledstring;
void cleanup_strip();
int setup_strip();
void set_led_color(int, uint8_t, uint8_t, uint8_t);
void set_led_32int_color(int, uint32_t);
char *led_test();
char *turn_led_off_test();

#endif