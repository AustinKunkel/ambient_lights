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

        printf("Captured a frame of %d bytes\n", buf.bytesused);

        // Process the frame (stored in `buffer`)

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
