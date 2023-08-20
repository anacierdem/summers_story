#ifndef PLAYER_H_
#define PLAYER_H_

#include <math.h>

#include "summer.h"
#include "utils.h"
#include "terrain.h"
#include "dialogue.h"
#include "story.h"

#define PLAYER_BOUND 9
#define PLAYER_OFFSET 1.0f
#define RATE 2.0f // meter per second
#define LOOK_RATE 2.0f // degree per second divided by controller rate

typedef struct gamestate_s gamestate_t;

typedef struct player_s {
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
} player_t;


void player_init(player_t *const player, gamestate_t *game_state, terrain_t *m_map);
void player_update(player_t *player, float delta_time, bool suppress_digging);
void player_transform(player_t *const player);

#endif // PLAYER_H_