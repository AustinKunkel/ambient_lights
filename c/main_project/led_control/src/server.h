#include <stdint.h>
#pragma once
struct led_position {
  int x;
  int y;
  int side;  // 0 for right, 1 for top, 2 for left, 3 for bottom
  uint32_t color; // 0xRRGGBB
};

void send_led_strip_colors(struct led_position*);