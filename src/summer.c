#include "summer.h"

static terrain_t *m_map;
static surface_t zbuffer;
static player_t player;
static gamestate_t game_state = {
    .good_ending = true, // This will determine the helping text color
};
static bool game_ended = false;

static GLfloat environment_color[] = { 0.53, 0.8, 0.92, 1.0 };
static GLfloat orig_environment_color[] = { 0.53, 0.8, 0.92, 1.0 };
static GLfloat water_transform[16] = { 0 };

#ifndef NDEBUG
static float gFPS;
static float delta_time;
static unsigned int old_time;

static void calculate_framerate(void) {
    static unsigned int curFrameTimeIndex = 0;
    static unsigned int frameTimes[30];
    unsigned int newTime = timer_ticks();
    unsigned int oldTime = frameTimes[curFrameTimeIndex];
    frameTimes[curFrameTimeIndex] = newTime;

    curFrameTimeIndex++;
    if (curFrameTimeIndex >= 30) {
        curFrameTimeIndex = 0;
    }
    delta_time = TIMER_MICROS(newTime - old_time) / 1000000.0f;
    old_time = newTime;
    gFPS = (30.0f * 1000000.0f) / TIMER_MICROS(newTime - oldTime);
}
#endif

static void system_init(surface_t *zbuffer)
{
    int seed = TICKS_READ();
    srand(seed);

    console_init();
    debug_init_usblog();
    debug_init_isviewer();
    console_set_debug(true);

    display_init(RESOLUTION_640x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    rdpq_init();
#ifndef NDEBUG
    rdpq_debug_start();
    // rdpq_debug_log(true);
#endif
    gl_init();
    dfs_init(DFS_DEFAULT_LOCATION);
    controller_init();
    timer_init();

    audio_init(44100, 6);
    mixer_init(4);

    *zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
}

static void gl_configure(void) {
    glEnable(GL_MULTISAMPLE_ARB);
    glDepthFunc(GL_LESS_INTERPENETRATING_N64);

    glEnable(GL_FOG);

    glFogf(GL_FOG_START, 0.3);
    glFogf(GL_FOG_END, RENDERING_DISTANCE);
    glFogfv(GL_FOG_COLOR, environment_color);

    glDisable(GL_ALPHA_TEST);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Init projection
    glMatrixMode(GL_PROJECTION);
    float aspect_ratio = 0.5 * (float)display_get_width() / (float)display_get_height();
    float near_plane = 0.5f;
    float angleOfView = 50;
    glLoadIdentity();
    gluPerspective(angleOfView, aspect_ratio, near_plane, RENDERING_DISTANCE);
}

GLuint water_idx = 0;
void setup(void)
{
    m_map = terrain_load("rom:/map.terrain");
    rdpq_font_t *font_small = rdpq_font_load("rom:/font_small.font64");
    rdpq_font_t *font_big = rdpq_font_load("rom:/font_big.font64");

    rdpq_fontstyle_t font_style = {
        .color = RGBA32(0x12, 0x2b, 0x3b, 0xFF),
    };

    rdpq_font_style(font_small, 0, &(rdpq_fontstyle_t){
        .color = RGBA32(0, 0, 0, 0xFF),
    });

    rdpq_font_style(font_small, 1, &font_style);

    rdpq_font_style(font_small, 2, &(rdpq_fontstyle_t){
        .color = RGBA32(0xFF, 0xFF, 0xFF, 0xFF),
    });

    rdpq_font_style(font_big, 0, &font_style);

    rdpq_text_register_font(1, font_small);
    rdpq_text_register_font(2, font_big);

    assertf(m_map != NULL, "Map not loaded");
    for (size_t i = 0; i < m_map->num_meshes; i++) {
        if (strcmp(m_map->meshes[i].name, "water") == 0) {
            water_idx = glGenLists(1);
            assertf(water_idx, "Unable to create water dlist");
            memcpy(water_transform, m_map->meshes[i].transform, sizeof(water_transform));
            glNewList(water_idx, GL_COMPILE);
                terrain_draw_mesh(&m_map->meshes[i]);
            glEndList();
        }
    }

    gl_configure();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    player_init(&player, &game_state, m_map);
    dialogue_init();
    story_init(m_map, &game_state);

    menu_init(font_big);
    tutorial_init();
}

#ifdef ENABLE_ROTATION
static float get_angle() {
    static long long last_time = 0;
    static float start = 0;
    static float end = 0;
    static float duration = 0;
    float timeline = duration == 0 ? 1.0f : (float)(TIMER_MICROS(timer_ticks() - last_time)/1000) / duration;
    timeline = timeline > 1.0f ? 1.0f : timeline;

    static float angle = 0;

    if (timeline >= 1.0f) {
        duration = 3000 + 3000 * (float)(rand()) / (float)(RAND_MAX);
        start = angle;
        end = 360 * (float)(rand()) / (float)(RAND_MAX);
        // on Ares, timer_ticks() overflows, so we always reset last_time
        timeline = .0f;
        last_time = timer_ticks();
        // debugf("s: %f e: %f\n", start, end);
    }
    angle = start + timeline * (end - start);
    return angle;
}
#endif

static float get_offset() {
    float intensity = 0.15f;

    static long long last_time = 0;
    static float start = 0;
    static float end = 0;
    static float duration = 0;
    float timeline = duration == 0 ? 1.0f : (float)(TIMER_MICROS(timer_ticks() - last_time)/1000) / duration;
    timeline = timeline > 1.0f ? 1.0f : timeline;

    static float offset = 0;

    // on Ares, timer_ticks() overflows, reset when timeline is negative
    if (timeline >= 1.0f || timeline < 0.0f) {
        duration = 1000 + 1000 * (float)(rand()) / (float)(RAND_MAX);
        start = offset;
        end = intensity * (float)(rand()) / (float)(RAND_MAX);
        timeline = .0f;
        last_time = timer_ticks();
    }
    offset = start + timeline * (end - start);
    return offset;
}

void update_world(void)
{
    if (game_ended) {
        return;
    }

    if (game_state.game_end_ticks) {
        float timeline;

        if (game_ended) {
            timeline = 1.0f;
        } else {
            timeline = (float)(TIMER_MICROS(timer_ticks() - game_state.game_end_ticks)/1000) / 3000.0f;
            if (timeline >= 1) {
                // on Ares, timer_ticks() overflows, explicitly end
                game_ended = true;
                story_end(&game_state);
                timeline = 1.0f;
            }
        }

        // Fade to black/white
        if  (game_state.good_ending) {
            float target_color = game_state.good_ending ? 1.0f : 0.0f;
            environment_color[0] = orig_environment_color[0] * (1.0f - timeline) + timeline * target_color;
            environment_color[1] = orig_environment_color[1] * (1.0f - timeline) + timeline * target_color;
            environment_color[2] = orig_environment_color[2] * (1.0f - timeline) + timeline * target_color;
        } else {
            if ((1.0f - timeline) > 0.01f && environment_color[0] > 0.01f) {
                environment_color[0] *= (1.0f - timeline);
                environment_color[2] *= (1.0f - timeline);
                environment_color[1] *= (1.0f - timeline);
            } else {
                environment_color[0] = 0.0f;
                environment_color[1] = 0.0f;
                environment_color[2] = 0.0f;
            }
        }

        glFogf(GL_FOG_END, RENDERING_DISTANCE - RENDERING_DISTANCE * timeline + 0.4f);
        glFogfv(GL_FOG_COLOR, environment_color);
    }

    game_state.water_height = water_transform[13] + get_offset();
}

void render_world(void)
{
    if (game_ended) {
        return;
    }

    glDepthFunc(GL_LESS_INTERPENETRATING_N64);
    terrain_draw_mesh(&m_map->meshes[0]);

    glPushMatrix();
        glTranslatef(player.x, game_state.water_height, player.z);

#ifdef ENABLE_ROTATION
        glRotatef(get_angle(), 0, 1, 0);
#endif

        glCallList(water_idx);
    glPopMatrix();
}

int main(void)
{
    system_init(&zbuffer);

    setup();

    intro_show();

    while (1)
    {
        if (audio_can_write()) {
            short *buf = audio_write_begin();
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }

        controller_scan();

        surface_t *disp = display_get();
        rdpq_attach(disp, &zbuffer);
        gl_context_begin();

        glClearColor(environment_color[0], environment_color[1], environment_color[2], environment_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // FIXME: Update orders cause collision intersections for the current frame
        glPushMatrix();
            update_world();
            player_transform(&player);
            bool suppress_key = story_render(&game_state, &player);
            render_world();
        glPopMatrix();

        player_update(&player, delta_time, suppress_key);

        gl_context_end();

        // This is primarily for ares
        if (audio_can_write()) {
            short *buf = audio_write_begin();
            mixer_poll(buf, audio_get_buffer_length());
            audio_write_end();
        }

        // Render UI
        if(!game_state.started) menu_render(&game_state); 
        if(game_state.started) {
            dialogue_render(&game_state, suppress_key);
            tutorial_render(&game_state);
        }

        rdpq_detach_show();

#ifndef NDEBUG
        calculate_framerate();
        debugf("FPS: %f\n", gFPS);
        rdpq_debug_log(false);
#endif
    }

    gl_close();
    return 0;
}