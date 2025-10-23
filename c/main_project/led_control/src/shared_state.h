#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <stdint.h>
#include <pthread.h>

// Shared state between screen capture and sound effects
typedef struct {
    uint32_t avg_screen_color;
    int valid;
    pthread_mutex_t mutex;
} SharedScreenState;

extern SharedScreenState g_screen_state;

// Helper functions
void init_shared_screen_state(void);
void set_avg_screen_color(uint32_t color);
uint32_t get_avg_screen_color(int *valid);
void cleanup_shared_screen_state(void);

#endif