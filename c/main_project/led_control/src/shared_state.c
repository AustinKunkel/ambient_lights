#include "shared_state.h"

SharedScreenState g_screen_state = {
  .avg_screen_color = 0,
  .valid = 0,
  .mutex = PTHREAD_MUTEX_INITIALIZER
};

void init_shared_screen_state(void) {
  pthread_mutex_init(&g_screen_state.mutex, NULL);
  g_screen_state.avg_screen_color = 0;
  g_screen_state.valid = 0;
}

void set_avg_screen_color(uint32_t color) {
  pthread_mutex_lock(&g_screen_state.mutex);
  g_screen_state.avg_screen_color = color;
  g_screen_state.valid = 1;
  pthread_mutex_unlock(&g_screen_state.mutex);
}

uint32_t get_avg_screen_color(int *valid) {
  pthread_mutex_lock(&g_screen_state.mutex);
  uint32_t color = g_screen_state.avg_screen_color;
  if (valid) *valid = g_screen_state.valid;
  pthread_mutex_unlock(&g_screen_state.mutex);
  return color;
}

void cleanup_shared_screen_state(void) {
  pthread_mutex_destroy(&g_screen_state.mutex);
}