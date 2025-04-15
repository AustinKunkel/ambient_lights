#pragma once
#include "ws2811.h"

int start_capturing(ws2811_t *);
int stop_capturing(); // return 0 if good
char *next_token(char **);