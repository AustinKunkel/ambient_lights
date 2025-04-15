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
#include "led_functions.h"
#include "screen_capture.h"
#include "screen_capture_functions.h"
#include "csv_control.h"

#define DEVICE "/dev/video0"
#define LEFT 2
#define RIGHT 0
#define TOP 1
#define BOTTOM 3
#define SC_SETTINGS_FILENAME "led_control/data/sc_settings.csv"
int LED_COUNT = 206;  // Number of LEDs
int WIDTH = 640;
int HEIGHT = 480;

volatile bool stop_capture = false;
pthread_t capture_thread;

void *capture_loop(void *);

struct led_position {
  int x;
  int y;
  int side; // 0 for right, 1 for top, 2 for left, 3 for bottom
};

struct led_position *led_positions;

CaptureSettings sc_settings;

/**
 * Uses the sc_settings variable and initailizes the struct based on csv file
 * 
 * currently uses default values
 */
bool initialize_settings() {
  char data_line[512];
  printf("reading sc_settings.csv...\n");
  if(read_one_line(SC_SETTINGS_FILENAME, data_line, sizeof(data_line)) == 0)
  {
    char *line_ptr = data_line;
    printf("Setting sc settings variables...\n");
    sc_settings.v_offset = atoi(next_token(&line_ptr));
    printf("v_offset: %d\t", sc_settings.v_offset);
    
    sc_settings.h_offset = atoi(next_token(&line_ptr));
    printf("h_offset: %d\t", sc_settings.h_offset);
    
    sc_settings.avg_color = atoi(next_token(&line_ptr));
    printf("avg_color: %d\t", sc_settings.avg_color);
    
    sc_settings.left_count = atoi(next_token(&line_ptr));
    printf("left_count: %d\t", sc_settings.left_count);
    
    sc_settings.right_count = atoi(next_token(&line_ptr));
    printf("right_count: %d\t", sc_settings.right_count);
    
    sc_settings.top_count = atoi(next_token(&line_ptr));
    printf("top_count: %d\t", sc_settings.top_count);
    
    sc_settings.bottom_count = atoi(next_token(&line_ptr));
    printf("bottom_count: %d\t", sc_settings.bottom_count);
    
    sc_settings.res_x = atoi(next_token(&line_ptr));
    printf("res_x: %d\t", sc_settings.res_x);
    
    sc_settings.res_y = atoi(next_token(&line_ptr));
    printf("res_y: %d\t", sc_settings.res_y);
    
    sc_settings.blend_depth = atoi(next_token(&line_ptr));
    printf("blend_depth: %d\t", sc_settings.blend_depth);
    
    sc_settings.blend_mode = atoi(next_token(&line_ptr));
    printf("blend_mode: %d\t", sc_settings.blend_mode);    

    WIDTH = sc_settings.res_x;
    HEIGHT = sc_settings.res_y;

    return true;
  } else {
    perror("Unable to read from sc_settings.csv!!\n");
    return false;
  }
}

void setup_left_side(int, struct led_position*, int, int, int, int);
void setup_right_side(int, struct led_position*, int, int, int, int, int);
void setup_top_side(int, struct led_position*, int, int, int, int);
void setup_bottom_side(int, struct led_position*, int, int, int, int, int);

/**
 * automatically aligns offsets to point to the correct position
 * (avoids the black bars) 
 */
int auto_align_offsets() {
  unsigned char *rgb_buffer = (unsigned char *)malloc(WIDTH * HEIGHT * 3);
  if(!rgb_buffer) {
    perror("Failed to allocate rgb_buffer for offsets...");
    return 1;
  }
  capture_frame(rgb_buffer);

  // left/right (assuming they're the same because I didnt set up a left and right offset)
  bool not_black = false;
  int middle = WIDTH / 2; // x middle
  sc_settings.h_offset = middle;
  for(int i = middle; i >= 0; i--) { // x
    not_black = false;
    for(int j = 0; j < HEIGHT; j++) { // y
      //index = (y * WIDTH + x) * 3;
      int index = (j * WIDTH + i) * 3;
      uint32_t color = rgb_buffer[index] << 16 | rgb_buffer[index + 1] << 8 | rgb_buffer[index + 2];
      if(color > 0) {
        not_black = true;
        break;
      }
    }
    if(!not_black) { // the current column is all black (we want to read farther in)
      sc_settings.h_offset += 2;
      break;
    }
    sc_settings.h_offset--;

    if(sc_settings.h_offset < 0) {
      sc_settings.h_offset = 0;
      break;
    }
   printf("H Offest: %d\n", sc_settings.h_offset);
  }

  // top/bottom (same condition applies)
  bool row_is_not_black = false;
  middle = HEIGHT / 2;
  
  for (int j = middle; j >= 0; j--) {
    row_is_not_black = false;
  
    for (int i = 0; i < WIDTH; i++) {
      int index = (j * WIDTH + i) * 3;
      uint32_t color = rgb_buffer[index] << 16 | rgb_buffer[index + 1] << 8 | rgb_buffer[index + 2];
      if(color > 0) {
        row_is_not_black = true;
        break;
      }
    }
  
    if (!row_is_not_black) {
      // This row is all black. Set the offset to the row just below it.
      sc_settings.v_offset = j + 1;
      break;
    }
  
    // If we reach the top without finding a black row:
    if (j == 0) {
      sc_settings.v_offset = 0;
    }
  
    printf("Checking row: %d\n", j);
  }
  printf("Final V Offset: %d\n", sc_settings.v_offset);
  
  free(rgb_buffer);
  return 0;
}


/**
 * Sets up the LEDs with screen capture.
 */
int setup_strip_capture(ws2811_t *strip) {
  LED_COUNT = led_settings.count;
  printf("Mallocing led positions...\n");
  led_positions = malloc(sizeof(struct led_position) * LED_COUNT);
  if (led_positions == NULL) {
    printf("Memory allocation failed!\n");
    return 1;
  }

  if(auto_align_offsets()) {
    printf("Aligning offsets failed!\n");
    return 1;
  }

  int next_index = 0;
  printf("Setting up right side...\n");
  setup_right_side(sc_settings.right_count, led_positions, WIDTH, HEIGHT, sc_settings.h_offset, next_index, -1);
  next_index += sc_settings.right_count;
  printf("Setting up top side...\n");
  setup_top_side(sc_settings.top_count, led_positions, WIDTH, sc_settings.v_offset, next_index, -1);
  next_index += sc_settings.top_count;
  printf("Setting up left side...\n");
  setup_left_side(sc_settings.left_count, led_positions, HEIGHT, sc_settings.h_offset, next_index, -1);
  next_index += sc_settings.left_count;
  printf("Setting up bottom side...\n");
  setup_bottom_side(sc_settings.bottom_count, led_positions, WIDTH, HEIGHT, sc_settings.v_offset, next_index, -1);

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
    led_list[index].side = LEFT;
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
    led_list[index].side = RIGHT;
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
    led_list[index].side = TOP;
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
    led_list[index].side = BOTTOM;
  }
}

void *capture_loop(void *);

int start_capturing(ws2811_t *strip) {
  if(!initialize_settings()) {
    printf("Failed to initialize sc_settings!\n");
    return 1;
  }

  if(setup_strip(LED_COUNT)) {
    printf("Failed to initialize LED strip!\n");
    return 1;
  } 
  printf("Setting up capture...\n");
  if(setup_capture(sc_settings.res_x, sc_settings.res_y)) {

    printf("Failed to set up screen capture!\n");
    return 1;
  }

  printf("Setting up strip capture\n");
  if(setup_strip_capture(strip)) {
    printf("Failed to set up strip screen capture!\n");
    return 1;
  }

  printf("Creating capture loop thread...\n");
  stop_capture = false;
  //printf("LED Count: %d\n", strip->channel[0].count);
  if(pthread_create(&capture_thread, NULL, capture_loop, (void *)strip) != 0) {
    free(led_positions);
    stop_video_capture();
    printf("Failed to create capture thread!\n"); 
    return 1;
  }

  printf("Capturing started...\n");
  return 0;
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
  bool blend_mode_active = sc_settings.blend_mode > 0;
  while(!stop_capture) {
    // printf("Capturing frame...\n");
    capture_frame(rgb_buffer);
    if(blend_mode_active) {
      for(int i = 0; i < LED_COUNT; i++) {
        int index = (led_positions[i].y * WIDTH + led_positions[i].x) * 3;
        uint32_t color = blend_colors(led_positions, rgb_buffer, i, 5);
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
  printf("Capture stopped...\n");
}

uint32_t blend_colors(struct led_position* led_list, unsigned char *rgb_buffer, int index, int depth) {
  int r_total = 0, g_total = 0, b_total = 0;
  int count = 0;
  for(int i = -depth; i < depth + 1; i++) {
    int check_index = (index + i + LED_COUNT) % LED_COUNT;
    // printf("led index: %d, check index: %d\t", index, check_index);
    struct led_position check_pixel_location = led_list[check_index];

    int buffer_index = (check_pixel_location.y * WIDTH + check_pixel_location.x) * 3;
    // printf("check index:%d, buffer index: %d, count: %d\t", check_index, buffer_index, count);
    if (buffer_index < 0 || buffer_index + 2 >= WIDTH * HEIGHT * 3) {
      continue; // or break, depending on your use-case
    }
    r_total += rgb_buffer[buffer_index];
    g_total += rgb_buffer[buffer_index + 1];
    b_total += rgb_buffer[buffer_index + 2];
    count++;
  }
  for(int j = 10; j <= depth; j += 10) { // read inwards
    int check_index; // index in the rgb buffer we want to check
    struct led_position current_pixel = led_list[index];
    int pixel_side = current_pixel.side;
    if(pixel_side == TOP) {
      int new_y = current_pixel.y + j;
      if(new_y >= HEIGHT) break;

      check_index = (new_y * WIDTH + current_pixel.x) * 3;
    } else if(pixel_side == BOTTOM) {
      int new_y = current_pixel.y - j;
      if(new_y < 0) break;

      check_index = (new_y * WIDTH + current_pixel.x) * 3;
    } else if(pixel_side == LEFT) {
      int new_x = current_pixel.x + j;
      if(new_x >= WIDTH) break;

      check_index = (current_pixel.y * WIDTH + new_x) * 3;
    } else { // right
      int new_x = current_pixel.x - j;
      if(new_x < 0) break;

      check_index = (current_pixel.y * WIDTH + new_x) * 3;
    }
    r_total += rgb_buffer[check_index];
    g_total += rgb_buffer[check_index + 1];
    b_total += rgb_buffer[check_index + 2];
    count++;
  }

  if(count == 0) {
    return 0;
  }

  return ((int)(r_total / count) << 16) | ((int)(g_total / count) << 8) | (int)(b_total / count);
}

int stop_capturing() {
  stop_capture = true;  // Signal thread to stop
  if(pthread_join(capture_thread, NULL)) { // Wait for thread to finish
    printf("Failed to join capture thread!\n");
    return 1;
  } 
  free(led_positions);
  stop_video_capture();
  printf("Capture thread joined.\n");
  return 0;
}
