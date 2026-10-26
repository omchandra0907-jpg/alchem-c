#ifndef ALCHEM_TERMINAL_H
#define ALCHEM_TERMINAL_H

#include "world.h"
#include <stdbool.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct {
    int term_cols;
    int term_rows;
    char *frame_buf;
    size_t buf_capacity;
    size_t buf_len;
} render_context_t;

/* Terminal lifecycle */
void terminal_init(void);
void terminal_restore(void);
void terminal_get_size(int *cols, int *rows);

/* Rendering */
render_context_t *render_create(int cols, int rows);
void render_destroy(render_context_t *rc);
void render_frame(render_context_t *rc, world_t *world, uint8_t active_element, int brush_size, double fps, bool paused);
rgb_t element_color(uint8_t type, uint8_t tint, uint8_t life);

/* Input */
typedef enum {
    INPUT_NONE = 0,
    INPUT_QUIT,
    INPUT_PAUSE,
    INPUT_CLEAR,
    INPUT_STEP,
    INPUT_SELECT_ELEMENT,
    INPUT_BRUSH_INC,
    INPUT_BRUSH_DEC,
    INPUT_MOUSE_PAINT,
} input_action_t;

typedef struct {
    input_action_t action;
    int target_x;
    int target_y;
    uint8_t selected_element;
    bool is_dragging;
} input_event_t;

void input_init(void);
bool input_poll(input_event_t *ev);

#endif /* ALCHEM_TERMINAL_H */
