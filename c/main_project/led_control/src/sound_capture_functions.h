#pragma once

void setup_audio_capture(unsigned int sample_rate, unsigned int channels);
void capture_audio_frame(float *buffer, int frame_size);
void cleanup_audio();