#ifndef ALCHEM_WORLD_H
#define ALCHEM_WORLD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ALCHEM_DEFAULT_WIDTH  120
#define ALCHEM_DEFAULT_HEIGHT 60

typedef enum {
    TYPE_EMPTY = 0,
    TYPE_SAND,
    TYPE_WATER,
    TYPE_LAVA,
    TYPE_ACID,
    TYPE_GUNPOWDER,
    TYPE_STEAM,
    TYPE_FIRE,
    TYPE_STONE,
    TYPE_COUNT
} element_type_t;

/* Compact 4-byte cell struct */
typedef struct {
    uint8_t type;   /* Element ID */
    uint8_t clock;  /* Parity bit to prevent double-update */
    uint8_t life;   /* Burn/decay/dissolve counter */
    uint8_t tint;   /* Grain/color jitter (0..15) */
} cell_t;

typedef struct {
    int width;
    int height;
    uint8_t current_clock;
    cell_t *cells;
} world_t;

/* World Lifecycle */
world_t *world_create(int width, int height);
void world_destroy(world_t *world);
void world_clear(world_t *world);

/* Cell access */
static inline int world_index(const world_t *world, int x, int y) {
    return y * world->width + x;
}

static inline bool world_in_bounds(const world_t *world, int x, int y) {
    return (x >= 0 && x < world->width && y >= 0 && y < world->height);
}

static inline cell_t *world_get(world_t *world, int x, int y) {
    if (!world_in_bounds(world, x, y)) return NULL;
    return &world->cells[world_index(world, x, y)];
}

static inline void world_set(world_t *world, int x, int y, cell_t cell) {
    if (world_in_bounds(world, x, y)) {
        world->cells[world_index(world, x, y)] = cell;
    }
}

cell_t cell_make(uint8_t type);
void world_set_cell(world_t *world, int x, int y, uint8_t type);
void world_paint_disc(world_t *world, int cx, int cy, int radius, uint8_t type);

/* Physics & Reactions */
void world_step(world_t *world);
bool world_react(world_t *world, int x1, int y1, int x2, int y2);

/* Metadata */
const char *element_name(uint8_t type);

#endif /* ALCHEM_WORLD_H */

/* Cache-aligned 1D linear buffer mapping */
