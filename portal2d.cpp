#include <stdio.h>
#include <GLFW/glfw3.h>

#include "math.hpp"
#include "gfx.hpp"


enum ItemType {
    EMPTY,
    FLOOR,
    WALL,
    BLOCK,
    PLAYER,
};


struct GridPosition {
    int x;
    int y;
};

bool operator==(GridPosition a, GridPosition b) {
    return a.x == b.x && a.y == b.y;
}

struct Move {
    int x;
    int y;
};

struct MaybeId {
    int id;
    bool valid;
};

const int max_items = 128;

struct Tilemap {
    GridPosition grid[max_items];
    Vec2 positions[max_items];
    ItemType types[max_items];
    int player_id;

    int n_items = 0;

    int new_id() {
        n_items++;
        assert(n_items < max_items);
        return n_items-1;
    }

    int add_grid_entity(ItemType type, int x, int y) {
        int id = new_id();
        positions[id] = {(float)x, (float)y};
        grid[id] = {x, y};
        types[id] = type;
        return id;
    }

    void add_player() {
        int id = add_grid_entity(PLAYER, 0, 0);
        player_id = id;
    }

    bool is_empty(GridPosition p) {
        for (int id = 0; id < n_items; ++id) {
            if (p == grid[id]) {
                if (types[id] == EMPTY) {
                    return true;
                } else {
                    return false;
                }
            }
        }

        return true;
    }

    MaybeId at(ItemType type, GridPosition p) {
        for (int id = 0; id < n_items; ++id) {
            if (p == grid[id]) {
                if (types[id] == type) {
                    return {.id = id, .valid = true};
                }
            }
        }

        return {.valid = false};
    }

    bool move(int id, Move m) {
        GridPosition p;
        p.x = grid[id].x + m.x;
        p.y = grid[id].y + m.y;

        if (is_empty(p)) {
            return false;
        }

        if (auto wall = at(WALL, p); wall.valid) {
            return false;
        }

        if (auto block = at(BLOCK, p); block.valid) {
            if (!move(block.id, m)) {
                return false;
            }
        }

        grid[id] = p;
        positions[id].x = p.x;
        positions[id].y = p.y;

        return true;
    }

    void move_player(Move m) {
        move(player_id, m);
    }
};

Tilemap tm;


void portal2d_init() {
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x) {
            tm.add_grid_entity(FLOOR, x, y);
        }

    tm.add_grid_entity(WALL, 5, 5);

    tm.add_grid_entity(BLOCK, 3, 1);

    tm.add_player();
}

void portal2d_key_input(int action, int key) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_W: tm.move_player({0, -1}); break;
            case GLFW_KEY_A: tm.move_player({-1, 0}); break;
            case GLFW_KEY_S: tm.move_player({0, 1}); break;
            case GLFW_KEY_D: tm.move_player({1, 0}); break;
        }
    }
}

void portal2d_update(float dt) {
    // update animations
}

void portal2d_draw() {
    float tile_size = 100;
    Vec2 dp = {(float)tile_size, (float)tile_size};

    for (int id = 0; id < tm.n_items; ++id) {
        Vec2 p = tm.positions[id];

        switch (tm.types[id]) {
            case FLOOR: {
                gfx_draw_rectangle(p * tile_size, p * tile_size + dp, {0.5, 0.5, 0.5, 1});
                break;
            }
            case WALL: {
                gfx_draw_rectangle(p * tile_size, p * tile_size + dp, {0.8, 0.8, 0.8, 1});
                break;
            }
            case BLOCK: {
                gfx_draw_rectangle(p * tile_size, p * tile_size + dp, {0.2, 0.2, 0.2, 1});
                break;
            }
            case PLAYER: {
                gfx_draw_point(p * tile_size + dp / 2.f, {0, 0, 0.8, 1}, 0.7 * tile_size / 2.f);
                break;
            }
        }
    }
}
