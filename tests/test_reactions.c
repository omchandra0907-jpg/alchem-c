#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void test_cell_size(void) {
    printf("[TEST] Checking cell_t memory footprint... ");
    assert(sizeof(cell_t) == 4);
    printf("PASSED (sizeof == 4 bytes)\n");
}

static void test_sand_gravity(void) {
    printf("[TEST] Checking sand gravity... ");
    world_t *w = world_create(20, 20);
    assert(w != NULL);

    world_set_cell(w, 10, 5, TYPE_SAND);
    assert(world_get(w, 10, 5)->type == TYPE_SAND);

    world_step(w);
    assert(world_get(w, 10, 5)->type == TYPE_EMPTY);
    assert(world_get(w, 10, 6)->type == TYPE_SAND);

    world_destroy(w);
    printf("PASSED\n");
}

static void test_sand_water_density(void) {
    printf("[TEST] Checking sand sinking through water... ");
    world_t *w = world_create(20, 20);

    /* Solid floor at y = 11 so water cannot simply fall down */
    for (int x = 8; x <= 12; x++) {
        world_set_cell(w, x, 11, TYPE_STONE);
    }
    /* Side walls so water cannot spread out */
    world_set_cell(w, 9, 10, TYPE_STONE);
    world_set_cell(w, 11, 10, TYPE_STONE);

    world_set_cell(w, 10, 10, TYPE_WATER);
    world_set_cell(w, 10, 9, TYPE_SAND);

    world_step(w);

    /* Sand should sink, water displaced upwards */
    assert(world_get(w, 10, 10)->type == TYPE_SAND);
    assert(world_get(w, 10, 9)->type == TYPE_WATER);

    world_destroy(w);
    printf("PASSED\n");
}

static void test_lava_water_reaction(void) {
    printf("[TEST] Checking lava + water reaction... ");
    world_t *w = world_create(20, 20);

    world_set_cell(w, 10, 10, TYPE_LAVA);
    world_set_cell(w, 10, 11, TYPE_WATER);

    world_step(w);

    /* Reaction should turn lava into stone and water into steam */
    cell_t *c1 = world_get(w, 10, 10);
    cell_t *c2 = world_get(w, 10, 11);
    bool has_stone = (c1->type == TYPE_STONE || c2->type == TYPE_STONE);
    bool has_steam = (c1->type == TYPE_STEAM || c2->type == TYPE_STEAM);
    assert(has_stone && has_steam);

    world_destroy(w);
    printf("PASSED\n");
}

static void test_gunpowder_explosion(void) {
    printf("[TEST] Checking fire + gunpowder explosion... ");
    world_t *w = world_create(30, 30);

    /* Place a gunpowder block */
    for (int y = 14; y <= 16; y++) {
        for (int x = 14; x <= 16; x++) {
            world_set_cell(w, x, y, TYPE_GUNPOWDER);
        }
    }
    /* Touch with fire */
    world_set_cell(w, 13, 15, TYPE_FIRE);

    world_step(w);

    /* Gunpowder should ignite / explode into fire */
    int fire_count = 0;
    for (int y = 12; y <= 18; y++) {
        for (int x = 12; x <= 18; x++) {
            if (world_get(w, x, y)->type == TYPE_FIRE) {
                fire_count++;
            }
        }
    }
    assert(fire_count > 0);

    world_destroy(w);
    printf("PASSED (blast ignited %d fire particles)\n", fire_count);
}

static void test_acid_corrosion(void) {
    printf("[TEST] Checking acid corrosion... ");
    world_t *w = world_create(20, 20);

    world_set_cell(w, 10, 10, TYPE_STONE);
    world_set_cell(w, 10, 9, TYPE_ACID);

    /* Step multiple times to allow acid reaction to complete */
    for (int i = 0; i < 5; i++) {
        world_step(w);
    }

    /* Stone should be corroded or destroyed */
    cell_t *c = world_get(w, 10, 10);
    assert(c->type != TYPE_STONE || c->life < 40);

    world_destroy(w);
    printf("PASSED\n");
}

int main(void) {
    printf("=== ALCHEM-C REACTION & PHYSICS TEST SUITE ===\n");
    test_cell_size();
    test_sand_gravity();
    test_sand_water_density();
    test_lava_water_reaction();
    test_gunpowder_explosion();
    test_acid_corrosion();
    printf("ALL ALCHEM-C TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
