#ifndef LED_CAPTURE_TEST_H
#define LED_CAPTURE_TEST_H

int setup_capture(int, int);
int stop_video_capture();
void capture_frame(unsigned char*);
#endif