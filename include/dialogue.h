#ifndef __DIALOGUE_H
#define __DIALOGUE_H

#include <libdragon.h>
#include <stdbool.h>

#include "utils.h"
#include "summer.h"

#define DIALOGUE_DELAY 25
#define DIALOGUE_DELAY_FAST 5
#define DIALOGUE_FONT_SIZE 12
#define DIALOGUE_HEIGHT 3

typedef struct gamestate_s gamestate_t;

typedef struct dialogue_s {
    char *text;
    // TODO: make this a count + text pair?
    struct dialogue_s *next;
    bool _played;
} dialogue_t;

void dialogue_init();
void dialogue_render(gamestate_t *game_state, bool supress);
bool dialogue_set_line(dialogue_t *line); // Returns whether it is shown or not

#endif // __DIALOGUE_H
