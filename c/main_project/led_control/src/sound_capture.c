#include "led_functions.h"
#include "sound_capture.h"
#include "sound_capture_functions.h"
#include "csv_control.h"
#include "server.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define SOUND_EFFECTS_FILENAME  "led_control/data/sound_effects.csv"
#define FRAME_SIZE              512
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif

volatile bool stop_sound_capture = false;

pthread_t sound_capture_thread;
pthread_t send_led_colors_thread;
pthread_t audio_capture_thread;
pthread_t audio_processing_thread;
pthread_t renderer_thread;

// Simple blocking ring buffer for audio frames (producer: capture, consumer: processing)
static int16_t *rb_data = NULL; // contiguous storage: capacity * FRAME_SIZE
static int rb_capacity = 0; // number of frames
static int rb_frame_size = 0;
static int rb_head = 0;
static int rb_tail = 0;
static int rb_count = 0;
static pthread_mutex_t rb_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t rb_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t rb_not_full = PTHREAD_COND_INITIALIZER;

// Strip lock to protect writes/reads to ws2811_t led buffer
static pthread_mutex_t strip_mutex = PTHREAD_MUTEX_INITIALIZER;

static ws2811_t *g_strip_ptr = NULL;
static struct sound_effect *g_effect_ptr = NULL;

static float window[FRAME_SIZE];

// Double-buffer for LED colors (RGB floats per LED)
static float *led_buf_front = NULL; // size: led_count * 3
static float *led_buf_back = NULL;
static int led_buf_count = 0;

// Forward declarations for functions defined later (avoid implicit declarations)
static int rb_init(int capacity, int frame_size);
static void rb_free(void);
static void *audio_capture_thread_fn(void *arg);
static void *audio_processing_thread_fn(void *arg);
static void *renderer_thread_fn(void *arg);
void process_brightness_on_volume_frame(struct sound_effect *effect, ws2811_t *strip, const int16_t *frame, const float *window, float *dc);
void process_melbank_frame(struct sound_effect *effect, ws2811_t *strip, const int16_t *frame, const float *window, float *dc);
float smooth_brightness_decay(float current_brightness, float target_brightness, float rise_alpha, float fall_alpha);
void apply_brightness_ratios_to_leds(ws2811_t *strip, int start, int end, float brightness);

typedef struct sound_effect sound_effect;
typedef void (*effect_frame_fn)(struct sound_effect *effect, ws2811_t *strip, const int16_t *frame, const float *window, float *dc);

typedef void (*effect_fn)(struct sound_effect *effect, ws2811_t *strip);

struct sound_effect {
  char name[50];
  float sensitivity; // multiplier for volume
  int min_freq; // minimum frequency to react to
  int max_freq; // maximum frequency to react to
  int led_start; // starting led index
  int led_end; // ending led index
  int effect_num; // The number for the effect
  effect_fn apply_effect; // legacy function pointer (not used by frame-based pipeline)
  effect_frame_fn process_frame; // called per audio frame by processing thread
  /* per-effect runtime state */
  float dc; // DC estimate for this effect
  float brightness_smooth; // smoothed brightness state
  float smoothed_rms_sq;
  float rms_alpha; // smoothing weight computed from tau
  // mel filter data (optional)
  float *mel_filters; // n_mel_filters * n_bins
  int n_mel_filters;
  float *mel_energies; // n_mel_filters
};

sound_effect *sound_effects;
struct led_position *led_colors; // server.h
int sound_effect_count = 0;

void assign_effect_function(sound_effect *effect) {
  // Map effect numbers to processors. apply_effect remains a lightweight
  // initializer (non-blocking) for legacy compatibility. The per-frame
  // processing function is stored in process_frame and will be called by
  // the audio processing thread for each captured frame.
  switch(effect->effect_num) {
    case 1:
      // melbank effect (default)
      effect->apply_effect = NULL; // legacy initializer removed
      effect->process_frame = process_melbank_frame;
      break;
    case 2:
      // classic brightness-on-volume per-frame processor
      effect->apply_effect = NULL;
      effect->process_frame = process_brightness_on_volume_frame;
      break;
    default:
      // fallback to melbank
      effect->apply_effect = NULL;
      effect->process_frame = process_melbank_frame;
  }
}

bool initialize_sound_effects() {
  FILE *fp = fopen(SOUND_EFFECTS_FILENAME, "r");
  if (!fp) {
    perror("Unable to open sound_effects.csv");
    return false;
  }

  char line[512];
  sound_effect_count = 0;

  // Skip header
  if (!fgets(line, sizeof(line), fp)) {
    fclose(fp);
    return false;
  }

  // First, count the number of effects
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] != '\n' && line[0] != '\0') sound_effect_count++;
  }

  if (sound_effect_count == 0) {
    fclose(fp);
    return false;
  }

  // Allocate array
  sound_effects = (sound_effect *)malloc(sizeof(sound_effect) * sound_effect_count);
  if (!sound_effects) {
    perror("Failed to allocate memory for sound effects");
    fclose(fp);
    return false;
  }

  // Rewind and skip header again
  rewind(fp);
  fgets(line, sizeof(line), fp);

  int idx = 0;
  while (fgets(line, sizeof(line), fp) && idx < sound_effect_count) {
    printf("Parsing line: %s", line);
    char *line_ptr = line;
    char *token;

    token = next_token(&line_ptr);
    snprintf(sound_effects[idx].name, sizeof(sound_effects[idx].name), "%s", token ? token : "");

    token = next_token(&line_ptr);
    sound_effects[idx].sensitivity = token ? atof(token) : 1.0;

    token = next_token(&line_ptr);
    sound_effects[idx].min_freq = token ? atoi(token) : 20;

    token = next_token(&line_ptr);
    sound_effects[idx].max_freq = token ? atoi(token) : 20000;

    token = next_token(&line_ptr);
    sound_effects[idx].led_start = token ? atoi(token) : 0;

    token = next_token(&line_ptr);
    sound_effects[idx].led_end = token ? atoi(token) : 0;

    token = next_token(&line_ptr);
    sound_effects[idx].effect_num = token ? atoi(token) : 1;

    assign_effect_function(&sound_effects[idx]);

    // compute dt = frame_duration:
    float sample_rate = 48000.0f;        // must match the rate in setup_audio_capture()
    float dt = (float)FRAME_SIZE / sample_rate; // frame duration in seconds
    float tau = 0.05f;                   // desired time constant (50 ms). Tweak 0.03 - 0.1
    float alpha = 1.0f - expf(-dt / tau);

    // clamp alpha just in case
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    sound_effects[idx].rms_alpha = alpha;
    sound_effects[idx].smoothed_rms_sq = 0.0f;
    sound_effects[idx].dc = 0.0f;
    sound_effects[idx].brightness_smooth = 0.0f;

    idx++;

    // Print each token for debugging
    printf("  name: %s\n", sound_effects[idx-1].name);
    printf("  sensitivity: %f\n", sound_effects[idx-1].sensitivity);
    printf("  min_freq: %d\n", sound_effects[idx-1].min_freq);
    printf("  max_freq: %d\n", sound_effects[idx-1].max_freq);
    printf("  led_start: %d\n", sound_effects[idx-1].led_start);
    printf("  led_end: %d\n", sound_effects[idx-1].led_end);
    printf("  effect_num: %d\n", sound_effects[idx-1].effect_num);
  }

  fclose(fp);
  printf("Loaded %d sound effects.\n", sound_effect_count);
  return true;
}

/**
 * Loop that takes the current led_colors and
 * sends them to the server to give to the client
 */
void *send_led_colors_loop(void *) {
  struct timespec ts;

  ts.tv_sec = 0;
  ts.tv_nsec = 33333333L;  // 33 milliseconds = 33,000,000 nanoseconds, 30fps

  while(!stop_sound_capture) {
    send_led_strip_colors(led_colors);
    nanosleep(&ts, NULL);
  }

  return NULL;
}

struct sound_effect_arg {
  sound_effect *effect;
  ws2811_t *strip;
};

/**
 * Function to call the effect's function pointer
 */
void *sound_effect_thread_func(void *arg) {
  struct sound_effect_arg *effect_arg = (struct sound_effect_arg *)arg;

  /* per-effect runtime state */
  struct sound_effect *effect = effect_arg->effect;
  ws2811_t *strip = effect_arg->strip;

  // Non-blocking initializer for effects. Previously this called a
  // legacy `apply_effect` function which could block; that behavior has
  // been removed. Do lightweight runtime state initialization here.
  printf("Applying effect (initializer): %s\n", effect->name);
  effect->dc = 0.0f;
  effect->brightness_smooth = 0.0f;
  effect->smoothed_rms_sq = 0.0f;

  free(arg);
  return NULL;
}

int start_sound_capture(ws2811_t *strip, int effect_index) {

  if(!initialize_sound_effects()) {
    printf("Failed to initialize sound effects!\n");
    return 1;
  }

  if(effect_index < 0 || effect_index >= sound_effect_count) {
    printf("Invalid effect index %d, defaulting to 0\n", effect_index);
    effect_index = 0;
  }

  int led_count = led_settings.count;
  printf("Mallocing led positions for %d LEDs...\n", led_count);
  led_colors = malloc(sizeof(struct led_position) * led_count);
  if (led_colors == NULL) {
    printf("Memory allocation failed!\n");
    free(sound_effects);
    return 1;
  }

  // Initialize the led_colors to the colors already in the strip at that position:
  for (int i = 0; i < led_count; i++) {
    led_colors[i].base_color = strip->channel[0].leds[i];
    led_colors[i].color = led_colors[i].base_color;
    led_colors[i].valid = true;
  }

  sound_effect effect = sound_effects[effect_index];
  if(effect.led_end >= led_count) {
    effect.led_end = led_count - 1;
  }
  printf("Effect: %s, LEDs: %d to %d\n", effect.name, effect.led_start, effect.led_end);

  struct sound_effect_arg *arg = malloc(sizeof(struct sound_effect_arg));
  if(!arg) {
    printf("Failed to allocate memory for sound effect argument!\n");
    free(sound_effects);
    free(led_colors);
    return 1;
  }
  arg->effect = &sound_effects[effect_index];
  arg->strip = strip;

  printf("Creating capture loop thread...\n");
  stop_sound_capture = false;
  // free the temporary arg; we'll use global pointers for threads
  free(arg);

  // Set globals for processing threads
  g_strip_ptr = strip;
  g_effect_ptr = &sound_effects[effect_index];

  // Precompute global Hann window once
  for (int i = 0; i < FRAME_SIZE; i++) {
    window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FRAME_SIZE - 1)));
  }

  // If the selected effect wants per-LED frequency mapping, create mel filters
  int led_range = sound_effects[effect_index].led_end - sound_effects[effect_index].led_start + 1;
  int n_mel = led_range;
  if (n_mel < 4) n_mel = 4;
  if (n_mel > 64) n_mel = 64; // cap for performance
  sound_effects[effect_index].n_mel_filters = n_mel;
  int n_bins = FRAME_SIZE / 2 + 1;
  sound_effects[effect_index].mel_filters = create_mel_filterbank(n_mel, n_bins, FRAME_SIZE, 48000.0f);
  if (sound_effects[effect_index].mel_filters) {
    sound_effects[effect_index].mel_energies = calloc(n_mel, sizeof(float));
  } else {
    sound_effects[effect_index].n_mel_filters = 0;
    sound_effects[effect_index].mel_energies = NULL;
  }

  // initialize ring buffer: capacity 16 frames (tunable)
  if (rb_init(16, FRAME_SIZE) != 0) {
    printf("Failed to init audio ring buffer\n");
    free(sound_effects);
    free(led_colors);
    return 1;
  }

  // allocate LED double-buffers
  led_buf_count = led_count;
  led_buf_front = calloc(led_buf_count * 3, sizeof(float));
  led_buf_back = calloc(led_buf_count * 3, sizeof(float));
  if (!led_buf_front || !led_buf_back) {
    printf("Failed to allocate LED buffers\n");
    rb_free();
    free(sound_effects);
    free(led_colors);
    return 1;
  }

  // start threads: capture, processing, renderer, and send_led_colors
  if (pthread_create(&audio_capture_thread, NULL, audio_capture_thread_fn, NULL) != 0) {
    printf("Failed to create audio capture thread\n");
    rb_free();
    free(sound_effects);
    free(led_colors);
    return 1;
  }

  if (pthread_create(&audio_processing_thread, NULL, audio_processing_thread_fn, (void *)strip) != 0) {
    printf("Failed to create audio processing thread\n");
    stop_sound_capturing();
    return 1;
  }

  if (pthread_create(&renderer_thread, NULL, renderer_thread_fn, (void *)strip) != 0) {
    printf("Failed to create renderer thread\n");
    stop_sound_capturing();
    return 1;
  }

  if(pthread_create(&send_led_colors_thread, NULL, send_led_colors_loop, NULL) != 0) {
    stop_sound_capturing();
    printf("Failed to create send positions thread!\n");
    return 1;
  }

  return 0;
  // Keep going: brightness_on_volume_effect will create the needed audio threads

}

// ---- Ring buffer helpers ----
static int rb_init(int capacity, int frame_size) {
  rb_capacity = capacity;
  rb_frame_size = frame_size;
  rb_head = rb_tail = rb_count = 0;
  rb_data = malloc(sizeof(int16_t) * capacity * frame_size);
  if (!rb_data) return -1;
  return 0;
}

static void rb_free() {
  if (rb_data) free(rb_data);
  rb_data = NULL;
  rb_capacity = rb_frame_size = rb_head = rb_tail = rb_count = 0;
}

// push blocks if full
static void rb_push(const int16_t *frame) {
  pthread_mutex_lock(&rb_mutex);
  while (rb_count == rb_capacity && !stop_sound_capture) {
    pthread_cond_wait(&rb_not_full, &rb_mutex);
  }
  if (stop_sound_capture) {
    pthread_mutex_unlock(&rb_mutex);
    return;
  }
  int base = rb_tail * rb_frame_size;
  memcpy(&rb_data[base], frame, sizeof(int16_t) * rb_frame_size);
  rb_tail = (rb_tail + 1) % rb_capacity;
  rb_count++;
  pthread_cond_signal(&rb_not_empty);
  pthread_mutex_unlock(&rb_mutex);
}

// pop blocks if empty; returns 0 on success, -1 if stopping
static int rb_pop(int16_t *out_frame) {
  pthread_mutex_lock(&rb_mutex);
  while (rb_count == 0 && !stop_sound_capture) {
    pthread_cond_wait(&rb_not_empty, &rb_mutex);
  }
  if (rb_count == 0 && stop_sound_capture) {
    pthread_mutex_unlock(&rb_mutex);
    return -1;
  }
  int base = rb_head * rb_frame_size;
  memcpy(out_frame, &rb_data[base], sizeof(int16_t) * rb_frame_size);
  rb_head = (rb_head + 1) % rb_capacity;
  rb_count--;
  pthread_cond_signal(&rb_not_full);
  pthread_mutex_unlock(&rb_mutex);
  return 0;
}

// ---- Audio capture thread ----
static void *audio_capture_thread_fn(void *arg) {
  (void)arg;
  // Setup audio here so capture happens inside this thread
  setup_audio_capture(48000, 1);

  int16_t local_frame[FRAME_SIZE];
  int skip = 0;
  while (!stop_sound_capture) {
    capture_audio_frame(local_frame, FRAME_SIZE, &skip);
    if (skip) continue;
    rb_push(local_frame);
  }

  return NULL;
}

// ---- Audio processing thread ----
static void *audio_processing_thread_fn(void *arg) {
  ws2811_t *strip_local = (ws2811_t *)arg;
  int16_t local_frame[FRAME_SIZE];
  // use per-effect state (dc, brightness_smooth) stored in g_effect_ptr
  if (g_effect_ptr) {
    g_effect_ptr->dc = 0.0f;
    g_effect_ptr->brightness_smooth = 0.0f;
  }

  // Precompute Hann window once
  float window[FRAME_SIZE];
  for (int i = 0; i < FRAME_SIZE; i++) window[i] = window[i]; // use global precomputed window

  while (!stop_sound_capture) {
    if (rb_pop(local_frame) != 0) break; // stopping

    // If effect provides a per-frame processor, call it.
    if (g_effect_ptr && g_effect_ptr->process_frame) {
      g_effect_ptr->process_frame(g_effect_ptr, strip_local, local_frame, window, &g_effect_ptr->dc);
    } else if (g_effect_ptr && g_effect_ptr->n_mel_filters > 0 && g_effect_ptr->mel_filters) {
      // Compute FFT magnitude bins
      int n = FRAME_SIZE;
      int n_bins = n / 2 + 1;
      float real[n];
      float imag[n];
      for (int i = 0; i < n; i++) {
        real[i] = local_frame[i] / 32768.0f * window[i];
        imag[i] = 0.0f;
      }
      fft_inplace(real, imag, n);
      // magnitude bins (only 0..n/2)
      float mag[n_bins];
      for (int k = 0; k < n_bins; k++) mag[k] = sqrtf(real[k]*real[k] + imag[k]*imag[k]);

      // compute mel energies
      compute_mel_energies(mag, n_bins, g_effect_ptr->mel_filters, g_effect_ptr->n_mel_filters, g_effect_ptr->mel_energies);

      // smooth mel energies and map to LEDs
      for (int f = 0; f < g_effect_ptr->n_mel_filters; f++) {
        float e = g_effect_ptr->mel_energies[f];
        // simple EMA smoothing per band (reuse rms_alpha)
        g_effect_ptr->mel_energies[f] = g_effect_ptr->rms_alpha * e + (1.0f - g_effect_ptr->rms_alpha) * g_effect_ptr->mel_energies[f];
        // map to LED index
        int led_idx = g_effect_ptr->led_start + f * (g_effect_ptr->led_end - g_effect_ptr->led_start) / (g_effect_ptr->n_mel_filters - 1);
        if (led_idx < g_effect_ptr->led_start) led_idx = g_effect_ptr->led_start;
        if (led_idx > g_effect_ptr->led_end) led_idx = g_effect_ptr->led_end;
        float norm = g_effect_ptr->mel_energies[f];
        // normalize loosely (user tweakable) and clamp
        float brightness = fminf(norm * g_effect_ptr->sensitivity * 10.0f, 1.0f);
        pthread_mutex_lock(&strip_mutex);
        apply_brightness_ratios_to_leds(strip_local, led_idx, led_idx, brightness);
        pthread_mutex_unlock(&strip_mutex);
      }

    } else {
      // Fallback: basic brightness-on-volume processing
      float rms = compute_rms(local_frame, FRAME_SIZE, &g_effect_ptr->dc, window);
      float brightness = fminf(rms * g_effect_ptr->sensitivity, 1.0f);
      if (brightness < 0.0f) brightness = 0.0f;
      g_effect_ptr->brightness_smooth = smooth_brightness_decay(g_effect_ptr->brightness_smooth, brightness, 0.1f, 0.04f);
      pthread_mutex_lock(&strip_mutex);
      apply_brightness_ratios_to_leds(strip_local, g_effect_ptr->led_start, g_effect_ptr->led_end, g_effect_ptr->brightness_smooth);
      pthread_mutex_unlock(&strip_mutex);
    }
  }

  return NULL;
}

// ---- Renderer thread: calls ws2811_render at fixed FPS ----
static void *renderer_thread_fn(void *arg) {
  ws2811_t *strip_local = (ws2811_t *)arg;
  const long frame_ns = 16666667L; // ~60Hz
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = frame_ns;

  while (!stop_sound_capture) {
    pthread_mutex_lock(&strip_mutex);
    // strip data (led_colors) already updated by processing thread under strip_mutex
    ws2811_render(strip_local);
    pthread_mutex_unlock(&strip_mutex);
    nanosleep(&ts, NULL);
  }
  return NULL;
}

/**
 * Applys different smoothing for rising or falling brightness levels
 */
float smooth_brightness_decay(float current_brightness, float target_brightness, float rise_alpha, float fall_alpha) {
    if (target_brightness > current_brightness) {
        // Rising quickly
        return rise_alpha * target_brightness + (1.0f - rise_alpha) * current_brightness;
    } else {
        // Falling slowly
        return fall_alpha * target_brightness + (1.0f - fall_alpha) * current_brightness;
    }
}

void apply_brightness_ratios_to_leds(ws2811_t *strip, int start, int end, float brightness) {
    int value = (int)(brightness * 255.0f);
    for (int i = start; i <= end; i++) {
        uint32_t base = led_colors[i].base_color;
        uint8_t r = ((base >> 16) & 0xFF) * brightness;
        uint8_t g = ((base >> 8) & 0xFF) * brightness;
        uint8_t b = (base & 0xFF) * brightness;
        set_led_color(i, r, g, b);
        led_colors[i].color = (r << 16) | (g << 8) | b;
        led_colors[i].valid = true;
    }
}


int stop_sound_capturing() {
    stop_sound_capture = true;

  // Wake any waiting threads on ring buffer
  pthread_mutex_lock(&rb_mutex);
  pthread_cond_broadcast(&rb_not_empty);
  pthread_cond_broadcast(&rb_not_full);
  pthread_mutex_unlock(&rb_mutex);

  // Join threads if they were created
  pthread_join(audio_capture_thread, NULL);
  pthread_join(audio_processing_thread, NULL);
  pthread_join(renderer_thread, NULL);
  pthread_join(send_led_colors_thread, NULL);

  cleanup_audio();

  rb_free();
  // Free mel filter resources for all effects
  for (int i = 0; i < sound_effect_count; i++) {
    if (sound_effects[i].mel_filters) free_mel_filterbank(sound_effects[i].mel_filters);
    if (sound_effects[i].mel_energies) free(sound_effects[i].mel_energies);
  }

    // Free LED double buffers
    if (led_buf_front) free(led_buf_front);
    if (led_buf_back) free(led_buf_back);

  free(sound_effects);
  free(led_colors);
  return 0;
}

/**
 * Per-frame processor for brightness-on-volume effect.
 * This is called from the audio processing thread for each popped frame.
 */
void process_brightness_on_volume_frame(struct sound_effect *effect, ws2811_t *strip, const int16_t *frame, const float *window, float *dc) {
  float rms = compute_rms(frame, FRAME_SIZE, &effect->dc, window);
  // square the RMS to get power
  float rms_sq = rms * rms;

  // EMA on power
  effect->smoothed_rms_sq = effect->rms_alpha * rms_sq + (1.0f - effect->rms_alpha) * effect->smoothed_rms_sq;

  // brightness = sqrt(smoothed_power) * sensitivity
  float brightness = sqrtf(effect->smoothed_rms_sq) * effect->sensitivity;
  if (brightness > 1.0f) brightness = 1.0f;

  // optional: small-deadzone to avoid noise floor flicker
  const float noise_floor = 0.001f; // tune
  if (brightness < noise_floor) brightness = 0.0f;

  // apply attack/release smoothing as you already do
  effect->brightness_smooth = smooth_brightness_decay(effect->brightness_smooth, brightness, .15, .05);

  // Apply to LEDs
  pthread_mutex_lock(&strip_mutex);
  apply_brightness_ratios_to_leds(strip, effect->led_start, effect->led_end, effect->brightness_smooth);
  pthread_mutex_unlock(&strip_mutex);
}

void process_melbank_frame(struct sound_effect *effect, ws2811_t *strip, const int16_t *frame, const float *window, float *dc) {
  if (!effect->mel_filters || effect->n_mel_filters == 0) return;

  int n = FRAME_SIZE;
  int n_bins = n / 2 + 1;
  // prepare real/imag arrays
  float real[n];
  float imag[n];
  for (int i = 0; i < n; i++) {
    real[i] = (frame[i] / 32768.0f) * window[i];
    imag[i] = 0.0f;
  }
  fft_inplace(real, imag, n);

  // magnitudes
  float mag[n_bins];
  for (int k = 0; k < n_bins; k++) mag[k] = sqrtf(real[k]*real[k] + imag[k]*imag[k]);

  // compute mel energies
  compute_mel_energies(mag, n_bins, effect->mel_filters, effect->n_mel_filters, effect->mel_energies);

  // smooth energies and map to LEDs low->high
  int led_start = effect->led_start;
  int led_end = effect->led_end;
  int led_count = led_end - led_start + 1;

  for (int f = 0; f < effect->n_mel_filters; f++) {
    float e = effect->mel_energies[f];
    // EMA smoothing per band
    effect->mel_energies[f] = effect->rms_alpha * e + (1.0f - effect->rms_alpha) * effect->mel_energies[f];
  }

  // Map mel filters evenly across LEDs (low->high)
  // Write into back buffer
  for (int i = 0; i < led_count; i++) {
    float pos = (float)i / (float)(led_count - 1);
    int filter_index = (int)roundf(pos * (effect->n_mel_filters - 1));
    if (filter_index < 0) filter_index = 0;
    if (filter_index >= effect->n_mel_filters) filter_index = effect->n_mel_filters - 1;
    float energy = effect->mel_energies[filter_index];
    // convert to dB-like: avoid log(0)
    float db = 10.0f * log10f(fmaxf(energy, 1e-12f));
    // map db range [-80..0] to [0..1]
    float db_min = -80.0f;
    float db_max = 0.0f;
    float norm = (db - db_min) / (db_max - db_min);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    // apply sensitivity/gain
    float brightness = fminf(norm * effect->sensitivity, 1.0f);
    // write RGB float into back buffer (scale base color)
    uint32_t base = led_colors[led_start + i].base_color;
    float r = ((base >> 16) & 0xFF) / 255.0f * brightness;
    float g = ((base >> 8) & 0xFF) / 255.0f * brightness;
    float b = (base & 0xFF) / 255.0f * brightness;
    int off = (led_start + i) * 3;
    led_buf_back[off + 0] = r;
    led_buf_back[off + 1] = g;
    led_buf_back[off + 2] = b;
  }

  // Swap back -> front under lock
  pthread_mutex_lock(&strip_mutex);
  float *tmp = led_buf_front;
  led_buf_front = led_buf_back;
  led_buf_back = tmp;
  // copy front float buffer into actual LED strip storage (uint8/uint32) for rendering
  for (int i = 0; i < led_count; i++) {
    int off = i * 3;
    uint8_t rr = (uint8_t)fminf(led_buf_front[off + 0] * 255.0f, 255.0f);
    uint8_t gg = (uint8_t)fminf(led_buf_front[off + 1] * 255.0f, 255.0f);
    uint8_t bb = (uint8_t)fminf(led_buf_front[off + 2] * 255.0f, 255.0f);
    set_led_color(led_start + i, rr, gg, bb);
    led_colors[led_start + i].color = (rr << 16) | (gg << 8) | bb;
    led_colors[led_start + i].valid = true;
  }
  pthread_mutex_unlock(&strip_mutex);
}

// Legacy initializer used when starting effects via apply_effect. This is
// intentionally non-blocking: it performs any lightweight setup required by
// the effect and returns immediately. The actual per-frame work is handled
// via the function pointer stored in effect->process_frame which is invoked
// by the audio processing thread.
void brightness_on_volume_effect(sound_effect *effect, ws2811_t *strip) {
  // Currently no heavy initialization required for brightness-on-volume.
  // Ensure runtime state is zeroed and ready for per-frame processing.
  printf("brightness_on_volume_effect (initializer) for effect %s\n", effect->name);
  effect->dc = 0.0f;
  effect->brightness_smooth = 0.0f;
  effect->smoothed_rms_sq = 0.0f;
  // No blocking loop — return immediately so startup is non-blocking.
}
