#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include "led_test.h"

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480
#define LED_COUNT 206  // Number of LEDs


void yuyv_to_rgb(unsigned char y, unsigned char u, unsigned char v, 
  unsigned char *r, unsigned char *g, unsigned char *b);