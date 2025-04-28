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
#define CLAMP(x) ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))

struct capture_device {
  int device_id; // the id of the **open** device
  void *buffers[NUM_BUFFERS]; // Pointers to the buffers
  size_t buffer_lengths[NUM_BUFFERS]; // Length of each buffer
  struct v4l2_buffer buf[NUM_BUFFERS]; // Buffer info for each buffer
};

struct capture_device dev; 

int setup_capture(int width, int height) {

  // Open device
  dev.device_id = open(DEVICE, O_RDWR);
  if(dev.device_id == -1) {
    perror("Failed to open capture device");
    return 1;
  }


  // Configure device format
  struct v4l2_format format;
  memset(&format, 0, sizeof(format));
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = width;
  format.fmt.pix.height = height;
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
  unsigned char *ptr = rgb_buffer;

  for (size_t x = 0; x < frame_size; x += 4) {
    float y0 = (float)(yuv_buffer[x + 0] - 16) * 255.0f / 219.0f;
    float u  = (float)(yuv_buffer[x + 1] - 128);
    float y1 = (float)(yuv_buffer[x + 2] - 16) * 255.0f / 219.0f;
    float v  = (float)(yuv_buffer[x + 3] - 128);

    int r, g, b;

    // First pixel
    float c = y0 < 0 ? 0 : y0;
    r = (int)((298.082 * c + 408.583 * v) / 256.0);
    g = (int)((298.082 * c - 100.291 * u - 208.120 * v) / 256.0);
    b = (int)((298.082 * c + 516.412 * u) / 256.0);
    *(ptr++) = CLAMP(r);
    *(ptr++) = CLAMP(g);
    *(ptr++) = CLAMP(b);

    // Second pixel
    c = y1 < 0 ? 0 : y1;
    r = (int)((298.082 * c + 408.583 * v) / 256.0);
    g = (int)((298.082 * c - 100.291 * u - 208.120 * v) / 256.0);
    b = (int)((298.082 * c + 516.412 * u) / 256.0);
    *(ptr++) = CLAMP(r);
    *(ptr++) = CLAMP(g);
    *(ptr++) = CLAMP(b);
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

  // if(frame_size == (WIDTH * HEIGHT * 2)) {
  //   printf("Frame size matches expected");
  // }

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
