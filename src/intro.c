#include "intro.h"

char *sprites[2] = {"rom:/libdragon.sprite", "rom:/sandwich.sprite"};
static int64_t start_time;

void intro_show() {
    for (int i = 0; i < sizeof(sprites) / sizeof(char*); i++) {
        start_time = timer_ticks();
        sprite_t *sprite = sprite_load(sprites[i]);
        assertf(sprite, "missing intro sprite");

        surface_t *disp = display_get();
        rdpq_attach_clear(disp, NULL);

        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,TEX0), (0,0,0,TEX0)));

        // Polygon sandwich
        rdpq_sprite_blit(sprite, 0, 0, NULL);
        rdpq_detach_show();

        float timeline = 0.0f;
        do { timeline = (float)(TIMER_MICROS(timer_ticks() - start_time)/1000); }
        while (timeline < INTRO_STEP_TIME);
        sprite_free(sprite);
    }

    // rdpq_font_style(font, 0, &(rdpq_fontstyle_t){
    //     .color = RGBA32(0x12, 0x2b, 0x3b, 255 * (1.0 - (timeline / START_ANIMATION_TIME))),
    // });


    // rdpq_text_print(NULL, 2,
    //     display_get_width() * 0.5 + 120, display_get_height() * y_pos + 55
    // , "story");

    // rdpq_text_print(
    //     &(rdpq_textparms_t){
    //         .align = 1,
    //         .width =  200
    //     }, 2,
    //     display_get_width() * 0.5 - 50, display_get_height() * (0.4 + y_pos)
    // , "press START to begin");
}
