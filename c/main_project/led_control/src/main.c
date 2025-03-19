#include <stdio.h>
#include <stdlib.h>
#include "ws2811.h"

#define LED_COUNT 3  // Number of LEDs
#define GPIO_PIN 12   // GPIO pin for PWM output

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

void set_led_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    ledstring.channel[0].leds[index] = (r << 16) | (g << 8) | b;
}

int main() {
    if (ws2811_init(&ledstring) != WS2811_SUCCESS) {
        printf("Failed to initialize LEDs!\n");
        return 1;
    }

    for (int i = 0; i < LED_COUNT; i++) {
        set_led_color(i, 255, 0, 0); // Set to red
    }

    ws2811_render(&ledstring);
    
    printf("Press Enter to turn off LEDs...\n");
    getchar();

    ws2811_fini(&ledstring);
    return 0;
}
