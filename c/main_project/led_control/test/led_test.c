#include <stdio.h>
#include <stdlib.h>
#include "ws2811.h"

#define LED_COUNT 206  // Number of LEDs
#define GPIO_PIN 18   // GPIO pin for PWM output

void set_led_color(int, uint8_t, uint8_t, uint8_t);
void set_strip_color(uint8_t, uint8_t, uint8_t);
void set_brightness(int);


ledstring = {
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

int setup_strip() 
{
  if (ws2811_init(&ledstring) != WS2811_SUCCESS) 
  {
    printf("Failed to initialize LEDs!\n");
    return 1;
  }
  return 0;
}

void cleanup_strip()
{
  ws2811_fini(&ledstring);
}

char *turn_led_off_test()
{
  set_brightness(0);
  ws2811_render(&ledstring);  // Apply changes
  return "{\"message\": \"Turned LEDs off\"}";
}


char *led_test()
{
  set_brightness(125);
  set_strip_color(255, 0, 0); // set strip color to red

  ws2811_render(&ledstring);

  return "{\"message\": \"Turned on LEDs\"}";
}


void set_led_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    ledstring.channel[0].leds[index] = (r << 16) | (g << 8) | b;
}

void set_strip_color(uint8_t r, uint8_t g, uint8_t b)
{
  for (int i = 0; i < LED_COUNT; i++) {
    set_led_color(i, 255, 0, 0); // Set to red
  }
}

void set_brightness(int brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;  // Max brightness is 255

  ledstring.channel[0].brightness = brightness;
}

// int main() {
    
//     printf("Press Enter to turn off LEDs...\n");
//     getchar();
//     return 0;
// }
