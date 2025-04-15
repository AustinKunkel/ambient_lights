#pragma once
#include <stdint.h>
#include "ws2811.h"

#define GPIO_PIN 18
typedef struct {
  int v_offset;
  int h_offset;
  int avg_color;
  int left_count;
  int top_count;
  int right_count;
  int bottom_count;
  int res_x;
  int res_y;
  short blend_mode;
  int blend_depth;
  short auto_offset;
} CaptureSettings;

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

int setup_strip(int);
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