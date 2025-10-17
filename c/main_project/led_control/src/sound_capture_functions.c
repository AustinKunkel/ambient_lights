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
    //printf("Captured %d frames\n", err);
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

// Simple iterative radix-2 FFT (Cooley-Tukey) - not optimized, adequate for small frames
void fft_inplace(float *real, float *imag, int n) {
    int i, j, k, m;
    // bit reversal
    j = 0;
    for (i = 1; i < n - 1; i++) {
        m = n >> 1;
        while (j & m) {
            j ^= m;
            m >>= 1;
        }
        j |= m;
        if (i < j) {
            float tr = real[i]; real[i] = real[j]; real[j] = tr;
            float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }

    for (int len = 2; len <= n; len <<= 1) {
        float theta = -2.0f * M_PI / len;
        float wlen_r = cosf(theta);
        float wlen_i = sinf(theta);
        for (i = 0; i < n; i += len) {
            float wr = 1.0f;
            float wi = 0.0f;
            for (j = 0; j < (len >> 1); j++) {
                int u = i + j;
                int v = i + j + (len >> 1);
                float xr = real[u];
                float xi = imag[u];
                float yr = wr * real[v] - wi * imag[v];
                float yi = wr * imag[v] + wi * real[v];

                real[u] = xr + yr;
                imag[u] = xi + yi;
                real[v] = xr - yr;
                imag[v] = xi - yi;

                float tmp_wr = wr * wlen_r - wi * wlen_i;
                wi = wr * wlen_i + wi * wlen_r;
                wr = tmp_wr;
            }
        }
    }
}

// helper to convert frequency (Hz) to mel
static float hz_to_mel(float hz) {
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}

static float mel_to_hz(float mel) {
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

// Create triangular mel filterbank
float *create_mel_filterbank(int n_filters, int n_bins, int fft_size, float sample_rate) {
    int n_fft_bins = fft_size / 2 + 1;
    if (n_bins != n_fft_bins) return NULL;

    float nyquist = sample_rate / 2.0f;
    float mel_min = hz_to_mel(0.0f);
    float mel_max = hz_to_mel(nyquist);

    float *mel_points = malloc(sizeof(float) * (n_filters + 2));
    for (int i = 0; i < n_filters + 2; i++) {
        mel_points[i] = mel_to_hz(mel_min + (mel_max - mel_min) * i / (n_filters + 1));
    }

    float *filters = calloc(n_filters * n_bins, sizeof(float));
    for (int f = 0; f < n_filters; f++) {
        float f_m_minus = mel_points[f];
        float f_m = mel_points[f + 1];
        float f_m_plus = mel_points[f + 2];
        for (int k = 0; k < n_bins; k++) {
            float freq = (float)k * sample_rate / fft_size;
            float weight = 0.0f;
            if (freq >= f_m_minus && freq <= f_m) {
                weight = (freq - f_m_minus) / (f_m - f_m_minus);
            } else if (freq >= f_m && freq <= f_m_plus) {
                weight = (f_m_plus - freq) / (f_m_plus - f_m);
            }
            filters[f * n_bins + k] = weight;
        }
        // normalize filter so sum = 1 to make energies comparable between bands
        float sum = 0.0f;
        for (int k = 0; k < n_bins; k++) sum += filters[f * n_bins + k];
        if (sum > 0.0f) {
            for (int k = 0; k < n_bins; k++) filters[f * n_bins + k] /= sum;
        }
    }

    free(mel_points);
    return filters;
}

void free_mel_filterbank(float *filters) {
    if (filters) free(filters);
}

// compute mel band energies given real-valued FFT bins (magnitude)
void compute_mel_energies(const float *mag_bins, int n_bins, const float *filters, int n_filters, float *out_energies) {
    for (int f = 0; f < n_filters; f++) {
        float sum = 0.0f;
        const float *fil = &filters[f * n_bins];
        for (int k = 0; k < n_bins; k++) {
            float m = mag_bins[k];
            sum += fil[k] * (m * m); // use power (mag^2)
        }
        out_energies[f] = sum;
    }
}


