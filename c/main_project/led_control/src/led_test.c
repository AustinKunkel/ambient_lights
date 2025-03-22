#include <stdio.h>
#include <stdlib.h>
#include "ws2811.h"

#define LED_COUNT 3  // Number of LEDs
#define GPIO_PIN 12   // GPIO pin for PWM output

void set_led_color(int, uint8_t, uint8_t, uint8_t);
void set_strip_color(uint8_t, uint8_t, uint8_t);


ws2811_t ledstring = {
  .freq = WS2811_TARGET_FREQ,
  .dmanum = 10,
  .channel = {
      [0] = {
          .gpionum = GPIO_PIN,
          .count = LED_COUNT,
          .invert = 0,
          .brightness = 255,
          .strip_type = WS2811_STRIP_GRB,
      },
  },
};


char *led_test() {
  if (ws2811_init(&ledstring) != WS2811_SUCCESS) {
    printf("Failed to initialize LEDs!\n");
    return "{\"message\": \"Failed to initialize LEDs\"}";
  }

  set_strip_color(255, 0, 0); // set strip color to red

  ws2811_render(&ledstring);

  ws2811_fini(&ledstring);
  return "{\"message\": \"Turned on LEDs\"}";
}


void set_led_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    ledstring.channel[0].leds[index] = (r << 16) | (g << 8) | b;
}

void set_strip_color(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_COUNT; i++) {
    set_led_color(i, 255, 0, 0); // Set to red
  }
}

// int main() {
    
//     printf("Press Enter to turn off LEDs...\n");
//     getchar();
//     return 0;
// }
