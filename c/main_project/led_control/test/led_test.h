#include <stdint.h>
#ifndef LED_TEST_H
#define LED_TEST_H

ws2811_t ledstring;
void cleanup_strip();
int setup_strip();
void set_led_color(int, uint8_t, uint8_t, uint8_t);
char *led_test();
char *turn_led_off_test();

#endif