#ifndef __SUMMER_H
#define __SUMMER_H

#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#include "player.h"
#include "terrain.h"
#include "dialogue.h"
#include "story.h"
#include "menu.h"
#include "tutorial.h"
#include "intro.h"

typedef struct gamestate_s {
    GLuint spade_idx;
    GLuint compass_idx;
    GLuint compass_arrow_idx;
    GLuint teddy_idx;
    bool can_move;
    bool started;
    bool active_dialogue;
    bool can_interact;
    bool magnet_removed;
    long long game_end_ticks;
    int good_ending;
    int dig_count;
    float water_height;
} gamestate_t;

#define RENDERING_DISTANCE 14.0f
// FIXME: Ideally this should use the actual water level
#define WATER_LEVEL 0.95f

// I think there is something wrong with the fog implementation that makes it
// shimmer. This also happens without rotation but is less noticeable.
// #define ENABLE_ROTATION

#endif // __SUMMER_H