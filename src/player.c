#include <libdragon.h>
#include <GL/gl.h>
#include <math.h>

#include "player.h"

static long long dig_start_time;
static bool show_dig_anim = false;
static gamestate_t *game_state;
terrain_t *m_map;
// Max slowdown of 3x allowed
static float frame_time_limit = 3.0f / 60.0f;



#ifndef DEVELOPMENT
static wav64_t ambient[2];
static float wind_volume[2] = { 0.1f, 0.1f };
static float waves_volume[2] = { 1.0f, 1.0f };
static int loop_cb(int id) {
    float r = 6000.0f *  (float)(rand()) / RAND_MAX;
    int newFreq = 44100 - 3000 + (int)(r);
    int newLength = (int)(((float)(ambient[id].wave.len) / (float)(newFreq)) * 44100.0f);
    mixer_ch_play(id*2, &(ambient[id]).wave);
    mixer_ch_set_freq(id*2, newFreq);
    return (int)(newLength);
}
#endif

void player_init(player_t *const player, gamestate_t *gs, terrain_t *map)
{
    m_map = map;
    game_state = gs;
    player->x = 5.0f;
    player->y = 2.0f;
    player->z = 1.0f;
    player->yaw = 90.0f;
    player->pitch = .0f;

    if (get_tv_type() == TV_PAL) {
        frame_time_limit = 3.0f / 50.0f;
    }

#ifndef DEVELOPMENT
    wav64_open(&(ambient[0]), "wind.wav64");
    wav64_open(&(ambient[1]), "waves.wav64");

    mixer_add_event(0, (int (*)(void*))loop_cb, (void*)0);
    mixer_add_event(0, (int (*)(void*))loop_cb, (void*)1);
#endif
}

void update_position(player_t *player, struct controller_data *keys, float delta_time) {
    float limited_delta = fminf(delta_time, frame_time_limit);

    player->yaw -= keys->c[0].x * LOOK_RATE * limited_delta;
    player->pitch += keys->c[0].y * LOOK_RATE * limited_delta;

    player->pitch = fminf(player->pitch, 30.0f);
    player->pitch = fmaxf(player->pitch, -80.0f);
    player->yaw = fmodf(player->yaw, 360.0f);
    player->yaw = player->yaw < 0.0 ? (360.0 - player->yaw) : player->yaw;

    if (game_state->can_move) {
        float rot_rad = player->yaw * M_PI / 180.0f;
        float parallel = cos(rot_rad);
        float perp = sin(rot_rad);

        float new_x = player->x;
        float new_z = player->z;
        if(keys->c[0].C_up || keys->c[0].up) {
            new_x -= RATE * perp * limited_delta;
            new_z -= RATE * parallel * limited_delta;
        }

        if(keys->c[0].C_down || keys->c[0].down) {
            new_x += RATE * perp * limited_delta;
            new_z += RATE * parallel * limited_delta;
        }

        if(keys->c[0].C_right || keys->c[0].right) {
            new_x += RATE * parallel * limited_delta;
            new_z -= RATE * perp * limited_delta;
        }

        if(keys->c[0].C_left || keys->c[0].left) {
            new_x -= RATE * parallel * limited_delta;
            new_z += RATE * perp * limited_delta;
        }

        // Positive x
        if (new_x - PLAYER_BOUND > 0.0) {
            new_x = PLAYER_BOUND;
        }

        // Negative x
        if (new_x + PLAYER_BOUND < -2.0) {
            new_x = -PLAYER_BOUND -2.0;
        }

        // Positive z
        if (new_z - PLAYER_BOUND > 0.0) {
            new_z = PLAYER_BOUND;
        }

        // Negative z
        if (new_z + PLAYER_BOUND < 0.0) {
            new_z = -PLAYER_BOUND;
        }

        // commit to the change
        player->x = new_x;
        player->z = new_z;
    }

    float distance = sqrt(player->x * player->x + player->z * player->z) / PLAYER_BOUND;
    distance = fminf(distance, 1.0f) / 2.0f;

#ifndef DEVELOPMENT
    mixer_ch_set_vol(0,
        wind_volume[0] * 0.5f + wind_volume[0] * (0.5-distance),
        wind_volume[1] * 0.5f + wind_volume[1] * (0.5-distance)
    );
    mixer_ch_set_vol(2,
        waves_volume[0] * 0.5f + waves_volume[0] * distance,
        waves_volume[1] * 0.5f + waves_volume[1] * distance
    );
#endif
}

void player_update(player_t *player, float delta_time, bool suppress_digging) {
    struct controller_data keys = get_keys_pressed();
    update_position(player, &keys, delta_time);
    float height = terrain_get_height(m_map, player->x, player->z);
    player->y = height + PLAYER_OFFSET;
    // Don't clip into the water
    player->y = fmaxf(player->y, PLAYER_OFFSET + WATER_LEVEL);

    struct controller_data keys_down = get_keys_down();
    if(!game_state->active_dialogue && keys_down.c[0].Z && !suppress_digging) {
        if (game_state->spade_idx && player->pitch < -30.0) {
            game_state->dig_count++;
            terrain_dig(m_map, player->x, player->z, player->yaw);
        }
        dig_start_time = timer_ticks();
        show_dig_anim = true;
    }

    // Nothing else to display for player anymore
    if(game_state->game_end_ticks > 0) {
        return;
    }

    if(game_state->spade_idx) {
        float durationMs = 100.0f;
        float timeline = (float)(TIMER_MICROS(timer_ticks() - dig_start_time)/1000) / durationMs;
        // On ares timer_ticks overflows, so check if negative as well
        if (timeline > 1.0 || timeline < 0.0f) {
            show_dig_anim = false;
        }

        glPushMatrix();
            // TODO: can pre-bake some of this
            glTranslatef(0.3,-0.3,-0.8 + (show_dig_anim ? ((1.0f - timeline) * -0.1) : 0.0f));
            glRotatef(10 + (show_dig_anim ? (1.0f - timeline) * 10.0f : 0.0),1,0,0);
            glRotatef(-90 + (show_dig_anim ? (1.0f - timeline) * 10.0f : 0.0),0,1,0);
            glScalef(0.1, 0.1, 0.1);
            // if (!show_dig_anim) glDepthFunc(GL_ALWAYS);
            glDepthFunc(GL_ALWAYS);
            glCallList(game_state->spade_idx);
        glPopMatrix();
    }

    if(game_state->teddy_idx) {
        glPushMatrix();
            // TODO: can pre-bake some of this
            glTranslatef(-0.3,-0.3,-0.8);
            glScalef(0.8, 0.8, 0.8);
            glRotatef(-30,0,1,0);
            glDepthFunc(GL_ALWAYS);
            glCallList(game_state->teddy_idx);

        glPopMatrix();
    }

    if(game_state->compass_idx && game_state->compass_arrow_idx && !game_state->teddy_idx) {
        glPushMatrix();
            // TODO: can pre-bake this
            glTranslatef(-0.3,-0.3,-0.8);
            glScalef(0.05, 0.05, 0.05);
            glDepthFunc(GL_ALWAYS);
            glCallList(game_state->compass_idx);

            glPushMatrix();
                float needle = 200 - player->yaw;
                if (!game_state->magnet_removed) needle = (atan2f(player->x, player->z) * 360 / (2*M_PI)) - player->yaw;
                glRotatef(needle, 0, 1, 0);
                glCallList(game_state->compass_arrow_idx);
            glPopMatrix();

        glPopMatrix();
    }
}

void player_transform(player_t *const player)
{
    glRotatef(-player->pitch, 1, 0, 0);
    glRotatef(-player->yaw, 0, 1, 0);
    glTranslatef(-player->x, -player->y, -player->z);
}