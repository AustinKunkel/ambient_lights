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
#include "server.h"

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
pthread_t send_positions_thread;

struct led_position *led_positions; // server.h

// Default Screen capture settings
CaptureSettings sc_settings = {
  .v_offset = 0,
  .h_offset = 0,
  .avg_color = 0,
  .left_count = 36,
  .right_count = 37,
  .top_count = 66,
  .bottom_count = 67,
  .res_x = 640,
  .res_y = 480,
  .blend_depth = 5,
  .blend_mode = 1,
  .auto_offset = 1,
  .transition_rate = .3
};

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

    sc_settings.auto_offset = atoi(next_token(&line_ptr));
    printf("Auto offset: %d\t", sc_settings.auto_offset);

    sc_settings.transition_rate = atof(next_token(&line_ptr));
    printf("Transition Rate: %.2f\n", sc_settings.transition_rate);

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
      uint8_t r = rgb_buffer[index];
      uint8_t g = rgb_buffer[index + 1];
      uint8_t b = rgb_buffer[index + 2];

      if (r > 10 || g > 10 || b > 10) {
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
   //printf("H Offest: %d\n", sc_settings.h_offset);
  }

  // top/bottom (same condition applies)
  bool row_is_not_black = false;
  middle = HEIGHT / 2;
  
  for (int j = middle; j >= 0; j--) {
    row_is_not_black = false;
  
    for (int i = 0; i < WIDTH; i++) {
      int index = (j * WIDTH + i) * 3;
      uint8_t r = rgb_buffer[index];
      uint8_t g = rgb_buffer[index + 1];
      uint8_t b = rgb_buffer[index + 2];

      if (r > 10 || g > 10 || b > 10) {
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
  
  }
  
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

  if(sc_settings.auto_offset) {
    if(auto_align_offsets()) {
      printf("Aligning offsets failed!\n");
      return 1;
    }
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

/**
 * Loop that takes the current led_positions and
 * sends them to the server to give to the client
 */
void *send_led_positions_loop(void *) {
  struct timespec ts;

  ts.tv_sec = 0;
  ts.tv_nsec = 33333333L;  // 33 milliseconds = 33,000,000 nanoseconds, 30fps

  while(!stop_capture) {
    send_led_strip_colors(led_positions);
    nanosleep(&ts, NULL);
  }

  return NULL;
}

void *capture_loop(void *);

int start_capturing(ws2811_t *strip) {
  if(!initialize_settings()) {
    printf("Failed to initialize sc_settings!\n");
    return 1;
  }

  // if(setup_strip(LED_COUNT)) {
  //   printf("Failed to initialize LED strip!\n");
  //   return 1;
  // } 
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

  printf("Creating send led positions loop...\n");
  if(pthread_create(&send_positions_thread, NULL, send_led_positions_loop, NULL) != 0) {
    stop_video_capture();
    printf("Failed to create send positions thread!");
    return 1;
  }

  printf("Capturing started...\n");
  return 0;
}

/**
 * "Blends" the 2 colors together by a factor
 */
uint32_t lerp_color(uint32_t from, uint32_t to, float alpha) {
  uint8_t r1 = (from >> 16) & 0xFF;
  uint8_t g1 = (from >> 8) & 0xFF;
  uint8_t b1 = from & 0xFF;

  uint8_t r2 = (to >> 16) & 0xFF;
  uint8_t g2 = (to >> 8) & 0xFF;
  uint8_t b2 = to & 0xFF;

  uint8_t r = (uint8_t)(r1 + alpha * (r2 - r1));
  uint8_t g = (uint8_t)(g1 + alpha * (g2 - g1));
  uint8_t b = (uint8_t)(b1 + alpha * (b2 - b1));

  return (r << 16) | (g << 8) | b;
}

void avg_color_loop(unsigned char *, ws2811_t *, int);
void reg_capture_loop(unsigned char *, ws2811_t *);

void *capture_loop(void *strip_ptr) {
  ws2811_t *strip = (ws2811_t *)strip_ptr;
  printf("Mallocing rgb buffer...\n");
  unsigned char *rgb_buffer = (unsigned char *)malloc(WIDTH * HEIGHT * 3);
  if(!rgb_buffer) {
    perror("Failed to allocate rgb_buffer...");
    return NULL;
  }

  if(sc_settings.avg_color) {
    int steps = 60; // number of loops for transition
    avg_color_loop(rgb_buffer, strip, steps);
  } else {
    reg_capture_loop(rgb_buffer, strip);
  }

  free(rgb_buffer);
  printf("Capture stopped...\n");
}

uint32_t blend_colors(struct led_position* led_list, unsigned char *rgb_buffer, int index, int depth) {
  int r_total = 0, g_total = 0, b_total = 0;
  int count = 0;
  for(int i = -depth; i <= depth + 1; i++) {
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

  struct led_position current_pixel = led_list[index];
  const int pixel_side = current_pixel.side;
  // read inwards from the edge
  if(pixel_side == TOP) {
    for(int j = 10; j <= depth; j += 10) { // increment by 10 (can be changed but for now it works)
      int new_y = current_pixel.y + j; // find the new y to read from the frame
      if(new_y >= HEIGHT) break; // if it hits the bottom, dont read further

      int check_index = (new_y * WIDTH + current_pixel.x) * 3; // formula to read any pixel (x, y) from the frame
      r_total += rgb_buffer[check_index];
      g_total += rgb_buffer[check_index + 1];
      b_total += rgb_buffer[check_index + 2];
      count++;
    }
  } else if(pixel_side == BOTTOM) { // same applies from aboves
    for(int j = 10; j <= depth; j += 10) {
      int new_y = current_pixel.y - j;
      if(new_y < 0) break; // if we hit the top

      int check_index = (new_y * WIDTH + current_pixel.x) * 3;
      r_total += rgb_buffer[check_index];
      g_total += rgb_buffer[check_index + 1];
      b_total += rgb_buffer[check_index + 2];
      count++;
    }
  } else if(pixel_side == LEFT) {
    for(int j = 10; j <= depth; j += 10) {
      int new_x = current_pixel.x + j;
      if(new_x >= WIDTH) break;

      int check_index = (current_pixel.y * WIDTH + new_x) * 3;
      r_total += rgb_buffer[check_index];
      g_total += rgb_buffer[check_index + 1];
      b_total += rgb_buffer[check_index + 2];
      count++;
    }
  } else { // right
    for(int j = 10; j <= depth; j += 10) {
      int new_x = current_pixel.x - j;
      if(new_x < 0) break;

      int check_index = (current_pixel.y * WIDTH + new_x) * 3;
      r_total += rgb_buffer[check_index];
      g_total += rgb_buffer[check_index + 1];
      b_total += rgb_buffer[check_index + 2];
      count++;
    }
  }

  if(count == 0) {
    return 0;
  }

  return ((int)(r_total / count) << 16) | ((int)(g_total / count) << 8) | (int)(b_total / count);
}

/**
 * Returns the average color in the rgb_buffer array.
 * @param rgb_buffer The buffer of rgb colors passed in from the frame
 * @param width The width of the frame
 * @param height The height of the frame
 * @param skip_amt The step of pixels to skip when calculating the average
 */
uint32_t calculate_frame_average(unsigned char* rgb_buffer, int width, int height, int skip_amt) {
  if (!rgb_buffer || width <= 0 || height <= 0) return 0;
  
  uint32_t r_sum = 0, g_sum = 0, b_sum = 0;
  int pixel_count = 0;
  int step = skip_amt + 1;
  
  // Process in blocks of 4 pixels (manual loop unrolling)
  for(int y = 0; y < height; y += step) {
    unsigned char* row = &rgb_buffer[y * width * 3];
    int x;
    for(x = 0; x < width - 3 * step; x += 4 * step) {
      int idx = x * 3;
      // Process 4 pixels at once
      r_sum += row[idx] + row[idx + step*3] + row[idx + step*6] + row[idx + step*9];
      g_sum += row[idx+1] + row[idx + step*3+1] + row[idx + step*6+1] + row[idx + step*9+1];
      b_sum += row[idx+2] + row[idx + step*3+2] + row[idx + step*6+2] + row[idx + step*9+2];
      pixel_count += 4;
    }
    // Handle remaining pixels
    for(; x < width; x += step) {
      int idx = x * 3;
      r_sum += row[idx];
      g_sum += row[idx + 1];
      b_sum += row[idx + 2];
      pixel_count++;
    }
  }
  
  if(pixel_count == 0) return 0;
  
  uint8_t r_avg = r_sum / pixel_count;
  uint8_t g_avg = g_sum / pixel_count;
  uint8_t b_avg = b_sum / pixel_count;
  
  return (r_avg << 16) | (g_avg << 8) | (b_avg);
}

void avg_color_loop(unsigned char *rgb_buffer, ws2811_t *strip, int steps) {
  int skip_amt = 15;
  //struct timespec ts = {0, 5 * 1000000L}; // 5ms timer
  uint32_t cur_color = (uint32_t)strip->channel[0].leds[0];
 
  while(!stop_capture) {
    capture_frame(rgb_buffer);
    uint32_t avg_color = calculate_frame_average(rgb_buffer, WIDTH, HEIGHT, skip_amt);
    
    // get the avg color's r g b values
    uint8_t target_r = (avg_color >> 16) & 0xFF;
    uint8_t target_g = (avg_color >> 8) & 0xFF;
    uint8_t target_b = avg_color & 0xFF;
    
    // grab the current color's r g b values
    uint8_t cur_r = (cur_color >> 16) & 0xFF;
    uint8_t cur_g = (cur_color >> 8) & 0xFF;
    uint8_t cur_b = cur_color & 0xFF;
    
    // Use signed differences
    int16_t diff_r = target_r - cur_r;
    int16_t diff_g = target_g - cur_g;
    int16_t diff_b = target_b - cur_b;
    
    for(int i = 0; i < steps; i++) {
      // Calculate position (0.0 to 1.0) and apply to difference
      cur_r = ((cur_color >> 16) & 0xFF) + (diff_r * (i + 1)) / steps;
      cur_g = ((cur_color >> 8) & 0xFF) + (diff_g * (i + 1)) / steps;
      cur_b = (cur_color & 0xFF) + (diff_b * (i + 1)) / steps;
      
      set_strip_color(cur_r, cur_g, cur_b);
      ws2811_render(strip);
      //nanosleep(&ts, NULL);
    }
    
    cur_color = (target_r << 16) | (target_g << 8) | target_b;
  }
}

void reg_capture_loop(unsigned char *rgb_buffer, ws2811_t *strip) {
  //int LED_COUNT = sc_settings.top_count + sc_settings.right_count + sc_settings.bottom_count + sc_settings.left_count;
  bool blend_mode_active = sc_settings.blend_mode > 0;
  int i = 0;
  float alpha = sc_settings.transition_rate + 0.001f; // higher provides faster transitions than lower
  while(!stop_capture) {
    // printf("Capturing frame...\n");
    capture_frame(rgb_buffer);
    if(blend_mode_active) {
      for(int i = 0; i < LED_COUNT; i++) {
        int index = (led_positions[i].y * WIDTH + led_positions[i].x) * 3;
        uint32_t target_color = blend_colors(led_positions, rgb_buffer,i, sc_settings.blend_depth);
        uint32_t smoothed_color = lerp_color(led_positions[i].color, target_color, alpha);

        set_led_32int_color(i, smoothed_color);
        led_positions[i].color = smoothed_color;
        led_positions[i].valid = 1;
      }
    } else {
      for(int i = 0; i < LED_COUNT; i++) {
        int index = (led_positions[i].y * WIDTH + led_positions[i].x) * 3;
        int r = rgb_buffer[index], g = rgb_buffer[index + 1], b = rgb_buffer[index + 2];
        set_led_color(i, r, g, b);
        led_positions[i].color = (r << 16) | (g << 8) | b;
        led_positions[i].valid = 1;
      }
    }
    ws2811_render(strip);
  }
}

int stop_capturing() {
  stop_capture = true;  // Signal thread to stop
  if(pthread_join(capture_thread, NULL)) { // Wait for thread to finish
    printf("Failed to join capture thread!\n");
    return 1;
  } 
  if(pthread_join(send_positions_thread, NULL)) {
    printf("Failed to join send positions thread!\n");
    return 1;
  }
  free(led_positions);
  stop_video_capture();
  printf("Capture thread joined.\n");
  return 0;
}
