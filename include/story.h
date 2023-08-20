#ifndef __STORY_H
#define __STORY_H

#include <libdragon.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <GL/gl.h>

#include "terrain.h"
#include "dialogue.h"
#include "summer.h"
#include "player.h"

#define DIG_LIMIT                   200

#define STORY_FLAGS_ALIVE           0x1     // If not set, the item does nothing
#define STORY_FLAGS_ENABLED         0x2     // Can interact
#define STORY_FLAGS_COLLISION       0x4     // Can collide
#define STORY_FLAGS_FOLLOW_DEPTH    0x8     // Follow the depth relative to initial position
#define STORY_FLAGS_ALWAYS_INTERACT 0x10    // Trigger interact function even if under sand

typedef struct player_s player_t;
typedef struct gamestate_s gamestate_t;

typedef struct vec3_s {
    float x;
    float y;
    float z;
} vec3_t;

typedef struct story_s {
    char *id;                           // Should match the mesh name
    uint32_t flags;                     // See STORY_FLAGS_
    float radius;                       // Used for collision and deciding to render when under the sand
    float interaction_radius;           // When inside this range, you can trigger the func with Z
    float _y_offset;
    float _transform[16];               //
    float _current_height;               //
    int _model_id;                      // If there is a model, this is gt zero and we will render it.
    bool (*interaction_action)(         // This is the function to execute when it is interacted with
        struct story_s *self,           // Returns true if interaction actually happened
        gamestate_t *game_state,        // First interaction in the list wins
        float height
    );
    void (*update)(                     // This is the function to execute to udpate stuff
        struct story_s *self,
        gamestate_t *game_state,
        float height
    );
    void *data;
} story_t;

void story_init(terrain_t *map, gamestate_t *game_state);
bool story_render(gamestate_t *game_state, player_t *player);
void story_begin();
void story_end(gamestate_t *game_state);

#endif // __STORY_H
