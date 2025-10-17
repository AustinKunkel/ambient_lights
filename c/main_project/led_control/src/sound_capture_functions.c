#include "sound_capture_functions.h"
#include <alsa/asoundlib.h>
#include <math.h>
#include <stdlib.h>

snd_pcm_t *capture_handle;
snd_pcm_hw_params_t *hw_params;

#define DEVICE          "plughw:CARD=Video,DEV=0"  // Default capture device


void setup_audio_capture(unsigned int sample_rate, unsigned int channels) {
    int err;

    // open default capture device
    if ((err = snd_pcm_open(&capture_handle, DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "cannot open audio device (%s)\n", snd_strerror(err));
        return;
    }

    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(capture_handle, hw_params);

    snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &sample_rate, 0);
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels);

    snd_pcm_hw_params(capture_handle, hw_params);
    snd_pcm_prepare(capture_handle);
}

/**
 * 
 * @param buffer Pre-allocated buffer to hold audio samples
 * @param frame_size size of the frame (ex: 256 for 512 samples)
 * @param should_skip_loop set to true if read error or incorrect frame
 */
void capture_audio_frame(int16_t *buffer, int frame_size, int *should_skip_loop) {
    int err;

    // Read exactly frame_size frames; will block until available
    err = snd_pcm_readi(capture_handle, buffer, frame_size);

    if (err == -EPIPE) {
        // Buffer overrun
        fprintf(stderr, "Buffer overrun occurred\n");
        snd_pcm_prepare(capture_handle); // recover from overrun
        *should_skip_loop = 1;
        return;
    } else if (err < 0) {
        // Other read error
        fprintf(stderr, "Read from audio interface failed (%s)\n", snd_strerror(err));
        snd_pcm_prepare(capture_handle);
        *should_skip_loop = 1;
        return;
    } else if (err != frame_size) {
        // Partial read (rare in blocking mode)
        fprintf(stderr, "Short read: expected %d frames, got %d\n", frame_size, err);
        *should_skip_loop = 1;
        return;
    }

    // Success
    *should_skip_loop = 0;
    printf("Captured %d frames\n", err);
}

void cleanup_audio() {
    snd_pcm_close(capture_handle);
    snd_pcm_hw_params_free(hw_params);
}


/***** Sound Processing Related Functions *****/

/**
 * Handles Normalization, DC Offset filtering, Hann window, and RMS computation
*/
float compute_rms(const int16_t *buffer, int frame_size, float *dc, const float *window) {
    const float dc_beta = 0.001f;
    float sum_squares = 0.0f;

    for (int i = 0; i < frame_size; i++) {
        float s = buffer[i] / 32768.0f;
        *dc += dc_beta * (s - *dc);
        float s_ac = s - *dc;
        float x = s_ac * window[i];
        sum_squares += x * x;
    }

    float rms = sqrtf(sum_squares / frame_size);
    if (isnan(rms) || isinf(rms)) rms = 0.0f;
    return rms;
}

/**
 * Simple low pass filter
 */
void smooth_audio_frame(float *samples, int frame_size, float alpha) {
    // alpha: 0.05 - 0.3 (smaller = more smoothing)
    for (int i = 1; i < frame_size; i++) {
        samples[i] = alpha * samples[i] + (1.0f - alpha) * samples[i - 1];
    }
}

/**
 * A raised cosine filter
 */
void raised_cosine_filter(float *coeffs, int N) {
    for (int n = 0; n < N; n++) {
        coeffs[n] = 0.5f * (1.0f - cosf((2.0f * M_PI * n) / (N - 1)));
    }

    // Normalize coefficients so sum = 1
    float sum = 0.0f;
    for (int n = 0; n < N; n++) sum += coeffs[n];
    for (int n = 0; n < N; n++) coeffs[n] /= sum;
}

/**
 * Apply the filter to the samples (simple convolution)
 */
void apply_filter(const float *input, float *output, int frame_size, const float *coeffs, int N) {
    for (int i = 0; i < frame_size; i++) {
        float acc = 0.0f;
        for (int k = 0; k < N; k++) {
            int j = i - k;
            if (j >= 0) acc += coeffs[k] * input[j];
        }
        output[i] = acc;
    }
}


