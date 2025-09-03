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
    snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_FLOAT_LE);
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
void capture_audio_frame(float *buffer, int frame_size) {
    int err;
    if ((err = snd_pcm_readi(capture_handle, buffer, frame_size)) != frame_size) {
        fprintf(stderr, "read from audio interface failed (%s)\n", snd_strerror(err));
        snd_pcm_prepare(capture_handle); // recover from overrun
    }
}

void cleanup_audio() {
    snd_pcm_close(capture_handle);
    snd_pcm_hw_params_free(hw_params);
}