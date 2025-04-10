#include "led_functions.h"
#include <stdlib.h>

ws2811_t ledstring = {
  .freq = WS2811_TARGET_FREQ,
  .dmanum = 10,
  .channel = {
    [0] = {
      .gpionum = GPIO_PIN,
      .invert = 0,
      .brightness = 255,
      .strip_type = WS2811_STRIP_GRB,
    },
  },
};

int setup_strip(int led_count)
{
  cleanup_strip();
  ledstring.channel[0].count = led_count;
  if (ws2811_init(&ledstring) != WS2811_SUCCESS) 
  {
    return 1;
  }
  return 0;
}

void cleanup_strip()
{
  ws2811_fini(&ledstring);
}

int change_led_count(int new_count) {
  cleanup_strip();
  return setup_strip(new_count);
}

void set_led_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    ledstring.channel[0].leds[index] = (r << 16) | (g << 8) | b;
}

void set_led_32int_color(int index, uint32_t color) {
  ledstring.channel[0].leds[index] = color;
}

void set_strip_color(uint8_t r, uint8_t g, uint8_t b)
{
  for (int i = 0; i < ledstring.channel[0].count; i++) {
    set_led_color(i, r, g, b); // Set to red
  }
}

void set_strip_32int_color(uint32_t color) {
  for (int i = 0; i < ledstring.channel[0].count; i++) {
    set_led_32int_color(i, color);
  }
}

void set_brightness(uint8_t brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;  // Max brightness is 255

  ledstring.channel[0].brightness = brightness;
}

int get_led_count() {
  return ledstring.channel[0].count;
}

ws2811_t *get_ledstring() {
  return &ledstring;
}

void show_strip() {
  ws2811_render(&ledstring);
}