#include "world.hpp"
#include <GLFW/glfw3.h>

#include "gfx.hpp"
#include "image.hpp"
#include "math.hpp"
#include "utility.hpp"

struct Camera {
    Vec2 position;  // offset from game grid top-left (0,0)
    float size_x;   // size on game grid
    float size_y;
    int res_x;  // texture resolution
    int res_y;

    float to_viewport(float a) { return a * res_x / size_x; }

    Affine view_transform() {
        Affine translation = from_translation(-position);
        Affine scale = from_scale(res_x / size_x);
        return mul(scale, translation);
    }
};

enum ShapeType {
    CIRCLE,
};

struct Shape {
    ShapeType type;
    float radius;
    RGBA color;
};

struct Controls {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

enum GroundType {
    GRASS,
    DIRT,
};

struct GroundSample {
    GroundType type;
    Vec2 p;
};

struct Texture {
    RGBA data[100];
};

const int max_items = 1024;

int world_x = 1000;
int world_y = 1000;
int n_ground_sample = 0;
GroundSample ground_sample[128];
Vec2 position[max_items];
Vec2 velocity[max_items];
Shape shape[max_items];
int n_ids = 0;
Controls controls;
int controlling;
unsigned long time_ms;
float time_scaling = 1;
float ms_accumulated;
Camera camera;
Image texture[10];
Image ground;

Image texture_grass_create() {
    Image t = image_create(10, 10);

    for (int i = 0; i < 100; ++i) {
        RGBA c = rgba_from_hex(0x7cb518);
        RGBA noise = randf() * 0.05 * RGBA{1, 1, 1, 1};
        t.data[i] = c + noise;
    }

    return t;
}

Image texture_dirt_create() {
    Image t = image_create(10, 10);

    for (int i = 0; i < 100; ++i) {
        RGBA c = rgba_from_hex(0x6c584c);
        RGBA noise = randf() * 0.05 * RGBA{1, 1, 1, 1};
        t.data[i] = c + noise;
    }

    return t;
}

int new_id() {
    assert(n_ids < max_items);
    return n_ids++;
}

int add_player() {
    int id = new_id();
    position[id] = {10, 10};
    velocity[id] = {0, 0};
    shape[id] = {.type = CIRCLE, .radius = 1, .color = rgba_from_hex(0x264653)};
    return id;
}

int add_tree(Vec2 p) {
    int id = new_id();
    position[id] = p;
    shape[id] = {.type = CIRCLE, .radius = 2, .color = rgba_from_hex(0x7f5539)};
    return id;
}

RGBA texture_sample(Image* t, Vec2 p) {
    int x = (int)p.x % 10;
    int y = (int)p.y % 10;
    return t->data[y * 10 + x];
}

void render_ground() {
    float max_distance = sqrt(world_x * world_y);
    for (int y = 0; y < world_y; ++y)
        for (int x = 0; x < world_x; ++x) {
            Vec2 p = {(float)x, (float)y};

            float weights = 0;
            RGBA c;
            for (int i = 0; i < n_ground_sample; ++i) {
                GroundSample g = ground_sample[i];
                float distance = vec2_norm(p - g.p);
                RGBA sample = texture_sample(&texture[g.type], {(float)x, (float)y});
                if (distance == 0) {
                    c = sample;
                    weights = 0;
                    break;
                } else {
                    float w = 1.f / powf(distance / max_distance, 8);
                    weights += w;
                    c = c + w * sample;
                }
            }
            if (weights > 0) {
                c = c / weights;
            }

            ground.data[y*ground.w + x] = c;
        }
}

void world_init() {
    int player_id = add_player();
    controlling = player_id;

    add_tree({15, 10});

    camera.position = {0, 0};
    camera.size_x = 100;
    camera.size_y = 100;
    camera.res_x = 512;
    camera.res_y = 512;

    ground_sample[n_ground_sample++] = {.type = GRASS, .p = {50, 50}};
    ground_sample[n_ground_sample++] = {.type = GRASS, .p = {40, 60}};
    ground_sample[n_ground_sample++] = {.type = GRASS, .p = {30, 70}};
    ground_sample[n_ground_sample++] = {.type = DIRT, .p = {180, 180}};

    texture[GRASS] = texture_grass_create();
    texture[DIRT] = texture_dirt_create();

    ground = image_create(world_x, world_y);
    render_ground();
}

void resolve_collision(int a, int b) {
    Vec2 ab = position[b] - position[a];
    float min_distance = shape[a].radius + shape[b].radius;
    float actual_distance = vec2_norm(ab);
    if (actual_distance < min_distance) {
        float d = min_distance - actual_distance;
        if (!is_zero(velocity[a])) {
            position[a] = position[a] - d * ab;
        } else {
            position[b] = position[b] + d * ab;
        }
        velocity[a] = {};
        velocity[b] = {};
    }
}

void world_update(float dt) {
    time_ms += dt * 1000;
    ms_accumulated += dt * 1000;
    if (ms_accumulated > 1000) {
        // printf("seconds: %d\n", time_ms / 1000);
        ms_accumulated = 0;
    }

    const float speed = 30;
    Vec2 vel;

    if (controls.left) {
        vel.x = -1;
    }

    if (controls.right) {
        vel.x = 1;
    }

    if (controls.up) {
        vel.y = -1;
    }

    if (controls.down) {
        vel.y = 1;
    }

    if (!is_zero(vel)) {
        vel = normalize(vel) * speed;
    }

    velocity[controlling] = vel;
    position[controlling] = position[controlling] + velocity[controlling] * dt;

    for (int id_a = 0; id_a < n_ids; ++id_a) {
        for (int id_b = id_a + 1; id_b < n_ids; ++id_b) {
            resolve_collision(id_a, id_b);
        }
    }

    camera.position = position[controlling] - Vec2{camera.size_x/2, camera.size_y/2};
}

void world_draw() {
    Affine t = camera.view_transform();
    gfx_draw_sprite(&ground, t, false);

    for (int id = 0; id < n_ids; ++id) {
        Shape s = shape[id];
        switch (s.type) {
            case CIRCLE: gfx_draw_point(multiply_affine_vec2(t, position[id]), s.color, camera.to_viewport(s.radius));
        }
    }
}

void world_key_input(int action, int key) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_A: controls.left = true; break;
            case GLFW_KEY_D: controls.right = true; break;
            case GLFW_KEY_W: controls.up = true; break;
            case GLFW_KEY_S: controls.down = true; break;
        }

    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_A: controls.left = false; break;
            case GLFW_KEY_D: controls.right = false; break;
            case GLFW_KEY_W: controls.up = false; break;
            case GLFW_KEY_S: controls.down = false; break;
        }
    }
}

void world_mouse_cursor_position(float xpos, float ypos) {}

void world_mouse_button(int button, int action) {}
