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

struct Sun {
    float angle;
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
float time_scaling = 10000;
float ms_accumulated;
float start_time_hours = 6.f;
Camera camera;
Image texture[10];
Image ground;
Sun sun;

float get_time_of_day() {
    int ms_in_hour = 3600 * 1000;
    int offset = start_time_hours * ms_in_hour;
    unsigned long clock = (time_ms + offset) % (24 * ms_in_hour);
    return clock / (float)ms_in_hour;
}

void sun_update_angle() {
    // TODO: use sunset/sunrise to set the angle
    // const float sunset = 18;
    // const float noon = 12;
    // const float sunrise = 6;
    sun.angle = 2 * M_PI * get_time_of_day() / 24 - M_PI / 2;
    printf("angle: %f\n", sun.angle);
}

RGB kelvin_to_color(float t) {
    t /= 100;

    float red = 255;
    if (t >= 66) {
        red = t - 60;
        red = 329.698727446 * (pow(red, -0.1332047592));
    }

    float green;
    if (t <= 66) {
        green = t;
        green = 99.4708025861 * log(green) - 161.1195681661;
    } else {
        green = t - 60;
        green = 288.1221695283 * (pow(green, -0.0755148492));
    }

    float blue = 255;
    if (t < 66) {
        if (t <= 19) {
            blue = 0;
        } else {
            blue = t - 10;
            blue = 138.5177312231 * log(blue) - 305.0447927307;
        }
    }

    return rgb_clamp(rgb_div({red, green, blue}, 255.f));
}

float sun_temperature_from_angle(float angle) {
    const float pi2 = M_PI/2;
    const float temp_sunrise = 2500;
    const float temp_sunset = 2500;
    const float temp_noon = 7000;
    if (angle >= 0 && angle < pi2) {
        float x = angle / pi2;
        return (1 - x) * temp_sunrise + x * temp_noon;
    } else if (angle >= pi2 && angle < M_PI) {
        float x = (angle - pi2) / pi2;
        return (1 - x) * temp_noon + x * temp_sunset;
    } else {
        return temp_sunrise;
    }
}

Image texture_grass_create() {
    Image t = image_create(30, 30);

    for (int i = 0; i < 900; ++i) {
        RGBA c = rgba_from_hex(0x7cb518);
        RGBA noise = randf() * 0.05 * RGBA{1, 1, 1, 1};
        t.data[i] = c + noise;
    }

    return t;
}

Image texture_dirt_create() {
    Image t = image_create(30, 30);

    for (int i = 0; i < 900; ++i) {
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
    int x = (int)p.x % t->w;
    int y = (int)p.y % t->h;
    return t->data[y * t->w + x];
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

            ground.data[y * ground.w + x] = c;
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

    sun.angle = M_PI / 2.f;
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
    time_ms += time_scaling * dt * 1000;
    ms_accumulated += time_scaling * dt * 1000;
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

    camera.position = position[controlling] - Vec2{camera.size_x / 2, camera.size_y / 2};

    sun_update_angle();
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

    const Vec3 normal = {0, 0, 1};
    const Vec3 sun_vec = {cos(sun.angle), 0, sin(sun.angle)};
    const RGB sun_color_rgb = kelvin_to_color(sun_temperature_from_angle(sun.angle));
    const RGBA sun_color = {sun_color_rgb.r, sun_color_rgb.g, sun_color_rgb.b, 1};
    for (int y = 0; y < camera.res_y; ++y)
        for (int x = 0; x < camera.res_x; ++x) {
            RGBA color = gfx_get(x, y);
            float diffuse = fmax(0, vec3_dot(normal, sun_vec));
            RGBA shaded = (diffuse * sun_color) * color;
            gfx_set(x, y, shaded);
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

void world_scroll_input(float xoffset, float yoffset) {
    float delta = yoffset * 20;
    camera.size_x += delta;
    camera.size_y += delta;
    const float min_size = 10;
    if (camera.size_x < min_size || camera.size_y < min_size) {
        camera.size_x -= delta;
        camera.size_y -= delta;
    }
}
