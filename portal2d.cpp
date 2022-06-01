#include <GLFW/glfw3.h>
#include <stdio.h>

#include "gfx.hpp"
#include "image.hpp"
#include "math.hpp"
#include "utility.hpp"

enum ItemType {
    EMPTY,
    FLOOR,
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

bool operator==(Move a, Move b) {
    return a.x == b.x && a.y == b.y;
}

struct MaybeId {
    int id;
    bool valid;
};

struct Portals {
    int tunnel[4] = {-1, -1, -1, -1};
};

const int max_items = 128;
const int tile_size = 100;
const float tile_sizef = 100;

int move_to_face(Move m) {
    assert(m.x != 0 || m.y != 0);
    if (m.x > 0)
        return 3;
    else if (m.x < 0)
        return 1;
    else if (m.y > 0)
        return 0;
    else if (m.y < 0)
        return 2;
}

Move face_to_move(int face) {
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

    int max_id = 0;

    int new_id() {
        max_id++;
        assert(max_id < max_items);
        return max_id - 1;
    }

    int add_grid_entity(ItemType type, int x, int y) {
        int id = new_id();
        transforms[id] = multiply_affine(from_scale(tile_sizef), from_translation({(float)x, (float)y}));
        grid[id] = {x, y};
        types[id] = type;
        return id;
    }

    void remove(int id) { types[id] = EMPTY; }
};

struct Editor {
    Vec2 mouse;
    int selected = -1;
};

Tilemap tm;
int player_id;
bool editor_mode = true;
Editor editor;

bool is_empty(GridPosition p) {
    for (int id = 0; id < tm.max_id; ++id) {
        if (p == tm.grid[id]) {
            if (tm.types[id] != EMPTY) {
                return false;
            }
        }
    }

    return true;
}

MaybeId at(ItemType type, GridPosition p) {
    for (int id = 0; id < tm.max_id; ++id) {
        if (p == tm.grid[id]) {
            if (tm.types[id] == type) {
                return {.id = id, .valid = true};
            }
        }
    }

    return {.valid = false};
}

MaybeId item_at(GridPosition p) {
    for (int id = 0; id < tm.max_id; ++id) {
        if (p == tm.grid[id]) {
            if (tm.types[id] != FLOOR && tm.types[id] != EMPTY) {
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
        if (tm.types[item_id] == BLOCK) {
            for (int other_face = 0; other_face < 4; ++other_face) {
                if (item_id == from.item_id and other_face == from.face)
                    continue;
                if (tm.portals[item_id].tunnel[other_face] == tunnel_id) {
                    return {.item_id = item_id, .face = other_face};
                }
            }
        }
    }

    assert(false);
}

void set_item_position(int id, GridPosition p) {
    tm.grid[id] = p;
    tm.transforms[id].t = {(float)tile_size * p.x, (float)tile_size * p.y};
}

bool move(int id, Move m, GridPosition from) {
    GridPosition p;
    p.x = from.x + m.x;
    p.y = from.y + m.y;

    if (is_empty(p)) {
        return false;
    }

    if (auto block = at(BLOCK, p); block.valid) {
        auto face = move_to_face(m);
        auto tunnel_id = tm.portals[block.id].tunnel[face];
        if (tunnel_id >= 0) {
            printf("teleport through tunnel %d\n", tunnel_id);

            auto other_portal = find_exit_face({.item_id = block.id, .face = face});
            Move next_move = face_to_move(other_portal.face);

            // Don't return if false
            if (move(id, next_move, tm.grid[other_portal.item_id])) {
                
                Move rotated = m;
                auto tunnels = tm.portals[id].tunnel;
                for (int r = 0; r < 4; ++r) {
                    if (rotated == next_move) {
                        break;
                    }
                    rotated = {.x = -rotated.y, .y = rotated.x};
                    
                    int t0 = tunnels[0];
                    tunnels[0] = tunnels[3];
                    tunnels[3] = tunnels[2];
                    tunnels[2] = tunnels[1];
                    tunnels[1] = t0;
                }
                return true;
            }
        }
        if (!move(block.id, m, tm.grid[block.id])) {
            return false;
        }
    }

    set_item_position(id, p);

    return true;
}

void move_player(Move m) {
    move(player_id, m, tm.grid[player_id]);
}

struct Crop {
    int x;
    int y;
    int w;
    int h;
};

Sprite sprite_from_png(PngImage im, Crop roi) {
    Sprite s;
    s.w = roi.w;
    s.h = roi.h;
    s.data = (RGBA*)malloc(s.w * s.h * sizeof(RGBA));

    for (int crop_y = roi.y; crop_y < roi.y + roi.w; ++crop_y)
        for (int crop_x = roi.x; crop_x < roi.x + roi.w; ++crop_x) {
            RGBA c;
            c.r = im.data[crop_y * im.width * 4 + crop_x * 4 + 0];
            c.g = im.data[crop_y * im.width * 4 + crop_x * 4 + 1];
            c.b = im.data[crop_y * im.width * 4 + crop_x * 4 + 2];
            c.a = im.data[crop_y * im.width * 4 + crop_x * 4 + 3];
            int sprite_x = crop_x - roi.x;
            int sprite_y = crop_y - roi.y;
            s.data[sprite_y * roi.w + sprite_x] = c;
        }

    return s;
}

Sprite load_player_sprite() {
    return sprite_from_png(read_png("resources/sprites.png"), Crop{.x = 0, .y = 0, .w = 16, .h = 16});
}

void add_player() {
    int id = tm.add_grid_entity(PLAYER, 0, 0);
    player_id = id;
    tm.transforms[id] = mul(tm.transforms[id], from_scale(0.05));
}

GridPosition editor_mouse_position() {
    float ratio = gfx_width() / tile_size;
    int x = editor.mouse.x * ratio;
    int y = editor.mouse.y * ratio;
    return {x, y};
}

bool can_place_here(ItemType type, GridPosition p) {
    if (is_empty(p) and type != FLOOR)
        return false;
    return true;
}

void editor_place_item(ItemType type) {
    GridPosition p = editor_mouse_position();

    if (!can_place_here(type, p))
        return;
    tm.add_grid_entity(type, p.x, p.y);
}

void editor_remove_item() {
    GridPosition p = editor_mouse_position();
    if (MaybeId block = at(BLOCK, p); block.valid) {
        tm.remove(block.id);
        return;
    }

    if (MaybeId floor = at(FLOOR, p); floor.valid) {
        tm.remove(floor.id);
        return;
    }
}

void editor_start_dragging() {
    GridPosition p = editor_mouse_position();
    if (MaybeId item = item_at(p); item.valid) {
        editor.selected = item.id;
    }
}

void editor_while_dragging() {
    if (editor.selected < 0)
        return;

    GridPosition p = editor_mouse_position();
    if (!can_place_here(tm.types[editor.selected], p))
        return;
    set_item_position(editor.selected, p);
}

void editor_stop_dragging() {
    editor.selected = -1;
}

int editor_selected_face() {
    float ratio = gfx_width() / tile_size;
    float x = editor.mouse.x * ratio;
    float y = editor.mouse.y * ratio;
    x = x - floor(x);
    y = y - floor(y);
    bool bottomleft = x < y;
    bool topleft = 1 - y > x;

    if (topleft && !bottomleft)
        return 0;
    if (!topleft && !bottomleft)
        return 1;
    if (!topleft && bottomleft)
        return 2;
    if (topleft && bottomleft)
        return 3;
}

void editor_set_portal(int tunnel_id) {
    GridPosition p = editor_mouse_position();

    if (MaybeId item = item_at(p); item.valid) {
        if (tm.types[item.id] == BLOCK) {
            int face = editor_selected_face();
            printf("adding to face: %d\n", face);
            tm.portals[item.id].tunnel[face] = tunnel_id;
        }
    }
}

void portal2d_init() {
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x) {
            tm.add_grid_entity(FLOOR, x, y);
        }

    // tm.add_grid_entity(WALL, 5, 5);
    // tm.add_grid_entity(WALL, 2, 5);

    // int block1 = tm.add_grid_entity(BLOCK, 3, 1);
    // tm.portals[block1].tunnel[1] = 1;
    // tm.portals[block1].tunnel[2] = 1;

    // int block2 = tm.add_grid_entity(BLOCK, 4, 2);
    // tm.portals[block2].tunnel[0] = 0;
    // tm.portals[block2].tunnel[3] = 0;

    add_player();

    tm.sprites[FLOOR].w = 1;
    tm.sprites[FLOOR].h = 1;
    static RGBA floor_c[] = {
        {0.5, 0.5, 0.5, 1},
    };
    tm.sprites[FLOOR].data = floor_c;


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
            case GLFW_KEY_ENTER: editor_mode = !editor_mode; break;
        }

        if (editor_mode) {
            switch (key) {
                case GLFW_KEY_0: editor_set_portal(-1); break;
                case GLFW_KEY_1: editor_set_portal(0); break;
                case GLFW_KEY_2: editor_set_portal(1); break;
                case GLFW_KEY_3: editor_set_portal(2); break;

                case GLFW_KEY_F: editor_place_item(FLOOR); break;
                case GLFW_KEY_B: editor_place_item(BLOCK); break;
                case GLFW_KEY_X: editor_remove_item(); break;
            }
        }
    }
}

void portal2d_mouse_cursor_position(float xpos, float ypos) {
    if (editor_mode) {
        editor.mouse.x = xpos;
        editor.mouse.y = ypos;
        editor_while_dragging();
    }
}

void portal2d_mouse_button(int button, int action) {
    if (!editor_mode)
        return;

    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            editor_start_dragging();
        }
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            editor_stop_dragging();
        }
    }
}

void portal2d_update(float dt) {
    // update animations
}

void portal2d_draw() {
    int n_ids = 0;
    int ordered_ids[max_items];
    for (int id = 0; id < tm.max_id; ++id) {
        if (tm.types[id] == FLOOR) {
            ordered_ids[n_ids++] = id;
        }
    }

    for (int id = 0; id < tm.max_id; ++id) {
        if (tm.types[id] != FLOOR && tm.types[id] != EMPTY) {
            ordered_ids[n_ids++] = id;
        }
    }

    // Affine view_transform = from_scale(tile_size);
    for (int i = 0; i < n_ids; ++i) {
        int id = ordered_ids[i];
        auto type = tm.types[id];
        if (type == EMPTY)
            continue;

        auto view_model = mul(affine_eye(), tm.transforms[id]);
        gfx_draw_sprite(tm.sprites[type], view_model, false);

        if (type == BLOCK) {
            const RGBA tunnel_colors[] = {
                rgba_from_hex(0xff595e),
                rgba_from_hex(0xffca3a),
                rgba_from_hex(0x8ac926),
            };

            if (int tid = tm.portals[id].tunnel[0]; tid >= 0) {
                gfx_draw_line(multiply_affine_vec2(view_model, {0, 0}), multiply_affine_vec2(view_model, {1, 0}),
                              tunnel_colors[tid], 3);
            }
            if (int tid = tm.portals[id].tunnel[1]; tid >= 0) {
                gfx_draw_line(multiply_affine_vec2(view_model, {1, 0}), multiply_affine_vec2(view_model, {1, 1}),
                              tunnel_colors[tid], 3);
            }
            if (int tid = tm.portals[id].tunnel[2]; tid >= 0) {
                gfx_draw_line(multiply_affine_vec2(view_model, {1, 1}), multiply_affine_vec2(view_model, {0, 1}),
                              tunnel_colors[tid], 3);
            }
            if (int tid = tm.portals[id].tunnel[3]; tid >= 0) {
                gfx_draw_line(multiply_affine_vec2(view_model, {0, 1}), multiply_affine_vec2(view_model, {0, 0}),
                              tunnel_colors[tid], 3);
            }
        }
    }

    if (editor_mode) {
        gfx_draw_point({10, 10}, {1, 0, 0, 1}, 5);
    }
}
