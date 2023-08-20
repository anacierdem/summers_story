#ifndef __MENU_H
#define __MENU_H

#include <libdragon.h>
#include "story.h"

#define START_ANIMATION_TIME 1500.0f
#define START_DELAY 1000.0f

void menu_init(rdpq_font_t *font);
void menu_render(gamestate_t *game_state);
void menu_start_game();

#endif // __MENU_H
