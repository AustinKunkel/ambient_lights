#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "main.h"
#include "led_functions.h"
#include "screen_capture.h"
#include "ws2811.h"

// main control file (replaces led_functions.py for the most part)

LEDSettings led_settings = {
    .brightness = 255,        // Default brightness (full brightness)
    .color = 0xFFFFFF,        // Default color (white)
    .capture_screen = 0,      // Default: screen capture disabled
    .sound_react = 0,         // Default: sound reaction disabled
    .fx_num = 0,              // Default: no effect
    .count = 206,             // Example count of LEDs
    .id = 0                   // Default id
};

typedef struct {
    pthread_t thread_id;
    int task_status; // 0 for completed, 1 for running
} task_info;

task_info screen_capture_task;
task_info sound_effect_task;

void stop_current_task();

char *update_leds() {
    printf("Updating the LEDs...\n");
    stop_current_task();
    printf("Stopped current task...\n");
    if(get_led_count() != led_settings.count) {
        cleanup_strip();
        if(setup_strip(led_settings.count)) {
            return "{\"Error\": \"Failed to set up the strip with the count\"}";
        }
    }
    set_brightness(led_settings.brightness);

    if(led_settings.capture_screen) {
        printf("Creating task: Screen Capture...\n");
        ws2811_t *ledstring = get_ledstring();
        printf("LED Count: %d\n", ledstring->channel[0].count);
        if(start_capturing(get_ledstring())) {
            printf("Error starting screen capture!\n");
            return"{\"Error\": \"Error starting screen capture!\"}";
        }
        screen_capture_task.task_status = 1;
    } else if(led_settings.sound_react) { // TODO: Sound Effect functions
        printf("Creating task: Sound Capture...\n");
        if(pthread_create(&sound_effect_task.thread_id, NULL, NULL, NULL)) {
            printf("Error creating Sound Effect thread!\n");
            return"{\"Error\": \"Error creating Sound Effect thread!\"}";
        }
    } else if(led_settings.fx_num > 0) {
           printf("Creating task: Effect...\n");
        // TODO: Effect functions
        //if(pthread_create())
    } else {
        set_strip_32int_color(led_settings.color);
        show_strip();
    }

    return"{\"Success\": \"LEDs updated!\"}";
}

/** 
 * adjusts led variables that are able outside of resetting whole strip
 * 
 */ 
void update_led_vars() {
    set_brightness(led_settings.brightness);
    show_strip();
}

void stop_current_task() {
    if(screen_capture_task.task_status == 1) {
        stop_capturing();
    }
    if(sound_effect_task.task_status == 1) {
        pthread_join(sound_effect_task.thread_id, NULL);
        sound_effect_task.task_status = 0;
        printf("Joined sound effect task");
    }
}

void cancel_bad_thread(task_info* task) {
    pthread_cancel(task->thread_id);
    task->task_status = 0;
    printf("Task cancelled...\n");
}
