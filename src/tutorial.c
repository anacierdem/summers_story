#include "tutorial.h"

static bool discovered_look = false;
static bool discovered_move = false;
static bool timed_out = false;

static int64_t start_time = 0;

void tutorial_init() {

}

void tutorial_render(gamestate_t *game_state) {
    if(game_state->can_interact && !game_state->active_dialogue) {
        rdpq_text_print(&(rdpq_textparms_t){
                .align = 2,
                .width =  display_get_width() * 0.8
            }, 1,
            display_get_width() * 0.1, display_get_height() * 0.1
            , "^01Z: interact"
        );

        // Must have discovered these already
        discovered_look = true;
        discovered_move = true;
        return;
    }

    if (game_state->can_move && start_time == 0) {
        start_time = timer_ticks();
    }

    // The idea here is expecting the player to look around and move around once
    // the initial dialog makes them press Z to skip the dialog. The hand will
    // be in place and they may naturally look around and move around. o/w we
    // have some handholding in place.
    float timeline = (float)(TIMER_MICROS(timer_ticks() - start_time)/1000) / 8000.0f;
    if (timeline > 1.0) {
        timed_out = true;
    }

    struct controller_data keys = get_keys_held();

    if (keys.c[0].x > 30 || keys.c[0].x < -30 || keys.c[0].y > 30 || keys.c[0].y < -30) discovered_look = true;
    if (
        (keys.c[0].C_up || keys.c[0].up) ||
        (keys.c[0].C_down || keys.c[0].down) ||
        (keys.c[0].C_left || keys.c[0].left) ||
        (keys.c[0].C_right || keys.c[0].right)
    ) discovered_move = true;

    // Don't show if the player is in a dialogue
    if (game_state->active_dialogue) return;

    if(!discovered_look && timed_out) {
        rdpq_text_print(&(rdpq_textparms_t){
                .align = 2,
                .width =  display_get_width() * 0.8
            }, 1,
            display_get_width() * 0.1, display_get_height() * 0.1
            , "^01Use the joystick to look around."
        );
        return;
    }

    if(!discovered_move && timed_out) {
        rdpq_text_print(&(rdpq_textparms_t){
                .align = 2,
                .width =  display_get_width() * 0.8
            }, 1,
            display_get_width() * 0.1, display_get_height() * 0.1
            , "^01Use the direction buttons to move around."
        );
        return;
    }
}