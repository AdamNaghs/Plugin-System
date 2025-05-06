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
static struct timespec last_time = {0};

void dt_update() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_time.tv_sec == 0 && last_time.tv_nsec == 0) {
        last_time = now;
        delta_time = 0.0f;
    } else {
        delta_time = (now.tv_sec - last_time.tv_sec)
                   + (now.tv_nsec - last_time.tv_nsec) / 1e9f;
        last_time = now;
    }
}

float get_dt() {
    return delta_time;
}


#endif
