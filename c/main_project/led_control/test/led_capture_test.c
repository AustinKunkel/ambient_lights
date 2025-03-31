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
#include "led_test.h"
#include "ws2811.h"

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480
#define LED_COUNT 206  // Number of LEDs

volatile bool stop_capture = false;
pthread_t capture_thread;

void *capture_loop(ws2811_t);

struct Settings {
  int v_offset = 0;
  int h_offset = 0;
  int avg_color = 0;
  int left_count = 36;
  int top_count = 66;
  int right_count = 37;
  int bottom_count = 67;
  int res_x = 640;
  int res_y = 480;
  short blend_mode = 0;
  int blend_depth = 5;
} sc_settings;

/**
 * Uses the sc_settings variable and initailizes the struct based on json file
 * 
 * currently uses default values
 */
bool initialize_settings() {
  sc_settings.v_offset = 0;
  sc_settings.h_offset = 0;
  sc_settings.avg_color = 0;
  sc_settings.left_count = 36;
  sc_settings.top_count = 66;
  sc_settings.right_count = 37;
  sc_settings.bottom_count = 67;
  sc_settings.res_x = 640;
  sc_settings.res_y = 480;
  sc_settings.blend_depth = 5;
  sc_settings.blend_mode = 0;
}

char *start_capturing(ws2811_t strip) {
  if(initalize_settings()) {
    return "{\"Error\": \"Failed to initialize sc_settings\"}";
  }

  if(setup_strip()) {
    return "{\"Error\": \"Failed to initialize LED strip\"}";
  }

  stop_capture = false;
  if(pthread_create(&capture_thread, NULL, capture_loop, NULL) != 0) {
    cleanup_strip();
    return  "{\"Error\": \"Failed to create capture thread\"}";
  }


  return "{\"Success: \"Capturing started\"}";
}

void *capture_loop(ws2811_t strip) {
  while(!stop_capture) {
    int led_count = strip.channel[0].count;
    for(int i = 0; i < led_count; i++) {
      set_led_color(i, 255, 0, 0);
      sleep(.01)
    }
    for(int i = 0; i < led_count; i++) {
      set_led_color(i, 0, 0, 255);
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

void yuyv_to_rgb(unsigned char y, unsigned char u, unsigned char v, 
  unsigned char *r, unsigned char *g, unsigned char *b);

int setup() {

  
}