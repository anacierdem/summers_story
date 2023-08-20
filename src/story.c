#include <stdbool.h>

#include "story.h"

static bool story_started = false;
static bool story_ended = false;
static terrain_t *m_map;

enum story_id {
    chest,
    chest_lid,
    // This is before others because of drawing order
    chest_details,

    ball,
    spade,
    flip_flop_a,
    flip_flop_b,
    compass,
    compass_arrow,
    key,
    magnet,
    teddy,
    story_item_end
};

static story_t story_items[];

static dialogue_t start_dialogue = (dialogue_t){
    .text=  "Summer awoke from a restless sleep.\n",
    .next= &(dialogue_t){
        .text=  "Last thing she remembered was that she was with her family, happy and careless.\n"
                "Then there was the storm and now, she was all alone...\n",
        .next= &(dialogue_t){
            .text=  "The place felt familiar to her, like a distant memory. \n"
                    "She couldn't explain the feeling, she'd never been here before.",
            .next= &(dialogue_t){
                .text= "She felt an uneasy feeling in her chest. It was her precious Teddy missing.\n"
                        "A cold shrouded her heart, she was a little child after all.",
                .next= &(dialogue_t){
                    .text= "The North, she remembered. It must be to the North shore.\n"
                            "If only she could align with the North Star like they did with grandpa, \n"
                            "but it was plain daylight and not star in the sky.",
                    .next= &(dialogue_t){
                        .text= "The warmth of the summer sun gave her hope."
                    }
                }
            }
        }
    }
};


static dialogue_t end_dialogue_good = (dialogue_t){
    .text = "Teddy was finally in her heart,         \n"
            "buried.",
    .next= &(dialogue_t){
        .text= "She woke up.                          \n",
        .next= &(dialogue_t){
            .text= "Grandpa was ok after the accident.      \n"
                    "This was just a bad dream after all.\n",
            .next= &(dialogue_t){
                .text= "A bad dream that she may never forget...",
                .next= &(dialogue_t){
                    .text= "She just turned the N64 off to let it go.",
                }
            }
        }
    }
};

static dialogue_t end_dialogue_bad = (dialogue_t){
    .text = "^02Teddy was finally in her heart\n"
            "Buried.",
    .next= &(dialogue_t){
        .text= "^02She woke up.                          ",
        .next= &(dialogue_t){
            .text= "^02He was gone forever...",
            .next= &(dialogue_t){
                .text= "^02The bad dream helped her forget him a tiny bit more.",
                .next= &(dialogue_t){
                    .text= "^02She just turned the N64 off to let him rest.\n"
                            "She could try again without disturbing the sands that much.",
                }
            }
        }
    }
};

static void kill_item(story_t *self) {
    self->_model_id = 0;
    self->flags = 0;
}

/**
 * BALL
*/

static dialogue_t ball_taken_dialogue = (dialogue_t){
    .text=  "This was her old beachball. It was lost a long time ago...\n",
    .next= &(dialogue_t){
        .text= "Why was it here though? It didn't make any sense.\n"
                "When that storm hit, it wasn't even there with them.",
        .next= &(dialogue_t){
            .text= "She clearly remembers the summer days she used to play with it.\n"
                    "With her Grandpa...",
        }
    }
};

static bool ball_interaction(story_t *self, gamestate_t *game_state, float height) {
    kill_item(self);
    return dialogue_set_line(&ball_taken_dialogue);
}

/**
 * SPADE
*/

static dialogue_t spade_taken_dialogue = (dialogue_t){
    .text=  "It was one of her Grandpa's tools.        \n"
            "He would let her use this one, but he always supervised.",
    .next= &(dialogue_t){
        .text= "\"You never know what you'll find.\" he would warn,\n"
                "\"You should be careful of where you are digging\".",
        .next= &(dialogue_t){
            .text= "She knew she must think twice before using it.",
            .next= &(dialogue_t){
                .text= "She could disturb her fear,  \n"
                        "from under the precious sands.",
                .next= &(dialogue_t){
                    .text= "Where was her Grandpa though? He would never\n"
                            "leave his tools around.",
                }
            }
        }

    }
};

static bool spade_interaction(story_t *self, gamestate_t *game_state, float height) {
    // Cannot force interact with the flip flop anymore
    story_items[flip_flop_a].flags &= ~STORY_FLAGS_ALWAYS_INTERACT;

    // Transfer model ownership to game state
    game_state->spade_idx = self->_model_id;
    kill_item(self);
    return dialogue_set_line(&spade_taken_dialogue);
}

/**
 * FLIP FLOPS
*/

static dialogue_t flip_flop_a_first_interaction_dialogue = (dialogue_t){
    .text = "Something bright and colorful stood out from the sand.\n"
            "Summer tried to investigate further, but the sand refused to release the object,\n"
            "almost as if it was frozen in time.",
};

static dialogue_t flip_flop_taken_dialogue = (dialogue_t){
    .text = "One of her flip flops.        \n"
            "They must have been covered in sand when the storm hit.\n"
            "The other one must be near by as well..."
};

static dialogue_t flip_flop_both_dialogue = (dialogue_t){
    .text = "She remembers when she was young, she would leave her shoes pointing\n"
            "in the direction home in case the wind covered her tracks across the dunes.",
};

static bool flip_flop_a_interaction(story_t *self, gamestate_t *game_state, float height) {
    bool interacted = game_state->spade_idx ? false : dialogue_set_line(&flip_flop_a_first_interaction_dialogue);

    // Disable interaction as the initial dialogue is over
    self->flags &= ~STORY_FLAGS_ALWAYS_INTERACT;

    // Cannot pickup if origin is under sand
    if (height > self->_transform[13]) return interacted;

    if (!((bool)self->data)) {
        self->data = (void*)true;
    }
    interacted = dialogue_set_line(&flip_flop_taken_dialogue);

    self->flags &= ~STORY_FLAGS_ENABLED;

    // Both are taken
    if ((bool)(story_items[flip_flop_a].data) && (bool)(story_items[flip_flop_b].data)) {
        interacted = true;
        dialogue_set_line(&flip_flop_both_dialogue);
    }

    return interacted;
}

/**
 * CHEST
*/

typedef struct chest_lid_data_s {
    long long anim_start_ticks;
    bool opened;
    bool done;
} chest_lid_data_t;

static dialogue_t chest_locked_dialogue = (dialogue_t){
    .text = "It was a container of sorts, but it wouldn't budge.\n",
    .next= &(dialogue_t){
        .text=  "It refused to open itself for anyone, just like her heart at this very moment.",
    }
};

static dialogue_t chest_open_dialogue = (dialogue_t){
    .text = "No. It was not over yet. Still everything reminds her of Grandpa.\n"
};

static void chest_lid_update(story_t *self, gamestate_t *game_state, float height) {
    chest_lid_data_t * lid_data = (chest_lid_data_t*)self->data;
    float start_angle = lid_data->opened ? 0.0f : 110.0f;
    float end_angle = lid_data->opened ? 110.0f : 0.0f;

    float timeline = (float)(TIMER_MICROS(timer_ticks() - ((chest_lid_data_t*)self->data)->anim_start_ticks)/1000) / 1000.0f;

    if (timeline >= 1) {
        // on Ares, timer_ticks() overflows
        ((chest_lid_data_t*)self->data)->done = true;

        // We kill the compass when we are done with it so we can skip enabling it if done.
        if (story_items[compass].flags & STORY_FLAGS_ALIVE) {
            story_items[compass].flags |= STORY_FLAGS_ENABLED;
            story_items[compass_arrow].flags |= STORY_FLAGS_ENABLED;
        }
    }

    // TODO: implement a basic animation system
    glRotatef(((chest_lid_data_t*)self->data)->done ? end_angle : (timeline * end_angle + (1.0f - timeline) * start_angle), 0, 0, 1);
}

static void chest_detail_update(story_t *self, gamestate_t *game_state, float height) {
    glDepthFunc(GL_ALWAYS);
}


static bool chest_interaction(story_t *self, gamestate_t *game_state, float height) {
    // Don't have the key, only option is the initial dialogue
    if (story_items[key].flags != 0) {
        // Disable force interaction as the initial dialogue is over
        self->flags &= ~STORY_FLAGS_ALWAYS_INTERACT;
        return dialogue_set_line(&chest_locked_dialogue);
    }

    chest_lid_data_t *chest_lid_data = (chest_lid_data_t*)story_items[chest_lid].data;
    if (!chest_lid_data->opened) {
        chest_lid_data->done = false;
        chest_lid_data->opened = true;
        chest_lid_data->anim_start_ticks = timer_ticks();
        dialogue_set_line(&chest_open_dialogue);

        // Disable chest until teddy is found, if not already found
        if (story_items[teddy].flags) {
            story_items[compass].flags |= STORY_FLAGS_ALIVE;
            story_items[compass_arrow].flags |= STORY_FLAGS_ALIVE;

            self->flags &= ~STORY_FLAGS_ENABLED;
            // Without this the above would be ineffective
            self->flags &= ~STORY_FLAGS_ALWAYS_INTERACT;
        }

        return true;
    }

    if (chest_lid_data->opened) {
        // Disable chest to prevent furthe interaction hints
        self->flags &= ~STORY_FLAGS_ENABLED;
        chest_lid_data->done = false;
        chest_lid_data->opened = false;
        chest_lid_data->anim_start_ticks = timer_ticks();
        game_state->game_end_ticks = timer_ticks();
        game_state->game_end_ticks = chest_lid_data->anim_start_ticks;
        game_state->good_ending = game_state->dig_count < DIG_LIMIT ? 1 : 0;
    }

    return false;
}

/**
 * COMPASS
*/

static dialogue_t compass_dialogue = (dialogue_t){
    .text = "The compass helped them find the North Star.",
    .next= &(dialogue_t){
        .text=  "\"Polaris\", he would say. Part of the \"Little Bear\" constellation.\n"
                "Reminded her of her Teddy bear again.",
        .next= &(dialogue_t){
            .text=  "She couldn't rip off the feeling. She must find it.\n"
                    "The agony is unbearable now.",
        }
    }
};

static bool compass_interaction(story_t *self, gamestate_t *game_state, float height) {
    // Transfer model ownership to game state
    game_state->compass_idx = self->_model_id;
    game_state->compass_arrow_idx = story_items[compass_arrow]._model_id;
    kill_item(self);
    kill_item(&story_items[compass_arrow]);
    return dialogue_set_line(&compass_dialogue);
}

/**
 * KEY
*/

static dialogue_t key_dialogue = (dialogue_t){
    .text = "She felt something unexplainable in her chest. A relief...\n",
    .next= &(dialogue_t){
        .text= "This must be it, she thought. An escape from this exile.",
        .next= &(dialogue_t){
            .text= "The effect of the storm was starting to fade.\n"
                    "Even she mistaken the noise of the wind for his Grandpa for a moment.",
        }
    }
};

static bool key_interaction(story_t *self, gamestate_t *game_state, float height) {
    story_items[chest].flags |= STORY_FLAGS_ENABLED;
    kill_item(self);
    return dialogue_set_line(&key_dialogue);
}

/**
 * MAGNET
*/

static dialogue_t magnet_dialogue = (dialogue_t){
    .text = "This was their science toy. It was messing with the compass the whole time.\n",
    .next= &(dialogue_t){
        .text= "With the distraction gone, this must be it now, she thought.\n"
                "An escape from this exile. \n",
        .next= &(dialogue_t){
            .text= "Now she exactly knows which one is the North shore.\n"
                    "In perfect alignment with the Star,\n"
                    "she must find her Teddy.",
            .next= &(dialogue_t){
                .text= "She could feel it in her heart. Teddy was cold and soaked\n"
                        "with water...",
            }
        }
    }
};

static bool magnet_interaction(story_t *self, gamestate_t *game_state, float height) {
    game_state->magnet_removed = true;
    kill_item(self);
    return dialogue_set_line(&magnet_dialogue);
}

/**
 * TEDDY
*/

static dialogue_t teddy_dialogue = (dialogue_t){
    .text = "At last!\n"
            "Her Teddy.",
    .next= &(dialogue_t){
        .text= "This was a birthday present from Grandpa. Her idol.\n"
                "At that moment, the hard truth hit her.     \n"
                "Nothing was because of the storm.",
        .next= &(dialogue_t){
            .text= "She was just upset, very upset. Her Grandpa was in grave danger.\n"
                    "He had an accident the day before.",
            .next= &(dialogue_t){
                .text= "She must end this, put Teddy in her heart, now.",
            }
        }
    }
};

static bool teddy_interaction(story_t *self, gamestate_t *game_state, float height) {
    // Re-enable chest if has key. o/w player still needs to find it
    if (!story_items[key].flags) {
        story_items[chest].flags |= STORY_FLAGS_ENABLED;
    }

    // Transfer model ownership to game state
    game_state->teddy_idx = self->_model_id;
    kill_item(self);
    return dialogue_set_line(&teddy_dialogue);
}


static story_t story_items[] = {
    [ball] = {
        .id = "ball",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED | STORY_FLAGS_COLLISION | STORY_FLAGS_FOLLOW_DEPTH,
        .radius = 0.8f,
        .interaction_radius = 1.f,
        .interaction_action = &ball_interaction,
    },
    [spade] = {
        .id = "spade",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED,
        .radius = 0.8f,
        .interaction_radius = 1.f,
        .interaction_action = &spade_interaction,
    },
    [flip_flop_a] = {
        .id = "flip_flop_a",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED | STORY_FLAGS_ALWAYS_INTERACT | STORY_FLAGS_FOLLOW_DEPTH,
        .radius = 0.2f,
        .interaction_radius = 0.8f,
        .interaction_action = &flip_flop_a_interaction,
        .data = false, // Taken?
    },
    [flip_flop_b] = {
        .id = "flip_flop_b",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED | STORY_FLAGS_FOLLOW_DEPTH,
        .radius = 0.2f,
        .interaction_radius = 0.8f,
        .interaction_action = &flip_flop_a_interaction,
        .data = false, // Taken?
    },
    [chest] = {
        .id = "Chest",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_COLLISION | STORY_FLAGS_FOLLOW_DEPTH | STORY_FLAGS_ALWAYS_INTERACT,
        .radius = 1.0f,
        .interaction_radius = 1.3f,
        .interaction_action = &chest_interaction,
    },
    [chest_lid] = {
        .id = "Chest_Lid",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_FOLLOW_DEPTH,
        .interaction_radius = 1.2f,
        .data = &(chest_lid_data_t){
            .opened = false,
            .done = true,
        },
        .update = chest_lid_update,
    },
    [chest_details] = {
        .id = "Chest_Details",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_FOLLOW_DEPTH,
        .update = chest_detail_update,
    },
    [compass] = {
        .id = "compass",
        .flags = STORY_FLAGS_FOLLOW_DEPTH,
        .interaction_radius = 1.2f,
        .interaction_action = &compass_interaction,
    },
    [compass_arrow] = {
        .id = "arrow",
        .flags = STORY_FLAGS_FOLLOW_DEPTH,
    },
    [key] = {
        .id = "Key",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED,
        .radius = 0.2f,
        .interaction_radius = 0.8f,
        .interaction_action = &key_interaction,
    },
    [magnet] = {
        .id = "magnet",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED,
        .radius = 0.5f,
        .interaction_radius = 0.8f,
        .interaction_action = &magnet_interaction,
    },
    [teddy] = {
        .id = "Teddy",
        .flags = STORY_FLAGS_ALIVE | STORY_FLAGS_ENABLED,
        .radius = 0.5f,
        .interaction_radius = 0.8f,
        .interaction_action = &teddy_interaction,
    },
};
_Static_assert(sizeof(story_items) / sizeof(story_t) == story_item_end, "invalid story item size");

void story_init(terrain_t *map, gamestate_t *game_state) {
    assertf(map != NULL, "Map not loaded");
    m_map = map;

    for (size_t i = 0; i < sizeof(story_items) / sizeof(story_t); i++) {

        story_items[i]._model_id = -1;

        // We know for sure the first one is the terrain
        // Could also skip water but we are just setting things up yet.
        for (size_t j = 1; j < m_map->num_meshes; j++) {
            if (strcmp(m_map->meshes[j].name, story_items[i].id) == 0) {
                story_items[i]._model_id = glGenLists(1);
                assertf(story_items[i]._model_id, "Unable to create dlist"); 
                glNewList(story_items[i]._model_id, GL_COMPILE);
                    terrain_draw_mesh(&m_map->meshes[j]);
                glEndList();
                memcpy(story_items[i]._transform, m_map->meshes[j].transform, sizeof(story_items[i]._transform));
                story_items[i]._current_height = terrain_get_height(m_map, story_items[i]._transform[12], story_items[i]._transform[14]);
                story_items[i]._y_offset = story_items[i]._transform[13] - story_items[i]._current_height;
                story_items[i]._y_offset = story_items[i]._y_offset < 0 ? 0 : story_items[i]._y_offset;
                break;
            }
        }
    }
}

void story_begin() {
    if (story_started) return;
    story_started = true;
    dialogue_set_line(&start_dialogue);
}

void story_end(gamestate_t *game_state) {
    if (!story_started) return;
    if (story_ended) return;
    if (game_state->good_ending) {
        dialogue_set_line(&end_dialogue_good);
    } else {
        dialogue_set_line(&end_dialogue_bad);
    }
}

/**
 * Updates the player position to prevent object collision
 * This is just a rough approximation to minimize clipping
 * Returns the distance between given item's position and the player
*/
static float handle_collision(story_t *story_item, player_t *player) {
    float x_comp = player->x - story_item->_transform[12];
    float z_comp = player->z - story_item->_transform[14];
    float distance = sqrtf(x_comp * x_comp + z_comp * z_comp);
    if(story_item->radius == 0.0f || !(story_item->flags & STORY_FLAGS_COLLISION)) return distance;

    float intersection = story_item->radius - distance;
    if (intersection > 0.0) {
        float x_unit_comp = x_comp / distance;
        float z_unit_comp = z_comp / distance;
        player->x += x_unit_comp * intersection;
        player->z += z_unit_comp * intersection;
    }
    return distance;
}

bool story_render(gamestate_t *game_state, player_t *player) {
    game_state->can_interact = false;
    bool taken_action = false;

    for (size_t i = 0; i < sizeof(story_items) / sizeof(story_t); i++) {
        if (!(story_items[i].flags & STORY_FLAGS_ALIVE)) continue;

        float height = terrain_get_height(m_map, story_items[i]._transform[12], story_items[i]._transform[14]);

        // Can't even interact if it is fully under the sand
        if (height > (story_items[i]._transform[13] + story_items[i].radius)) continue;

        // Render the object
        if (story_items[i]._model_id > 0) {
            glDepthFunc(GL_LESS_INTERPENETRATING_N64);
            glPushMatrix();

            float old_y = story_items[i]._transform[13];
            // Enable follow depth once it is surfaced
            if(story_items[i].flags & STORY_FLAGS_FOLLOW_DEPTH && height < story_items[i]._transform[13]) {
                story_items[i]._transform[13] = fmaxf(height, game_state->water_height + 0.05f) + story_items[i]._y_offset;
                story_items[i]._current_height = story_items[i]._transform[13];
            }
            glMultMatrixf(story_items[i]._transform);
            story_items[i]._transform[13] = old_y;

            // Update if necessary
            if (story_items[i].update) story_items[i].update(&story_items[i], game_state, height);

            glCallList(story_items[i]._model_id);
            glPopMatrix();
        }

        if (!game_state->started) continue;

        float distance = handle_collision(&story_items[i], player);

        // No interaction potential
        if (!story_items[i].interaction_action) continue;

        // Cannot interact if origin is under sand or disabled, except if overridden
        if (
            !(story_items[i].flags & STORY_FLAGS_ALWAYS_INTERACT) &&
            (
                height > story_items[i]._transform[13] ||
                !(story_items[i].flags & STORY_FLAGS_ENABLED)
            )
        ) continue;

        if (
            distance > 0 &&
            distance < story_items[i].interaction_radius
        ) {
            float dir = (atan2f(story_items[i]._transform[12] - player->x, story_items[i]._transform[14] - player->z) * 360 / (2*M_PI)) + 180;
            float vert_distance = story_items[i]._current_height - player->y;
            if (fabsf(dir - player->yaw) < 30 && fabsf(atanf(vert_distance/distance)  * 360 / (2*M_PI) - player->pitch) < 30) {
                game_state->can_interact = true;

                struct controller_data keys_down = get_keys_down();

                if (!taken_action && !game_state->active_dialogue && keys_down.c[0].Z) {
                    // Suppress digging for this keypress
                    taken_action = story_items[i].interaction_action(&story_items[i], game_state, height);
                }
            }
        }
    }
    return taken_action;
}
