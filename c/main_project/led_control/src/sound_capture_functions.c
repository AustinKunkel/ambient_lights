#include "sound_capture_functions.h"
#include <alsa/asoundlib.h>

snd_pcm_t *capture_handle;
snd_pcm_hw_params_t *hw_params;

#define DEVICE "hw:0,0"  // Default capture device

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