#include <stdio.h>
#include <GLFW/glfw3.h>

#include "math.hpp"
#include "gfx.hpp"
#include "utility.hpp"
#include "image.hpp"


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

struct Portals {
    int tunnel[4] = {-1, -1, -1, -1};
};

const int max_items = 128;
const int tile_size = 100;

unsigned int move_to_face(Move m) {
    assert(m.x != 0 || m.y != 0);
    if (m.x > 0) {
        return 3;
    } else if (m.x < 0) {
        return 1;
    } else if (m.y > 0) {
        return 0;
    } else if (m.y < 0) {
        return 2;
    }
}

Move face_to_move(unsigned int face) {
    assert(face < 4);
    switch (face) {
        case 0: return {0, -1};
        case 1: return {1, 0};
        case 2: return {0, 1};
        case 3: return {-1, 0};
    }
}

struct Tilemap {
    GridPosition grid[max_items];
    ItemType types[max_items];
    Portals portals[max_items];
    Sprite sprites[20];
    Affine transforms[max_items];

    int n_items = 0;

    int new_id() {
        n_items++;
        assert(n_items < max_items);
        return n_items-1;
    }

    int add_grid_entity(ItemType type, int x, int y) {
        int id = new_id();
        transforms[id] = from_translation({(float)x, (float)y});
        grid[id] = {x, y};
        types[id] = type;
        return id;
    }
};

Tilemap tm;
int player_id;

struct Crop {
    int x;
    int y;
    int w;
    int h;
};

void add_player() {
    int id = tm.add_grid_entity(PLAYER, 0, 0);
    player_id = id;
    tm.transforms[id] = mul(tm.transforms[id], from_scale(0.05));
}

bool is_empty(GridPosition p) {
    for (int id = 0; id < tm.n_items; ++id) {
        if (p == tm.grid[id]) {
            if (tm.types[id] == EMPTY) {
                return true;
            } else {
                return false;
            }
        }
    }

    return true;
}

MaybeId at(ItemType type, GridPosition p) {
    for (int id = 0; id < tm.n_items; ++id) {
        if (p == tm.grid[id]) {
            if (tm.types[id] == type) {
                return {.id = id, .valid = true};
            }
        }
    }

    return {.valid = false};
}

struct PortalFace {
    int item_id;
    int face;
};

PortalFace find_exit_face(PortalFace from) {
    int tunnel_id = tm.portals[from.item_id].tunnel[from.face];
    for (int item_id = 0; item_id < max_items; ++item_id) {
        if (tm.types[item_id] == WALL) {
            for (int other_face = 0; other_face < 4; ++other_face) {
                if (item_id == from.item_id and other_face == from.face) continue;
                if (tm.portals[item_id].tunnel[other_face] == tunnel_id) {
                    return {.item_id = item_id, .face = other_face};
                }
            }
        }
    }

    assert(false);
}

bool move(int id, Move m, GridPosition from) {
    GridPosition p;
    p.x = from.x + m.x;
    p.y = from.y + m.y;

    if (is_empty(p)) {
        return false;
    }

    if (auto wall = at(WALL, p); wall.valid) {
        auto face = move_to_face(m);
        auto tunnel_id = tm.portals[wall.id].tunnel[face];
        if (tunnel_id >= 0) {
            printf("teleport through tunnel %d\n", tunnel_id);

            auto other_portal = find_exit_face({.item_id = wall.id, .face = face});
            Move next_move = face_to_move(other_portal.face);
            return move(id, next_move, tm.grid[other_portal.item_id]);
        }
        return false;
    }

    if (auto block = at(BLOCK, p); block.valid) {
        if (!move(block.id, m, tm.grid[block.id])) {
            return false;
        }
    }

    tm.grid[id] = p;
    tm.transforms[id].t = {p.x, p.y};

    return true;
}

void move_player(Move m) {
    move(player_id, m, tm.grid[player_id]);
}

Sprite sprite_from_png(PngImage im, Crop roi) {
    Sprite s;
    s.w = roi.w;
    s.h = roi.h;
    s.data = (RGBA*)malloc(s.w*s.h*sizeof(RGBA));

    for (int crop_y = roi.y; crop_y < roi.y + roi.w; ++crop_y) 
        for (int crop_x = roi.x; crop_x < roi.x + roi.w; ++crop_x) {
            RGBA c;
            c.r = im.data[crop_y * im.width * 4 + crop_x*4 + 0];
            c.g = im.data[crop_y * im.width * 4 + crop_x*4 + 1];
            c.b = im.data[crop_y * im.width * 4 + crop_x*4 + 2];
            c.a = im.data[crop_y * im.width * 4 + crop_x*4 + 3];
            int sprite_x = crop_x - roi.x;
            int sprite_y = crop_y - roi.y;
            s.data[sprite_y * roi.w + sprite_x] = c;
        }

    return s;
}

Sprite load_player_sprite() {
    return sprite_from_png(read_png("resources/sprites.png"), Crop{.x = 0, .y = 0, .w = 16, .h = 16});
}

void portal2d_init() {
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x) {
            tm.add_grid_entity(FLOOR, x, y);
        }

    int wall_id = tm.add_grid_entity(WALL, 5, 5);
    tm.portals[wall_id].tunnel[0] = 0;
    tm.portals[wall_id].tunnel[1] = 1;
    tm.portals[wall_id].tunnel[2] = 1;

    int wall_id2 = tm.add_grid_entity(WALL, 2, 5);
    tm.portals[wall_id2].tunnel[0] = 0;

    tm.add_grid_entity(BLOCK, 3, 1);

    add_player();

    tm.sprites[FLOOR].w = 1;
    tm.sprites[FLOOR].h = 1;
    static RGBA floor_c[] = {
        {0.5, 0.5, 0.5, 1},
    };
    tm.sprites[FLOOR].data = floor_c;

    tm.sprites[WALL].w = 1;
    tm.sprites[WALL].h = 1;
    static RGBA wall_c[] = {
        {0.8, 0.8, 0.8, 1},
    };
    tm.sprites[WALL].data = wall_c;

    tm.sprites[BLOCK].w = 1;
    tm.sprites[BLOCK].h = 1;
    static RGBA block_c[] = {
        {0.2, 0.2, 0.2, 1},
    };
    tm.sprites[BLOCK].data = block_c;

    tm.sprites[PLAYER] = load_player_sprite();
}

void portal2d_key_input(int action, int key) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_UP: move_player({0, -1}); break;
            case GLFW_KEY_LEFT: move_player({-1, 0}); break;
            case GLFW_KEY_DOWN: move_player({0, 1}); break;
            case GLFW_KEY_RIGHT: move_player({1, 0}); break;
        }
    }
}

void portal2d_update(float dt) {
    // update animations
}

void portal2d_draw() {
    Vec2 dp = {(float)tile_size, (float)tile_size};

    Affine view_transform = from_scale(tile_size);
    for (int id = 0; id < tm.n_items; ++id) {
        // Vec2 p = tm.positions[id];
        ItemType type = tm.types[id];
        // if (type == PLAYER) {
        //     t = mul(from_translation(p * tile_size), from_scale(5));
        // }
        gfx_draw_sprite(tm.sprites[type], mul(view_transform, tm.transforms[id]), false);
    }
}
