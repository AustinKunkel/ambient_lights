#pragma once
#include <stdlib.h>
#include <stdint.h>

typedef struct {
  uint8_t brightness;
  uint32_t color;
  int capture_screen;
  int sound_react;
  int fx_num;
  int count;
  int id; // for sections (not implemented)
} LEDSettings;

extern LEDSettings led_settings;

char *update_leds();
void update_led_vars();