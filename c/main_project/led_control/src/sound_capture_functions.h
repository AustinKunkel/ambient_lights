#pragma once

#include <stdint.h>

#define FILTER_TAPS     64  //  32-128 Higher=smoother but slower
#define ALPHA           0.2f // smoothing strength (.05 - .3)

void setup_audio_capture(unsigned int sample_rate, unsigned int channels);
void capture_audio_frame(int16_t *buffer, int frame_size, int *should_skip_loop);
void cleanup_audio();

// Sound-processing related functions
float compute_rms(const int16_t *buffer, int frame_size, float *dc, const float *window);
void smooth_audio_frame(float *samples, int frame_size, float alpha);
void raised_cosine_filter(float *coeffs, int N);
void apply_filter(const float *input, float *output, int frame_size, const float *coeffs, int N);
