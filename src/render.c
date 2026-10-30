#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

static struct termios orig_termios;
static bool termios_saved = false;

void terminal_restore(void) {
    if (termios_saved) {
        /* Disable mouse tracking, show cursor, exit alternate buffer */
        write(STDOUT_FILENO, "\033[?1006l\033[?1002l\033[?1000l\033[?25h\033[?1049l", 32);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        termios_saved = false;
    }
}

void terminal_init(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        termios_saved = true;
        atexit(terminal_restore);

        struct termios raw = orig_termios;
        /* Input modes: no break, no CR to NL, no parity, no strip, no flow control */
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        /* Output modes: disable post processing */
        raw.c_oflag &= ~(OPOST);
        /* Control modes: set 8 bit chars */
        raw.c_cflag |= (CS8);
        /* Local modes: echo off, canonical off, extended input off, signal off */
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        /* Control characters: non-blocking read with timeout 0 */
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        /* Enter alternate screen buffer, hide cursor, enable SGR mouse tracking */
        write(STDOUT_FILENO, "\033[?1049h\033[2J\033[H\033[?25l\033[?1000h\033[?1002h\033[?1006h", 38);
    }
}

void terminal_get_size(int *cols, int *rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    } else {
        *cols = 120;
        *rows = 40;
    }
}

rgb_t element_color(uint8_t type, uint8_t tint, uint8_t life) {
    rgb_t c = {0, 0, 0};
    int t = (int)tint - 8; /* -8 to +7 jitter */

    switch (type) {
        case TYPE_EMPTY:
            c.r = 15; c.g = 17; c.b = 23; /* Deep dark slate bg */
            break;
        case TYPE_SAND: {
            int r = 224 + t * 2;
            int g = 192 + t * 2;
            int b = 112 + t;
            c.r = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
            c.g = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
            c.b = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
            break;
        }
        case TYPE_WATER: {
            int r = 40 + t;
            int g = 110 + t * 2;
            int b = 225 + t * 2;
            c.r = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
            c.g = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
            c.b = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
            break;
        }
        case TYPE_LAVA: {
            int r = 255;
            int g = 80 + t * 4;
            int b = 15;
            c.r = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
            c.g = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
            c.b = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
            break;
        }
        case TYPE_ACID: {
            int r = 50 + t * 2;
            int g = 240 + t;
            int b = 60 + t;
            c.r = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
            c.g = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
            c.b = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
            break;
        }
        case TYPE_GUNPOWDER: {
            int v = 95 + t * 2;
            uint8_t val = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
            c.r = val; c.g = val; c.b = (uint8_t)(val + 5);
            break;
        }
        case TYPE_STEAM: {
            int v = 160 + (int)(life * 2) + t;
            uint8_t val = (uint8_t)(v < 0 ? 0 : (v > 240 ? 240 : v));
            c.r = val; c.g = val; c.b = (uint8_t)(val + 10);
            break;
        }
        case TYPE_FIRE: {
            if (life > 20) {
                c.r = 255; c.g = 240; c.b = 100; /* White-hot core */
            } else if (life > 10) {
                c.r = 255; c.g = 140; c.b = 20;  /* Bright orange */
            } else {
                c.r = 200; c.g = 45;  c.b = 10;  /* Dark red ember */
            }
            break;
        }
        case TYPE_STONE: {
            int v = 125 + t * 2;
            uint8_t val = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
            c.r = val; c.g = val; c.b = val;
            break;
        }
        default:
            break;
    }
    return c;
}

render_context_t *render_create(int cols, int rows) {
    render_context_t *rc = (render_context_t *)malloc(sizeof(render_context_t));
    if (!rc) return NULL;
    rc->term_cols = cols;
    rc->term_rows = rows;
    /* Estimate ~60 bytes per character with escape sequences */
    rc->buf_capacity = (size_t)cols * (size_t)rows * 64 + 4096;
    rc->frame_buf = (char *)malloc(rc->buf_capacity);
    rc->buf_len = 0;
    return rc;
}

void render_destroy(render_context_t *rc) {
    if (rc) {
        if (rc->frame_buf) free(rc->frame_buf);
        free(rc);
    }
}

static inline void buf_append(render_context_t *rc, const char *str, size_t len) {
    if (rc->buf_len + len < rc->buf_capacity) {
        memcpy(rc->frame_buf + rc->buf_len, str, len);
        rc->buf_len += len;
    }
}

void render_frame(render_context_t *rc, world_t *world, uint8_t active_element, int brush_size, double fps, bool paused) {
    if (!rc || !world) return;
    rc->buf_len = 0;

    /* Move cursor to home (1,1) */
    buf_append(rc, "\033[H", 3);

    int render_rows = world->height / 2;
    if (render_rows > rc->term_rows - 2) {
        render_rows = rc->term_rows - 2;
    }
    int render_cols = world->width;
    if (render_cols > rc->term_cols) {
        render_cols = rc->term_cols;
    }

    rgb_t last_fg = {255, 255, 255};
    rgb_t last_bg = {0, 0, 0};
    bool has_color = false;

    for (int ty = 0; ty < render_rows; ty++) {
        int y_top = ty * 2;
        int y_bot = y_top + 1;

        for (int x = 0; x < render_cols; x++) {
            cell_t *ct = world_get(world, x, y_top);
            cell_t *cb = world_get(world, x, y_bot);

            rgb_t fg = ct ? element_color(ct->type, ct->tint, ct->life) : (rgb_t){0, 0, 0};
            rgb_t bg = cb ? element_color(cb->type, cb->tint, cb->life) : (rgb_t){0, 0, 0};

            if (!has_color ||
                fg.r != last_fg.r || fg.g != last_fg.g || fg.b != last_fg.b ||
                bg.r != last_bg.r || bg.g != last_bg.g || bg.b != last_bg.b) {
                
                char col_seq[64];
                int slen = snprintf(col_seq, sizeof(col_seq),
                                    "\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm",
                                    fg.r, fg.g, fg.b, bg.r, bg.g, bg.b);
                buf_append(rc, col_seq, (size_t)slen);
                last_fg = fg;
                last_bg = bg;
                has_color = true;
            }

            /* Upper half-block: ▀ */
            buf_append(rc, "\xE2\x96\x80", 3);
        }
        /* Reset attributes at row end and newline */
        buf_append(rc, "\033[0m\r\n", 6);
        has_color = false;
    }

    /* HUD Bar */
    char hud_buf[512];
    int hud_len = snprintf(hud_buf, sizeof(hud_buf),
        "\033[0m\033[7m ALCHEM-C \033[27m [1-8]: \033[1;33m%-9s\033[0m | Brush: \033[1;36m%d\033[0m | FPS: \033[1;32m%4.1f\033[0m | Status: %s | [C]lear [Spc]Pause [Q]uit\033[K\r\n",
        element_name(active_element),
        brush_size,
        fps,
        paused ? "\033[1;31mPAUSED\033[0m" : "\033[1;32mRUNNING\033[0m");
    buf_append(rc, hud_buf, (size_t)hud_len);

    /* Single syscall flush */
    if (rc->buf_len > 0) {
        write(STDOUT_FILENO, rc->frame_buf, rc->buf_len);
    }
}

/* ANSI 24-bit TrueColor mapping table */

/* Single write(STDOUT_FILENO, frame_buf, len) syscall */

/* Grain tint variation: (rand() % 16) */

/* HUD overlay compositor */
