#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>

#define DEVICE "/dev/video0"
#define WIDTH  640
#define HEIGHT 480

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd == -1) {
        perror("Failed to open capture device");
        return 1;
    }

    // Set video format
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = WIDTH;
    format.fmt.pix.height = HEIGHT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // Adjust format if needed

    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("Failed to set format");
        close(fd);
        return 1;
    }

    // Request buffer
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("Failed to request buffer");
        close(fd);
        return 1;
    }

    // Query buffer
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        perror("Failed to query buffer");
        close(fd);
        return 1;
    }

    // Map buffer
    void *buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED) {
        perror("Failed to mmap buffer");
        close(fd);
        return 1;
    }

    // Queue buffer
    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        perror("Failed to queue buffer");
        close(fd);
        return 1;
    }

    // Start streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("Failed to start streaming");
        close(fd);
        return 1;
    }

    // Capture loop
    while (1) {
        // Dequeue buffer (waits for frame)
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("Failed to dequeue buffer");
            break;
        }

        // printf("Captured a frame of %d bytes\n", buf.bytesused);

        // Process the frame (stored in `buffer`)
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

        // Requeue the buffer for next frame
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Failed to requeue buffer");
            break;
        }
    }

    // Cleanup
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    munmap(buffer, buf.length);
    close(fd);
    return 0;
}
