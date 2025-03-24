#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>

#define DEVICE "/dev/video0"

void yuyv_to_rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char *r, unsigned char *g, unsigned char *b) {
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;

    int r_temp = (298 * c + 409 * e + 128) >> 8;
    int g_temp = (298 * c - 100 * d - 208 * e + 128) >> 8;
    int b_temp = (298 * c + 516 * d + 128) >> 8;

    *r = (r_temp < 0) ? 0 : (r_temp > 255) ? 255 : r_temp;
    *g = (g_temp < 0) ? 0 : (g_temp > 255) ? 255 : g_temp;
    *b = (b_temp < 0) ? 0 : (b_temp > 255) ? 255 : b_temp;
}

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Cannot open device");
        return 1;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("Setting pixel format failed");
        close(fd);
        return 1;
    }
    while(1)
    {
      struct v4l2_requestbuffers req;
      memset(&req, 0, sizeof(req));
      req.count = 1;
      req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      req.memory = V4L2_MEMORY_MMAP;

      if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
          perror("Requesting buffer failed");
          close(fd);
          return 1;
      }

      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(buf));
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = 0;

      if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
          perror("Querying buffer failed");
          close(fd);
          return 1;
      }

      void *buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
      if (buffer == MAP_FAILED) {
          perror("Memory mapping failed");
          close(fd);
          return 1;
      }

      if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
          perror("Queue buffer failed");
          close(fd);
          return 1;
      }

      int type = buf.type;
      if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
          perror("Start capture failed");
          close(fd);
          return 1;
      }

      if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
          perror("Retrieving frame failed");
          close(fd);
          return 1;
      }

      // Compute average color
      unsigned char *frame_data = (unsigned char *)buffer;
      long long total_r = 0, total_g = 0, total_b = 0;
      int pixel_count = (fmt.fmt.pix.width * fmt.fmt.pix.height);

      for (int i = 0; i < buf.bytesused; i += 4) {
          unsigned char y1 = frame_data[i];
          unsigned char u = frame_data[i + 1];
          unsigned char y2 = frame_data[i + 2];
          unsigned char v = frame_data[i + 3];

          unsigned char r1, g1, b1, r2, g2, b2;
          yuyv_to_rgb(y1, u, v, &r1, &g1, &b1);
          yuyv_to_rgb(y2, u, v, &r2, &g2, &b2);

          total_r += r1 + r2;
          total_g += g1 + g2;
          total_b += b1 + b2;
      }

      unsigned char avg_r = total_r / pixel_count;
      unsigned char avg_g = total_g / pixel_count;
      unsigned char avg_b = total_b / pixel_count;

      printf("Average Color: R=%d, G=%d, B=%d\n", avg_r, avg_g, avg_b);

    }

    ioctl(fd, VIDIOC_STREAMOFF, &type);
    munmap(buffer, buf.length);
    close(fd);

    return 0;
}
