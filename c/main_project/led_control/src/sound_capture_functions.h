#pragma once

#include <stdint.h>

void setup_audio_capture(unsigned int sample_rate, unsigned int channels);
void capture_audio_frame(int16_t *buffer, int frame_size);
void cleanup_audio();