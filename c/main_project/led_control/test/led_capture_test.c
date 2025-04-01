#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include "led_test.h"
#include "ws2811.h"

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480
#define LED_COUNT 206  // Number of LEDs

volatile bool stop_capture = false;
pthread_t capture_thread;

void *capture_loop(void *);

struct Settings {
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
};

struct led_position {
  int x;
  int y;
};

struct led_position *led_positions;

struct Settings sc_settings;

/**
 * Uses the sc_settings variable and initailizes the struct based on json file
 * 
 * currently uses default values
 */
bool initialize_settings() {
  sc_settings = (struct Settings){
    .h_offset = 0,
    .avg_color = 0,
    .left_count = 36,
    .top_count = 66,
    .right_count = 37,
    .bottom_count = 67,
    .res_x = 640,
    .res_y = 480,
    .blend_depth = 5,
    .blend_mode = 0
  };

  return true;
}

/**
 * Sets up the LEDs with screen capture.
 */
int setup_capture(ws2811_t *strip) {
  int led_count = strip->channel[0].count;
  led_positions = malloc(sizeof(struct led_position) * led_count);

  if (led_positions == NULL) {
    printf("Memory allocation failed!\n");
    return 1;
  }
  return 0;
}

char *start_capturing(ws2811_t *strip) {
  if(!initialize_settings()) {
    return "{\"Error\": \"Failed to initialize sc_settings\"}";
  }

  if(setup_strip()) {
    return "{\"Error\": \"Failed to initialize LED strip\"}";
  }

  if(setup_capture()) {
    return "{\"Error\": \"Failed to set up screen capture\"}";
  }

  stop_capture = false;
  if(pthread_create(&capture_thread, NULL, capture_loop, (void *)strip) != 0) {
    cleanup_strip();
    return  "{\"Error\": \"Failed to create capture thread\"}";
  }


  return "{\"Success: \"Capturing started\"}";
}

void *capture_loop(void *strip_ptr) {
  ws2811_t *strip = (ws2811_t *)strip_ptr;
  while(!stop_capture) {
    int led_count = strip->channel[0].count;
    for(int i = 0; i < led_count; i++) {
      set_led_color(i, 0, 255, 0);
      ws2811_render(strip);
      sleep(.01);
    }
    for(int i = 0; i < led_count; i++) {
      set_led_color(i, 0, 0, 255);
      ws2811_render(strip);
      sleep(.01);
    }
  }
  cleanup_strip();
  printf("Capture stopped...\n");
  return NULL;
}

char *stop_capturing() {
  stop_capture = true;  // Signal thread to stop

  if(pthread_join(capture_thread, NULL)) { // Wait for thread to finish
    return "{\"Error\": \"Failed to join capture thread\"}";
  } 
  printf("Capture thread joined.\n");
  return "{\"Success: \"Capture Thread stopped\"}";
}
