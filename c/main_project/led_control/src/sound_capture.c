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
#define FRAME_SIZE              256
#ifndef M_PI
#define M_PI                    3.14159265358979323846
#endif

volatile bool stop_sound_capture = false;

pthread_t sound_capture_thread;
pthread_t send_led_colors_thread;

typedef struct sound_effect sound_effect;

typedef void (*effect_fn)(struct sound_effect *effect, ws2811_t *strip);

struct sound_effect {
  char name[50];
  float sensitivity; // multiplier for volume
  int min_freq; // minimum frequency to react to
  int max_freq; // maximum frequency to react to
  int led_start; // starting led index
  int led_end; // ending led index
  int effect_num; // The number for the effect

  effect_fn apply_effect; // function pointer to effect function
};

sound_effect *sound_effects;
struct led_position *led_colors; // server.h
int sound_effect_count = 0;

void brightness_on_volume_effect(sound_effect *effect, ws2811_t *strip);

void assign_effect_function(sound_effect *effect) {
  switch(effect->effect_num) {
    case 1:
      effect->apply_effect = brightness_on_volume_effect;
      break;
    default:
      effect->apply_effect = brightness_on_volume_effect;
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

  struct sound_effect *effect = effect_arg->effect;
  ws2811_t *strip = effect_arg->strip;

  printf("Applying effect: %s\n", effect->name);
  effect->apply_effect(effect, strip);
  free(arg);
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
  if(pthread_create(&sound_capture_thread, NULL, sound_effect_thread_func, (void *)arg) != 0) {
    free(sound_effects);
    free(arg);
    printf("Failed to create sound capture thread!\n"); 
    return 1;
  }
  
  if(pthread_create(&send_led_colors_thread, NULL, send_led_colors_loop, NULL) != 0) {
    stop_sound_capturing();
    free(arg);
    printf("Failed to create send positions thread!");
    return 1;
  }

}

void brightness_on_volume_effect(sound_effect *effect, ws2811_t *strip) {
  // setup audio capture
  // get volume level
  // set brightness based on volume level
  // apply to leds from led_start to led_end

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 11000000L; // ~11ms for ~90fps
  printf("Setting up audio capture...\n");
  
  setup_audio_capture(48000, 1);

  int16_t buffer[FRAME_SIZE]; // stack allocation is fine for 256

  float window[FRAME_SIZE];
  for (int i = 0; i < FRAME_SIZE; i++) {
      window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FRAME_SIZE - 1)));
  }

  static float dc = 0.0f;          // running estimate of DC offset
  const float dc_beta = 0.001f;    // smoothing factor (tune 1e-4 .. 1e-2)

  int should_skip_loop = 0;

  while (!stop_sound_capture) {
    capture_audio_frame(buffer, FRAME_SIZE, &should_skip_loop);

    printf("Captured %d samples:\n", FRAME_SIZE);
    for (int i = 0; i < 20; i++) {  // print first 20 samples for brevity
      printf("%6d ", buffer[i]);
    }
    printf("\n");

    float sum_squares = 0.0f;
    for (int i = 0; i < FRAME_SIZE; i++) {
      float s = buffer[i] / 32768.0f;    // normalize to [-1, 1]
      dc += dc_beta * (s - dc);          // update DC offset
      float s_ac = s - dc;               // DC-removed sample
      float x = s_ac * window[i];        // apply Hann window
      sum_squares += x * x;
    }

    float rms = sqrtf(sum_squares / FRAME_SIZE);
    if (isnan(rms) || isinf(rms)) {
      rms = 0.0f;
    }

    float rms_db = 20.0f * log10f(rms + 1e-12f);
    if (rms_db < -50.0f) { // below -50 dBFS, treat as silence
        rms = 0.0f;
    }

    float volume = rms * effect->sensitivity;
    if (volume > 1.0f) volume = 1.0f;
    if (volume < 0.0f) volume = 0.0f;

    // Map volume to brightness (0-255)
    int brightness = (int)(volume * 255);
    if (brightness > 255) brightness = 255;
    if (brightness < 0) brightness = 0;

    printf("RMS: %.6f  Volume: %.4f  Brightness: %d\n", rms, volume, brightness);

    // Set LED colors based on brightness
    for (int i = effect->led_start; i <= effect->led_end; i++) {
      uint32_t color = led_colors[i].color;
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;

      float scale = brightness / 255.0f;
      r = (uint8_t)(r * scale);
      g = (uint8_t)(g * scale);
      b = (uint8_t)(b * scale);

      set_led_color(i, r, g, b);
      led_colors[i].color = (r << 16) | (g << 8) | b;
      led_colors[i].valid = true;
    }

    ws2811_render(strip);

    nanosleep(&ts, NULL); // Sleep for a short time
  }

}


int stop_sound_capturing() {
    stop_sound_capture = true;

    pthread_join(sound_capture_thread, NULL);
    pthread_join(send_led_colors_thread, NULL);
    cleanup_audio();
    free(sound_effects);
    free(led_colors);
    return 0;
}