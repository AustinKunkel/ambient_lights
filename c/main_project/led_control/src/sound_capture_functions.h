#pragma once

#include <stdint.h>

void setup_audio_capture(unsigned int sample_rate, unsigned int channels);
void capture_audio_frame(int16_t *buffer, int frame_size, int *should_skip_loop);
void cleanup_audio();