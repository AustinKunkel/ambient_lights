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
#define NUM_BUFFERS 4

struct capture_device {
  int device_id; // the id of the **open** device
  void *buffers[NUM_BUFFERS]; // Pointers to the buffers
  size_t buffer_lengths[NUM_BUFFERS]; // Length of each buffer
  struct v4l2_buffer buf[NUM_BUFFERS]; // Buffer info for each buffer
};

struct capture_device dev; 
int WIDTH = 640;
int HEIGHT = 480;

int setup_capture(int width, int height) {

  // Open device
  dev.device_id = open(DEVICE, O_RDWR);
  if(dev.device_id == -1) {
    perror("Failed to open capture device");
    return 1;
  }

  WIDTH = width;
  HEIGHT = height;

  // Configure device format
  struct v4l2_format format;
  memset(&format, 0, sizeof(format));
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = WIDTH;
  format.fmt.pix.height = HEIGHT;
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

  // Apply video format
  if (ioctl(dev.device_id, VIDIOC_S_FMT, &format) == -1) {
    perror("Failed to set format");
    close(dev.device_id);
    return 1;
  }

  // Request Buffer allocation
  struct v4l2_requestbuffers req;
  memset(&req, 0, sizeof(req));
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl(dev.device_id, VIDIOC_REQBUFS, &req) == -1) {
    perror("Failed to request buffer");
    close(dev.device_id);
    return 1;
  } 

  // Query and mmap buffers
  for (int i = 0; i < NUM_BUFFERS; i++) {
    memset(&dev.buf[i], 0, sizeof(dev.buf[i]));
    dev.buf[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dev.buf[i].memory = V4L2_MEMORY_MMAP;
    dev.buf[i].index = i;

    if (ioctl(dev.device_id, VIDIOC_QUERYBUF, &dev.buf[i]) == -1) {
      perror("Failed to query buffer");
      close(dev.device_id);
      return 1;
    }

    // Map the buffer
    dev.buffers[i] = mmap(NULL, dev.buf[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, dev.device_id, dev.buf[i].m.offset);
    if (dev.buffers[i] == MAP_FAILED) {
      perror("Failed to mmap buffer");
      close(dev.device_id);
      return 1;
    }

    dev.buffer_lengths[i] = dev.buf[i].length;
  }

  // Queue buffers
  for (int i = 0; i < NUM_BUFFERS; i++) {
    if (ioctl(dev.device_id, VIDIOC_QBUF, &dev.buf[i]) == -1) {
        perror("Failed to queue buffer");
        close(dev.device_id);
        return 1;
    }
  }

  // Start streaming
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(dev.device_id, VIDIOC_STREAMON, &type) == -1) {
    perror("Failed to start streaming");
    close(dev.device_id);
    return 1;
  }

  return 0;  // Success
}

void yuyv_to_rgb(unsigned char *yuv_buffer,unsigned char *rgb_buffer, size_t frame_size) {
  int x;
  int z=0;
  unsigned char *ptr = rgb_buffer;
  unsigned char *yuyv= yuv_buffer;
  for (x = 0; x < frame_size; x++)
  {
    int r, g, b;
    int y, u, v;
 
    if (!z)
      y = yuyv[0] << 8;
    else
      y = yuyv[2] << 8;
    u = yuyv[1] - 128;
    v = yuyv[3] - 128;
 
    r = (y + (359 * v)) >> 8;
    g = (y - (88 * u) - (183 * v)) >> 8;
    b = (y + (454 * u)) >> 8;
 
    *(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
    *(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
    *(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);
 
    if(z++)
    {
      z = 0;
      yuyv += 4;
    }
  }
}

/**
 * Writes to a 1D array of unsigned chars PRE-ALLOCATE!!
 * unsigned char *rgb_buffer = (unsigned char *)malloc(WIDTH * HEIGHT * 3);
 * Each subsequent row is "appended" onto the last
 * 
 * Formula for finding any index (x, y) format
 * index = (y * WIDTH + x) * 3;
 * r = buffer[index]
 * g = buffer[index + 1]
 * b = buffer[index + 2]
 */
void capture_frame(unsigned char *rgb_buffer) {
  if(!rgb_buffer) {
    perror("RGB buffer pointer is NULL...");
    return;
  }
  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (buf.index >= NUM_BUFFERS) {
    fprintf(stderr, "Invalid buffer index: %d\n", buf.index);
    return;
  }
  
  // Dequeue a buffer (retrieve a frame)
  if (ioctl(dev.device_id, VIDIOC_DQBUF, &buf) == -1) {
      perror("Failed to dequeue buffer");
      return;
  }

  // Process the frame data (dev.buffers[buf.index] contains the frame)
  void *frame_data = dev.buffers[buf.index];
  size_t frame_size = buf.bytesused;

  yuyv_to_rgb(frame_data, rgb_buffer, frame_size);

  // Requeue the buffer for future use
  if (ioctl(dev.device_id, VIDIOC_QBUF, &buf) == -1) {
      perror("Failed to requeue buffer");
      return;
  }
}

int stop_video_capture() {
  // Stop streaming
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(dev.device_id, VIDIOC_STREAMOFF, &type) == -1) {
      perror("Failed to stop streaming");
      return -1;
  }

  // Unmap buffers
  for (int i = 0; i < NUM_BUFFERS; i++) {
      munmap(dev.buffers[i], dev.buffer_lengths[i]);
  }

  // Close the device
  close(dev.device_id);
  return 0;  // Success
}
