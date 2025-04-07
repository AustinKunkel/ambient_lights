#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include "screen_capture_functions.h"

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480

int main() {
    struct timespec start, end;

    setup_capture(WIDTH, HEIGHT);
    unsigned char *rgb_buffer = (unsigned char *)malloc(WIDTH * HEIGHT * 3);
    if(!rgb_buffer) {
      perror("Failed to allocate rgb_buffer...");
      return 1;
    }

    // Capture loop
    while (1) {
      clock_gettime(CLOCK_MONOTONIC, &start);  // Start timer

      capture_frame(rgb_buffer);

      int r, g, b;
      // (0, 0)
      int index = 0;
      r = rgb_buffer[index];
      g = rgb_buffer[index + 1];
      b = rgb_buffer[index + 2];
      printf("Color at (0, 0):(%d, %d, %d)\t", r, g, b);

      int x = (int)(WIDTH / 2);
      int y = (int)(HEIGHT / 2);
      index = (y * WIDTH + x) * c;
      r = rgb_buffer[index];
      g = rgb_buffer[index + 1];
      b = rgb_buffer[index + 2];
      printf("Color at (WIDTH/2, HEIGHT/2):(%d, %d, %d)\t", r, g, b);

      x = WIDTH - 1;
      y = HEIGHT - 1;
      index = (y * WIDTH + x) * c;
      r = rgb_buffer[index];
      g = rgb_buffer[index + 1];
      b = rgb_buffer[index + 2];
      printf("Color at (WIDTH - 1, HEIGHT - 1):(%d, %d, %d)\t", r, g, b);

      clock_gettime(CLOCK_MONOTONIC, &end);  // End timer

      double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
      printf("time taken: %f\n", time_taken);
    }

    free(rgb_buffer);
    stop_video_capture();
    return 0;
}
