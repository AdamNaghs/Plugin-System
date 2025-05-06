#include "../include/dt.h"

#ifdef RAYLIB_H

#include <raylib.h>

void dt_update() {
    // Nothing needed, Raylib handles timing internally.
}

float get_dt() {
    return GetFrameTime();
}

#else

#include <time.h>

static float delta_time = 0.0f;
static clock_t last_time = 0;

void dt_update() {
    clock_t now = clock();
    if (last_time == 0) {
        last_time = now;
        delta_time = 0.0f;
    } else {
        delta_time = (float)(now - last_time) / CLOCKS_PER_SEC;
        last_time = now;
    }
}

float get_dt() {
    return delta_time;
}

#endif
