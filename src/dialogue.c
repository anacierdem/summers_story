#include <libdragon.h>

#include "dialogue.h"

static dialogue_t *current_line;
static uint64_t start_time;
static size_t current_line_len;
static int speed_up_from;
static bool dialog_skipped = false;

dialogue_t dialogue_intro = (dialogue_t){
    .text=  "Her name was Summer.\n"
            "Lorem ipsum dolor sit amet",
    .next=&(dialogue_t){
        .text=  "She walked the beaches\n"
                "And played with her toys"
    }
};

dialogue_t dialogue_pickup_spade = (dialogue_t){
    .text="She used to live with grandpa."
};

void dialogue_init() {

}

/**
 * Must not call when there is an active dialogue going on
 * FIXME: instead keep a reference to game state and use that
*/
bool dialogue_set_line(dialogue_t *line) {
    // No need to show something more than once, we already keep the last
    // three lines on screen
    if (line->_played) {
        return false;
    }

    current_line = line;
    if (line == NULL) {
        current_line_len = 0;
        return false;
    };

    speed_up_from = 0;
    start_time = timer_ticks();
    current_line_len = strlen(line->text);
    return true;
}

static char *marker1 = "^01>";
static char *marker2 = "^01|";
static char *marker1_bad = "^02>";
static char *marker2_bad = "^02|";

// Supress is a hacky way to prevetn the action triggering dialogue_set_line from also triggering speed up
void dialogue_render(gamestate_t *game_state, bool supress) {
    if (current_line == NULL) return;
    game_state->active_dialogue = true;

    struct controller_data keys_down = get_keys_down();

    int step = speed_up_from + (timer_ticks() - start_time) / TICKS_FROM_MS(speed_up_from ? DIALOGUE_DELAY_FAST : DIALOGUE_DELAY);
    int current_pos = MIN(step, current_line_len);

    bool current_done = current_pos >= (current_line_len - 1);

    if (current_done) {
        char *marker_final = marker1;
        current_line->_played = true;

        if (current_line->next) {
            marker_final = game_state->good_ending ? marker1 : marker1_bad;
            if (keys_down.c[0].Z) {
                dialog_skipped = true;
                dialogue_set_line(current_line->next);
                return;
            }
        } else {
            marker_final = game_state->good_ending ? marker2 : marker2_bad;
            if (keys_down.c[0].Z) {
                dialog_skipped = true;
                game_state->active_dialogue = false;
                current_line = NULL;
                return;
            }
        }

        if (!dialog_skipped) {
            rdpq_text_print(&(rdpq_textparms_t){
                    .align = 2,
                    .width =  display_get_width() * 0.8
                }, 1,
                display_get_width() * 0.1, display_get_height() * 0.1
                , "Z: continue"
            );
        }

        rdpq_text_print(NULL, 1,
            display_get_width() * 0.9 - DIALOGUE_FONT_SIZE, display_get_height() * 0.9 - DIALOGUE_FONT_SIZE
            , marker_final);
    }

    if (!supress && !speed_up_from && (keys_down.c[0].Z)) {
        speed_up_from = (timer_ticks() - start_time) / TICKS_FROM_MS(DIALOGUE_DELAY);
        start_time = timer_ticks();
    }

    char *start = current_line->text;

    rdpq_text_printn(
        &(rdpq_textparms_t){
            .width = display_get_width() * 0.8,
            .line_spacing = -2,
            .wrap = WRAP_WORD,
        }, 1,
        display_get_width() * 0.1, display_get_height() * 0.9 - DIALOGUE_FONT_SIZE * DIALOGUE_HEIGHT
        ,start, current_pos
    );
}