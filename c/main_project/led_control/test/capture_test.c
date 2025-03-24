#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>

#define DEVICE "/dev/video0"  // Change if needed

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd == -1) {
        perror("Failed to open capture device");
        return 1;
    }

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // Check your device format

    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("Failed to set format");
        close(fd);
        return 1;
    }

    // Start capture loop
    while (1) {
        unsigned char buffer[640 * 480 * 2];  // Adjust based on format
        int bytesRead = read(fd, buffer, sizeof(buffer));

        if (bytesRead < 0) {
            perror("Failed to read frame");
            break;
        }

        printf("Captured a frame of %d bytes\n", bytesRead);

        // Process the frame here (e.g., average color calculation)
    }

    close(fd);
    return 0;
}
