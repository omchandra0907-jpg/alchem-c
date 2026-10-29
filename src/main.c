#define _POSIX_C_SOURCE 199309L
#include "world.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

static volatile bool running = true;

static void handle_sigint(int sig) {
    (void)sig;
    running = false;
}

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void sleep_sec(double s) {
    if (s <= 0.0) return;
    struct timespec req;
    req.tv_sec = (time_t)s;
    req.tv_nsec = (long)((s - (double)req.tv_sec) * 1e9);
    nanosleep(&req, NULL);
}

int main(int argc, char *argv[]) {
    bool demo_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--demo") == 0) {
            demo_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("alchem-c: Terminal Falling-Sand & Chemical Reaction Engine\n");
            printf("Usage: %s [--demo] [--help]\n\n", argv[0]);
            printf("Controls:\n");
            printf("  [Mouse Drag]   Paint element on canvas\n");
            printf("  [1] Sand       [2] Water     [3] Lava        [4] Acid\n");
            printf("  [5] Gunpowder  [6] Fire      [7] Stone       [8] Eraser\n");
            printf("  [+/-]          Increase / Decrease brush size\n");
            printf("  [Space]        Pause / Resume simulation\n");
            printf("  [C]            Clear world\n");
            printf("  [Q]            Quit\n");
            return 0;
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    int cols, rows;
    terminal_get_size(&cols, &rows);
    if (cols < 40) cols = 80;
    if (rows < 20) rows = 30;

    int world_w = cols;
    int world_h = (rows - 2) * 2; /* 2 cells per terminal row */
    if (world_h < 20) world_h = 40;

    world_t *world = world_create(world_w, world_h);
    if (!world) {
        fprintf(stderr, "Failed to allocate world grid\n");
        return 1;
    }

    render_context_t *rc = render_create(cols, rows);
    if (!rc) {
        world_destroy(world);
        fprintf(stderr, "Failed to allocate render buffer\n");
        return 1;
    }

    terminal_init();
    input_init();

    uint8_t active_element = TYPE_SAND;
    int brush_size = 2;
    bool paused = false;

    const double target_dt = 1.0 / 60.0;
    double fps = 60.0;
    int frame_count = 0;

    /* Demo mode preset initialization */
    if (demo_mode) {
        /* Stone funnel */
        for (int y = 5; y < 25; y++) {
            world_set_cell(world, 30 + (y - 5), y, TYPE_STONE);
            world_set_cell(world, 90 - (y - 5), y, TYPE_STONE);
        }
        /* Obstacles & Gunpowder cache */
        world_paint_disc(world, 60, 45, 6, TYPE_GUNPOWDER);
        world_paint_disc(world, 35, 45, 4, TYPE_WATER);
        world_paint_disc(world, 85, 45, 4, TYPE_LAVA);
    }

    while (running) {
        double frame_start = get_time_sec();

        /* Input polling */
        input_event_t ev;
        while (input_poll(&ev)) {
            switch (ev.action) {
                case INPUT_QUIT:
                    running = false;
                    break;
                case INPUT_PAUSE:
                    paused = !paused;
                    break;
                case INPUT_CLEAR:
                    world_clear(world);
                    break;
                case INPUT_STEP:
                    world_step(world);
                    break;
                case INPUT_SELECT_ELEMENT:
                    active_element = ev.selected_element;
                    break;
                case INPUT_BRUSH_INC:
                    if (brush_size < 8) brush_size++;
                    break;
                case INPUT_BRUSH_DEC:
                    if (brush_size > 1) brush_size--;
                    break;
                case INPUT_MOUSE_PAINT:
                    world_paint_disc(world, ev.target_x, ev.target_y, brush_size, active_element);
                    break;
                default:
                    break;
            }
        }

        /* Demo mode scripted simulation flow */
        if (demo_mode) {
            frame_count++;
            if (frame_count % 3 == 0 && frame_count < 250) {
                /* Spout sand & water from top */
                world_paint_disc(world, 55, 2, 2, TYPE_SAND);
                world_paint_disc(world, 65, 2, 2, TYPE_WATER);
            } else if (frame_count >= 250 && frame_count < 380) {
                if (frame_count % 5 == 0) {
                    world_paint_disc(world, 60, 2, 2, TYPE_LAVA);
                }
            } else if (frame_count == 390) {
                /* Ignite gunpowder cache */
                world_set_cell(world, 60, 39, TYPE_FIRE);
            } else if (frame_count > 600) {
                /* Demo showcase complete */
                running = false;
            }
        }

        /* Physics update */
        if (!paused) {
            world_step(world);
        }

        /* Render frame */
        render_frame(rc, world, active_element, brush_size, fps, paused);

        /* Timestep regulation */
        double frame_end = get_time_sec();
        double elapsed = frame_end - frame_start;
        double sleep_time = target_dt - elapsed;
        if (sleep_time > 0.0) {
            sleep_sec(sleep_time);
        }

        double actual_dt = get_time_sec() - frame_start;
        if (actual_dt > 0.0) {
            double cur_fps = 1.0 / actual_dt;
            fps = 0.9 * fps + 0.1 * cur_fps;
        }
    }

    render_destroy(rc);
    world_destroy(world);
    terminal_restore();

    return 0;
}
