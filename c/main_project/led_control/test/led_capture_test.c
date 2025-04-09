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
#include "screen_capture_functions.h"

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480
int LED_COUNT = 206;  // Number of LEDs

volatile bool stop_capture = false;
bool blend_mode_active = true;
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

void setup_left_side(int, struct led_position*, int, int, int, int);
void setup_right_side(int, struct led_position*, int, int, int, int, int);
void setup_top_side(int, struct led_position*, int, int, int, int);
void setup_bottom_side(int, struct led_position*, int, int, int, int, int);


/**
 * Sets up the LEDs with screen capture.
 */
int setup_strip_capture(ws2811_t *strip) {
  int LED_COUNT = strip->channel[0].count;
  printf("Mallocing led positions...\n");
  led_positions = malloc(sizeof(struct led_position) * LED_COUNT);
  if (led_positions == NULL) {
    printf("Memory allocation failed!\n");
    return 1;
  }

  int next_index = 0;
  printf("Setting up right side...\n");
  setup_right_side(sc_settings.right_count, led_positions, WIDTH, HEIGHT, 0, next_index, -1);
  next_index += sc_settings.right_count;
  printf("Setting up top side...\n");
  setup_top_side(sc_settings.top_count, led_positions, WIDTH, 0, next_index, -1);
  next_index += sc_settings.top_count;
  printf("Setting up left side...\n");
  setup_left_side(sc_settings.left_count, led_positions, HEIGHT, 0, next_index, -1);
  next_index += sc_settings.left_count;
  printf("Setting up bottom side...\n");
  setup_bottom_side(sc_settings.bottom_count, led_positions, WIDTH, HEIGHT, 0, next_index, -1);

  return 0;
}

void setup_left_side(int count, struct led_position* led_list, int h, 
                     int h_offset, int next_index, int fwd_multiplier) {
  
  float spacing = (h - 1) / (count - 1);

  next_index += count;
  int start = count;
  int stop = 0;
  int x_index = 1 + h_offset;
  for(int i = start; i > stop; i--) {
    int y_index = i * spacing;
    if(y_index >= h) y_index = h - 1;
    int index = (next_index) + ((count - i) + 1)*fwd_multiplier;

    led_list[index].x = x_index;
    led_list[index].y = y_index;
  }
}

void setup_right_side(int count, struct led_position* led_list, int w, int h, 
                      int h_offset, int next_index, int fwd_multiplier) {
  
  if(count < 2) return;
  float spacing = (h - 1) / (count - 1);
  next_index += count;
  int start = 0;
  int stop = count;
  int x_index = (w - 1) - h_offset;
  for(int i = start; i < stop; i++) {
    int y_index = (int)(i * spacing);
    if (y_index >= h) y_index = h - 1;

    int index = next_index + (i + 1)*fwd_multiplier;

    led_list[index].x = x_index;
    led_list[index].y = y_index;
  }
}

void setup_top_side(int count, struct led_position* led_list, int w, 
                    int v_offset, int next_index, int fwd_multiplier) {
  float spacing = (w - 1) / (count - 1);
  next_index += count;
  int start = 0;
  int stop = count;
  int y_index = 1 + v_offset;
  for(int i = start; i < stop; i++) {
    int x_index = i * spacing;
    if(x_index >= w) x_index = w - 1;

    int index = next_index + (i + 1)*fwd_multiplier;

    led_list[index].x = x_index;
    led_list[index].y = y_index;
  }
}

void setup_bottom_side(int count, struct led_position* led_list, int w, int h, int v_offset, int next_index, int fwd_multiplier) {
  float spacing = (w - 1) / (count - 1);
  next_index += count;
  int start = count;
  int stop = 0;
  int y_index = (h - 1) - v_offset;
  for(int i = start; i > stop; i--) {
    int x_index = i * spacing;
    if(x_index >= w) x_index = w - 1;
    int index = next_index + ((count - i) + 1)*fwd_multiplier;

    led_list[index].x = x_index;
    led_list[index].y = y_index;
  }
}

char *start_capturing(ws2811_t *strip) {
  if(!initialize_settings()) {
    return "{\"Error\": \"Failed to initialize sc_settings\"}";
  }

  if(setup_strip()) {
    return "{\"Error\": \"Failed to initialize LED strip\"}";
  } 
  printf("Setting up strip capture\n");
  if(setup_strip_capture(&ledstring)) {
    return "{\"Error\": \"Failed to set up strip screen capture\"}";
  }
  printf("Setting up capture...\n");
  if(setup_capture(sc_settings.res_x, sc_settings.res_y)) {
    free(led_positions);
    return "{\"Error\": \"Failed to set up screen capture\"}";
  }
  printf("Creating capture loop thread...\n");
  stop_capture = false;
  if(pthread_create(&capture_thread, NULL, capture_loop, (void *)strip) != 0) {
    cleanup_strip();
    free(led_positions);
    stop_video_capture();
    return  "{\"Error\": \"Failed to create capture thread\"}";
  }

  return "{\"Success\": \"Capturing started\"}";
}

uint32_t blend_colors(struct led_position*, unsigned char*, int, int);

void *capture_loop(void *strip_ptr) {
  ws2811_t *strip = (ws2811_t *)strip_ptr;
  printf("Mallocing rgb buffer...\n");
  unsigned char *rgb_buffer = (unsigned char *)malloc(WIDTH * HEIGHT * 3);
  if(!rgb_buffer) {
    perror("Failed to allocate rgb_buffer...");
    return NULL;
  }
  //int LED_COUNT = sc_settings.top_count + sc_settings.right_count + sc_settings.bottom_count + sc_settings.left_count;

  while(!stop_capture) {
    // printf("Capturing frame...\n");
    capture_frame(rgb_buffer);
    if(blend_mode_active) {
      for(int i = 0; i < LED_COUNT; i++) {
        int index = (led_positions[i].y * WIDTH + led_positions[i].x) * 3;
        uint32_t color = blend_colors(led_positions, rgb_buffer, i, 10);
        set_led_32int_color(i, color);
      }
    } else {
      for(int i = 0; i < LED_COUNT; i++) {
        int index = (led_positions[i].y * WIDTH + led_positions[i].x) * 3;
        int r = rgb_buffer[index], g = rgb_buffer[index + 1], b = rgb_buffer[index + 2];
        set_led_color(i, r, g, b);
      }
    }
    ws2811_render(strip);
  }
  // while(!stop_capture) {
  //   int led_count = strip->channel[0].count;
  //   for(int i = 0; i < led_count; i++) {
  //     set_led_color(i, 0, 255, 0);
  //     ws2811_render(strip);
  //     sleep(.01);
  //   }
  //   for(int i = 0; i < led_count; i++) {
  //     set_led_color(i, 0, 0, 255);
  //     ws2811_render(strip);
  //     sleep(.01);
  //   }
  // }
  free(rgb_buffer);
  cleanup_strip();
  printf("Capture stopped...\n");
}

uint32_t blend_colors(struct led_position* led_list, unsigned char *rgb_buffer, int index, int depth) {
  int r_total = 0, g_total = 0, b_total = 0;
  int count = 0;
  for(int i = -depth; i < depth + 1; i++) {
    int check_index = (index + i) % LED_COUNT;
    printf("check index: %d\t", check_index);
    struct led_position check_pixel_location = led_list[check_index];

    int buffer_index = (check_pixel_location.y * WIDTH + check_pixel_location.x) * 3;
    // printf("check index:%d, buffer index: %d, count: %d\t", check_index, buffer_index, count);
    r_total += rgb_buffer[buffer_index];
    g_total += rgb_buffer[buffer_index + 1];
    b_total += rgb_buffer[buffer_index + 2];
    count++;
  }

  if(count == 0) {
    return 0;
  }

  return ((int)(r_total / count) << 16) | ((int)(g_total / count) << 8) | (int)(b_total / count);
}

char *stop_capturing() {
  stop_capture = true;  // Signal thread to stop
  if(pthread_join(capture_thread, NULL)) { // Wait for thread to finish
    return "{\"Error\": \"Failed to join capture thread\"}";
  } 
  free(led_positions);
  stop_video_capture();
  printf("Capture thread joined.\n");
  return "{\"Success: \"Capture Thread stopped\"}";
}
