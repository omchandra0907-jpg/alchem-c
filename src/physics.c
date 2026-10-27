#include "world.h"
#include <stdlib.h>
#include <string.h>

const char *element_name(uint8_t type) {
    switch (type) {
        case TYPE_EMPTY:     return "Air";
        case TYPE_SAND:      return "Sand";
        case TYPE_WATER:     return "Water";
        case TYPE_LAVA:      return "Lava";
        case TYPE_ACID:      return "Acid";
        case TYPE_GUNPOWDER: return "Gunpowder";
        case TYPE_STEAM:     return "Steam";
        case TYPE_FIRE:      return "Fire";
        case TYPE_STONE:     return "Stone";
        default:             return "Unknown";
    }
}

cell_t cell_make(uint8_t type) {
    cell_t c;
    c.type = type;
    c.clock = 0;
    c.tint = (uint8_t)(rand() % 16);
    
    switch (type) {
        case TYPE_FIRE:
            c.life = (uint8_t)(15 + (rand() % 15));
            break;
        case TYPE_STEAM:
            c.life = (uint8_t)(30 + (rand() % 25));
            break;
        case TYPE_ACID:
            c.life = (uint8_t)(40 + (rand() % 10));
            break;
        default:
            c.life = 0;
            break;
    }
    return c;
}

world_t *world_create(int width, int height) {
    world_t *w = (world_t *)malloc(sizeof(world_t));
    if (!w) return NULL;
    w->width = width;
    w->height = height;
    w->current_clock = 0;
    size_t count = (size_t)width * (size_t)height;
    w->cells = (cell_t *)calloc(count, sizeof(cell_t));
    if (!w->cells) {
        free(w);
        return NULL;
    }
    return w;
}

void world_destroy(world_t *world) {
    if (world) {
        if (world->cells) free(world->cells);
        free(world);
    }
}

void world_clear(world_t *world) {
    if (!world || !world->cells) return;
    memset(world->cells, 0, (size_t)world->width * (size_t)world->height * sizeof(cell_t));
}

void world_set_cell(world_t *world, int x, int y, uint8_t type) {
    if (!world_in_bounds(world, x, y)) return;
    cell_t c = cell_make(type);
    c.clock = world->current_clock;
    world_set(world, x, y, c);
}

void world_paint_disc(world_t *world, int cx, int cy, int radius, uint8_t type) {
    if (!world) return;
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= r2) {
                int px = cx + dx;
                int py = cy + dy;
                if (world_in_bounds(world, px, py)) {
                    /* Only overwrite if not stone, or if painting empty/stone */
                    cell_t *curr = world_get(world, px, py);
                    if (type == TYPE_EMPTY || type == TYPE_STONE || curr->type == TYPE_EMPTY) {
                        world_set_cell(world, px, py, type);
                    }
                }
            }
        }
    }
}

static void trigger_explosion(world_t *world, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int nx = cx + dx;
                int ny = cy + dy;
                if (world_in_bounds(world, nx, ny)) {
                    cell_t *c = world_get(world, nx, ny);
                    if (c->type == TYPE_GUNPOWDER) {
                        /* Chain reaction */
                        c->type = TYPE_FIRE;
                        c->life = 30;
                    } else if (c->type != TYPE_STONE) {
                        if ((rand() % 3) == 0) {
                            *c = cell_make(TYPE_FIRE);
                        } else {
                            *c = cell_make(TYPE_EMPTY);
                        }
                    }
                }
            }
        }
    }
}

bool world_react(world_t *world, int x1, int y1, int x2, int y2) {
    cell_t *c1 = world_get(world, x1, y1);
    cell_t *c2 = world_get(world, x2, y2);
    if (!c1 || !c2) return false;

    uint8_t t1 = c1->type;
    uint8_t t2 = c2->type;

    /* Lava + Water -> Stone + Steam */
    if ((t1 == TYPE_LAVA && t2 == TYPE_WATER) || (t1 == TYPE_WATER && t2 == TYPE_LAVA)) {
        cell_t stone = cell_make(TYPE_STONE);
        stone.clock = world->current_clock;
        cell_t steam = cell_make(TYPE_STEAM);
        steam.clock = world->current_clock;

        if (t1 == TYPE_LAVA) {
            *c1 = stone;
            *c2 = steam;
        } else {
            *c1 = steam;
            *c2 = stone;
        }
        return true;
    }

    /* Fire + Gunpowder -> Detonation */
    if ((t1 == TYPE_FIRE && t2 == TYPE_GUNPOWDER) || (t1 == TYPE_GUNPOWDER && t2 == TYPE_FIRE)) {
        int ex = (t1 == TYPE_GUNPOWDER) ? x1 : x2;
        int ey = (t1 == TYPE_GUNPOWDER) ? y1 : y2;
        trigger_explosion(world, ex, ey, 3);
        return true;
    }

    /* Lava + Gunpowder -> Detonation */
    if ((t1 == TYPE_LAVA && t2 == TYPE_GUNPOWDER) || (t1 == TYPE_GUNPOWDER && t2 == TYPE_LAVA)) {
        int ex = (t1 == TYPE_GUNPOWDER) ? x1 : x2;
        int ey = (t1 == TYPE_GUNPOWDER) ? y1 : y2;
        trigger_explosion(world, ex, ey, 3);
        return true;
    }

    /* Acid + Corrodibles (Sand, Stone, Gunpowder) */
    if (t1 == TYPE_ACID && (t2 == TYPE_SAND || t2 == TYPE_STONE || t2 == TYPE_GUNPOWDER)) {
        if (c1->life > 0) c1->life -= 5;
        if (c1->life == 0 || (rand() % 4 == 0)) {
            *c1 = cell_make(TYPE_STEAM);
            *c2 = cell_make(TYPE_EMPTY);
        } else {
            *c2 = cell_make(TYPE_EMPTY);
        }
        return true;
    }
    if (t2 == TYPE_ACID && (t1 == TYPE_SAND || t1 == TYPE_STONE || t1 == TYPE_GUNPOWDER)) {
        if (c2->life > 0) c2->life -= 5;
        if (c2->life == 0 || (rand() % 4 == 0)) {
            *c2 = cell_make(TYPE_STEAM);
            *c1 = cell_make(TYPE_EMPTY);
        } else {
            *c1 = cell_make(TYPE_EMPTY);
        }
        return true;
    }

    /* Fire + Water -> Steam */
    if ((t1 == TYPE_FIRE && t2 == TYPE_WATER) || (t1 == TYPE_WATER && t2 == TYPE_FIRE)) {
        *c1 = cell_make(TYPE_STEAM);
        *c2 = cell_make(TYPE_STEAM);
        return true;
    }

    return false;
}

static void update_sand(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;
    int ny = y + 1;
    if (ny >= world->height) return;

    /* Down */
    cell_t *down = world_get(world, x, ny);
    if (down && (down->type == TYPE_EMPTY || down->type == TYPE_WATER)) {
        cell_t temp = *down;
        *down = *c;
        *c = temp;
        return;
    }

    /* Diagonals with alternating bias */
    int dir = (world->current_clock % 2 == 0) ? -1 : 1;
    for (int i = 0; i < 2; i++) {
        int nx = x + dir;
        if (world_in_bounds(world, nx, ny)) {
            cell_t *diag = world_get(world, nx, ny);
            if (diag && (diag->type == TYPE_EMPTY || diag->type == TYPE_WATER)) {
                cell_t temp = *diag;
                *diag = *c;
                *c = temp;
                return;
            }
        }
        dir = -dir;
    }
}

static void update_water(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;

    /* Check reaction with neighbors */
    static const int dxs[] = {0, 0, -1, 1};
    static const int dys[] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
        int nx = x + dxs[i];
        int ny = y + dys[i];
        if (world_in_bounds(world, nx, ny)) {
            if (world_react(world, x, y, nx, ny)) return;
        }
    }

    int ny = y + 1;
    if (ny < world->height) {
        cell_t *down = world_get(world, x, ny);
        if (down && down->type == TYPE_EMPTY) {
            *down = *c;
            *c = cell_make(TYPE_EMPTY);
            return;
        }

        /* Diagonals */
        int dir = ((x + world->current_clock) % 2 == 0) ? -1 : 1;
        for (int i = 0; i < 2; i++) {
            int nx = x + dir;
            if (world_in_bounds(world, nx, ny)) {
                cell_t *diag = world_get(world, nx, ny);
                if (diag && diag->type == TYPE_EMPTY) {
                    *diag = *c;
                    *c = cell_make(TYPE_EMPTY);
                    return;
                }
            }
            dir = -dir;
        }
    }

    /* Horizontal spread */
    int dir = ((x + world->current_clock) % 2 == 0) ? -1 : 1;
    for (int i = 0; i < 2; i++) {
        int nx = x + dir;
        if (world_in_bounds(world, nx, y)) {
            cell_t *side = world_get(world, nx, y);
            if (side && side->type == TYPE_EMPTY) {
                *side = *c;
                *c = cell_make(TYPE_EMPTY);
                return;
            }
        }
        dir = -dir;
    }
}

static void update_lava(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;

    /* Check reactions with neighbors */
    static const int dxs[] = {0, 0, -1, 1};
    static const int dys[] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
        int nx = x + dxs[i];
        int ny = y + dys[i];
        if (world_in_bounds(world, nx, ny)) {
            if (world_react(world, x, y, nx, ny)) return;
        }
    }

    /* Lava moves slower (every 2nd frame) */
    if ((world->current_clock + x + y) % 2 != 0) return;

    int ny = y + 1;
    if (ny < world->height) {
        cell_t *down = world_get(world, x, ny);
        if (down && down->type == TYPE_EMPTY) {
            *down = *c;
            *c = cell_make(TYPE_EMPTY);
            return;
        }

        int dir = (world->current_clock % 2 == 0) ? -1 : 1;
        for (int i = 0; i < 2; i++) {
            int nx = x + dir;
            if (world_in_bounds(world, nx, ny)) {
                cell_t *diag = world_get(world, nx, ny);
                if (diag && diag->type == TYPE_EMPTY) {
                    *diag = *c;
                    *c = cell_make(TYPE_EMPTY);
                    return;
                }
            }
            dir = -dir;
        }
    }

    /* Slow horizontal oozing */
    int dir = (rand() % 2 == 0) ? -1 : 1;
    int nx = x + dir;
    if (world_in_bounds(world, nx, y)) {
        cell_t *side = world_get(world, nx, y);
        if (side && side->type == TYPE_EMPTY) {
            *side = *c;
            *c = cell_make(TYPE_EMPTY);
        }
    }
}

static void update_acid(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;

    /* Check reactions with all neighbors */
    static const int dxs[] = {0, 0, -1, 1};
    static const int dys[] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
        int nx = x + dxs[i];
        int ny = y + dys[i];
        if (world_in_bounds(world, nx, ny)) {
            if (world_react(world, x, y, nx, ny)) return;
        }
    }

    int ny = y + 1;
    if (ny < world->height) {
        cell_t *down = world_get(world, x, ny);
        if (down && down->type == TYPE_EMPTY) {
            *down = *c;
            *c = cell_make(TYPE_EMPTY);
            return;
        }
        int dir = (rand() % 2 == 0) ? -1 : 1;
        int nx = x + dir;
        if (world_in_bounds(world, nx, ny)) {
            cell_t *diag = world_get(world, nx, ny);
            if (diag && diag->type == TYPE_EMPTY) {
                *diag = *c;
                *c = cell_make(TYPE_EMPTY);
                return;
            }
        }
    }

    /* Sideways */
    int dir = (rand() % 2 == 0) ? -1 : 1;
    int nx = x + dir;
    if (world_in_bounds(world, nx, y)) {
        cell_t *side = world_get(world, nx, y);
        if (side && side->type == TYPE_EMPTY) {
            *side = *c;
            *c = cell_make(TYPE_EMPTY);
        }
    }
}

static void update_gunpowder(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;

    /* Check if neighboring heat triggers it */
    static const int dxs[] = {0, 0, -1, 1};
    static const int dys[] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
        int nx = x + dxs[i];
        int ny = y + dys[i];
        if (world_in_bounds(world, nx, ny)) {
            cell_t *nbr = world_get(world, nx, ny);
            if (nbr && (nbr->type == TYPE_FIRE || nbr->type == TYPE_LAVA)) {
                trigger_explosion(world, x, y, 3);
                return;
            }
        }
    }

    /* Falls like sand */
    int ny = y + 1;
    if (ny >= world->height) return;

    cell_t *down = world_get(world, x, ny);
    if (down && (down->type == TYPE_EMPTY || down->type == TYPE_WATER)) {
        cell_t temp = *down;
        *down = *c;
        *c = temp;
        return;
    }

    int dir = ((x + world->current_clock) % 2 == 0) ? -1 : 1;
    for (int i = 0; i < 2; i++) {
        int nx = x + dir;
        if (world_in_bounds(world, nx, ny)) {
            cell_t *diag = world_get(world, nx, ny);
            if (diag && (diag->type == TYPE_EMPTY || diag->type == TYPE_WATER)) {
                cell_t temp = *diag;
                *diag = *c;
                *c = temp;
                return;
            }
        }
        dir = -dir;
    }
}

static void update_fire(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;
    if (c->life == 0) {
        *c = cell_make(TYPE_EMPTY);
        return;
    }
    c->life--;

    /* Ignite neighbors */
    static const int dxs[] = {0, 0, -1, 1, -1, 1, -1, 1};
    static const int dys[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    for (int i = 0; i < 8; i++) {
        int nx = x + dxs[i];
        int ny = y + dys[i];
        if (world_in_bounds(world, nx, ny)) {
            cell_t *nbr = world_get(world, nx, ny);
            if (nbr && nbr->type == TYPE_GUNPOWDER) {
                trigger_explosion(world, nx, ny, 3);
                return;
            }
        }
    }

    /* Rise upwards */
    int ny = y - 1;
    if (ny >= 0) {
        int dir = (rand() % 3) - 1; /* -1, 0, 1 */
        int nx = x + dir;
        if (world_in_bounds(world, nx, ny)) {
            cell_t *target = world_get(world, nx, ny);
            if (target && target->type == TYPE_EMPTY) {
                *target = *c;
                *c = cell_make(TYPE_EMPTY);
                return;
            }
        }
    }
}

static void update_steam(world_t *world, int x, int y, cell_t *c) {
    c->clock = world->current_clock;
    if (c->life == 0) {
        *c = cell_make(TYPE_EMPTY);
        return;
    }
    c->life--;

    /* Rise upwards */
    int ny = y - 1;
    int dir = (rand() % 3) - 1;
    int nx = x + dir;
    if (ny >= 0 && world_in_bounds(world, nx, ny)) {
        cell_t *target = world_get(world, nx, ny);
        if (target && target->type == TYPE_EMPTY) {
            *target = *c;
            *c = cell_make(TYPE_EMPTY);
            return;
        }
    }
    /* Drifting sideways */
    dir = (rand() % 2 == 0) ? -1 : 1;
    nx = x + dir;
    if (world_in_bounds(world, nx, y)) {
        cell_t *side = world_get(world, nx, y);
        if (side && side->type == TYPE_EMPTY) {
            *side = *c;
            *c = cell_make(TYPE_EMPTY);
        }
    }
}

void world_step(world_t *world) {
    if (!world) return;

    /* Flip clock parity bit */
    world->current_clock = !world->current_clock;

    /* Bottom-to-top scan */
    for (int y = world->height - 1; y >= 0; y--) {
        /* Alternate x direction per row to prevent directional bias */
        bool left_to_right = ((y + world->current_clock) % 2 == 0);
        int start_x = left_to_right ? 0 : world->width - 1;
        int end_x = left_to_right ? world->width : -1;
        int step_x = left_to_right ? 1 : -1;

        for (int x = start_x; x != end_x; x += step_x) {
            cell_t *c = world_get(world, x, y);
            if (!c || c->type == TYPE_EMPTY || c->type == TYPE_STONE) continue;

            /* If already updated this tick, skip */
            if (c->clock == world->current_clock) continue;

            switch (c->type) {
                case TYPE_SAND:
                    update_sand(world, x, y, c);
                    break;
                case TYPE_WATER:
                    update_water(world, x, y, c);
                    break;
                case TYPE_LAVA:
                    update_lava(world, x, y, c);
                    break;
                case TYPE_ACID:
                    update_acid(world, x, y, c);
                    break;
                case TYPE_GUNPOWDER:
                    update_gunpowder(world, x, y, c);
                    break;
                case TYPE_FIRE:
                    update_fire(world, x, y, c);
                    break;
                case TYPE_STEAM:
                    update_steam(world, x, y, c);
                    break;
                default:
                    break;
            }
        }
    }
}

/* Water liquid lateral dispersion logic */
