#include "menu.h"

static sprite_t *title[4];
static bool start_pressed = false;
static int64_t start_time;
static int64_t last_frame_time;
static int current_frame = 0;
static rdpq_font_t *font;

void menu_init(rdpq_font_t *f) {
    font = f;
    title[0] = sprite_load("rom:/1.sprite");
    assertf(title[0], "missing menu sprite");

    title[1] = sprite_load("rom:/2.sprite");
    assertf(title[1], "missing menu sprite");

    title[2] = sprite_load("rom:/3.sprite");
    assertf(title[2], "missing menu sprite");

    title[3] = sprite_load("rom:/4.sprite");
    assertf(title[3], "missing menu sprite");
}

void menu_render(gamestate_t *game_state) {
    int controllers = get_controllers_present();

    if (game_state->started) return;
    struct controller_data keys_down = get_keys_down();

    if(controllers & CONTROLLER_1_INSERTED && !start_pressed && keys_down.c[0].start) {
        start_time = timer_ticks();
        start_pressed = true;
        game_state->can_move = true;
    }

    float timeline = start_pressed ? (float)(TIMER_MICROS(timer_ticks() - start_time)/1000) : 0.0f;

    if (timeline >= (START_ANIMATION_TIME + START_DELAY)) {
        game_state->started = true;
        story_begin();
    };

    if (timeline >= START_ANIMATION_TIME) return;

    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM), (TEX0,0,PRIM,0)));
    rdpq_mode_alphacompare(1);

    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);

    // TODO: Use the common color here
    rdpq_set_prim_color(RGBA32(0x12, 0x2b, 0x3b, 255 * (1.0 - (timeline / START_ANIMATION_TIME))));

    float y_pos = 0.25;

    // Update at 12 fps
    int64_t current_time = timer_ticks();
    if (TIMER_MICROS(current_time - last_frame_time)/1000 >= (1000/12)) {
        // int new_frame = rand() % 4;
        int new_frame = current_frame + 1;
        new_frame = new_frame == current_frame ? (new_frame + 1) : new_frame;
        current_frame = new_frame;
        last_frame_time = current_time;
    }
    rdpq_sprite_blit(title[(current_frame % 4)], display_get_width() * 0.5 - 200.0, display_get_height() * y_pos, NULL);

    rdpq_font_style(font, 0, &(rdpq_fontstyle_t){
        .color = RGBA32(0x12, 0x2b, 0x3b, 255 * (1.0 - (timeline / START_ANIMATION_TIME))),
    });


    rdpq_text_print(NULL, 2,
        display_get_width() * 0.5 + 120, display_get_height() * y_pos + 55
    , "story");

    rdpq_text_print(
        &(rdpq_textparms_t){
            .align = 1,
            .width =  200
        }, 2,
        display_get_width() * 0.5 - 50, display_get_height() * (0.4 + y_pos)
    , "press START to begin");
}