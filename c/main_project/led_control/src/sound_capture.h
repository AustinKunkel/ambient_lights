#pragma once
#include "ws2811.h"

int start_sound_capture(ws2811_t *, int);
int stop_sound_capturing(); // return 0 if good