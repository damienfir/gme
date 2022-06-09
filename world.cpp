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
    Vec2 mouse;
};

enum GroundType {
    NONE,
    GRASS,
    DIRT,
    WATER,
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

struct Torch {
    Vec2 direction;
};

struct Water {
    int w = 0;
    int h = 0;
    int x;
    int y;
    float *heightmap;
    float *velocity;
    float *temp;
    bool* mask;
};

struct Ground {
    GroundType *mat;
};

struct RenderingBuffer {
    float *test;
    Vec3 *normal;
    Vec3 *albedo;
};

const int max_items = 1024;

const int world_x = 1000;
const int world_y = 1000;
const int world_size = world_x * world_y;
Vec2 position[max_items];
Vec2 velocity[max_items];
Shape shape[max_items];
int n_ids = 0;
Controls controls;
int controlling;
unsigned long time_ms;
float time_scaling = 500;
float ms_accumulated;
float start_time_hours = 9.f;
Camera camera;
Image texture[10];
Sun sun;
Torch torch;
Ground ground;
RenderingBuffer rendering;
Image ground_sprite;
Water water[10];
const int max_ground_sample = 128;
int n_ground_sample = 0;
GroundSample ground_sample[max_ground_sample];

void water_add(int x, int y, int width, int height) {
    int id = 0;
    while (id < 10) {
        if (water[id].h == 0) break;
        id++;
    }
    assert(id < 10);

    Water *w = &water[id];
    w->x = x;
    w->y = y;
    w->h = height;
    w->w = width;
    w->heightmap = (float*)malloc(height*width*sizeof(float));
    w->temp = (float*)malloc(height*width*sizeof(float));
    w->velocity = (float*)malloc(height*width*sizeof(float));
    w->mask = (bool*)malloc(height*width*sizeof(bool));

    for (int i = 0; i < width*height; ++i) w->mask[i] = 1;
    
    // Define boundaries
    for (int y = 0; y < height; ++y) {
        w->mask[y * width] = 0;
        w->mask[y * width + width - 1] = 0;
    }

    for (int x = 0; x < width; ++x) {
        w->mask[x] = 0;
        w->mask[(height-1) * width + x] = 0;
    }

    w->mask[10 * width + 10] = 0;
    w->mask[11 * width + 10] = 0;
    w->mask[12 * width + 10] = 0;
    w->mask[13 * width + 10] = 0;
    w->mask[14 * width + 10] = 0;
    w->mask[15 * width + 10] = 0;
    w->mask[16 * width + 10] = 0;
    w->mask[17 * width + 10] = 0;
    w->mask[18 * width + 10] = 0;

    for (int i = 0; i < width*height; ++i) {
        if (!w->mask[i]) continue;

        w->heightmap[i] = 0;
        w->velocity[i] = 0;

        int xi = i % width;
        int yi = i / width;
        int world_xy = (y + yi) * world_x + (x + xi);
        ground.mat[world_xy] = WATER;
        rendering.normal[world_xy] = {0, 0, 1};
        rendering.albedo[world_xy] = {0, 0, 1};
    }

    w->heightmap[10 * width + 1] = 3;
    w->heightmap[11 * width + 1] = 3;

}

void water_update(float dt) {
    for (int i = 0; i < 10; ++i) {
        if (water[i].h == 0) continue;

        Water* w = &water[i];

        bool* m = w->mask;
        float* h = w->heightmap;

        for (int y = 0; y < w->h; ++y) {
            for (int x = 0; x < w->w; ++x) {
                int xy = y * w->w + x;
                if (!m[xy]) continue;

                int x0y = m[xy - 1] ? xy - 1 : xy;
                int x1y = m[xy + 1] ? xy + 1 : xy;
                int xy0 = m[xy - w->w] ? xy - w->w : xy;
                int xy1 = m[xy + w->w] ? xy + w->w : xy;
                int x0y0 = m[xy0 - 1] ? xy0 - 1 : xy0;
                int x1y0 = m[xy0 + 1] ? xy0 + 1 : xy0;
                int x0y1 = m[xy1 - 1] ? xy1 - 1 : xy1;
                int x1y1 = m[xy1 + 1] ? xy1 + 1 : xy1;

                const float c2 = 100;
                float sq2 = 1/sqrtf(2);
                float s = 4*sq2 + 4;
                float f = c2 * (
                        sq2 * h[x0y0] + sq2 * h[x0y1] + sq2 * h[x1y1] + sq2 * h[x1y0]
                        + h[x0y] + h[x1y] + h[xy0] + h[xy1]
                        - s * h[xy]
                        ) / s;
                w->velocity[xy] += f * dt;
                w->velocity[xy] *= 0.998;
                w->temp[xy] = h[xy] + w->velocity[xy] * dt;

                int world_xy = (w->y + y) * world_x + (w->x + x);
                float nx = (h[x1y] - h[x0y]) / 2;
                float ny = (h[xy1] - h[xy0]) / 2;
                Vec3 normal = vec3_normalize({nx, ny, 1});
                rendering.normal[world_xy] = normal;
            }
        }

        float* swapped = w->heightmap;
        w->heightmap = w->temp;
        w->temp = swapped;
    }
}

void water_touch(Vec2 pos) {
    int x = (int)pos.x;
    int y = (int)pos.y;

    for (int i = 0; i < 10; ++i) {
        if (water[i].h == 0) continue;

        Water w = water[i];
        if (x >= w.x && x < w.x + w.w && y >= w.y && y < w.y + w.h) {
            int water_xy = (y - w.y) * w.w + (x - w.x);
            w.heightmap[water_xy] = 3;
            break;
        }
    }
}

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
}

RGB kelvin_to_color(float t) {
    // Code from https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
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
    const float pi2 = M_PI / 2;
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

void make_ground() {
    int n = n_ground_sample;

    float *weights = (float*)malloc(n * sizeof(float));

    float max_distance = sqrt(world_x * world_y);
    for (int y = 0; y < world_y; ++y)
        for (int x = 0; x < world_x; ++x) {
            int world_xy = y * world_x + x;

            Vec2 p = {(float)x, (float)y};

            bool exact_match = false;
            float total_weight = 0;
            for (int i = 0; i < n; ++i) {
                GroundSample g = ground_sample[i];
                float distance = vec2_norm(p - g.p);

                if (distance == 0) {
                    ground.mat[y * world_x + x] = g.type;
                    exact_match = true;
                    break;
                } else {
                    weights[i] = total_weight;
                    float w =  1.f / powf(distance / max_distance, 8);
                    total_weight += w;
                }
            }

            if (exact_match) continue;

            float r = randf() * total_weight;
            int chosen_sample = n-1;
            for (int i = 1; i < n; ++i) {
                if (r < weights[i]) chosen_sample = i-1;
            }

            ground.mat[world_xy] = ground_sample[chosen_sample].type;
        }

    free(weights);
}

void draw_material_pass() {
    for (int i = 0; i < world_size; ++i) {
        GroundType mat = ground.mat[i];

        if (mat != DIRT && mat != GRASS) continue;

        int x = i % world_x;
        int y = i / world_x;
        RGBA color = texture_sample(&texture[mat], {(float)x, (float)y});
        rendering.normal[i] = {0, 0, 1};
        rendering.albedo[i] = {color.r, color.g, color.b};
    }
}

void make_default_ground() {
    ground_sample[n_ground_sample++] = {.type = GRASS, .p = {50, 50}};
    ground_sample[n_ground_sample++] = {.type = GRASS, .p = {40, 60}};
    ground_sample[n_ground_sample++] = {.type = GRASS, .p = {30, 70}};
    ground_sample[n_ground_sample++] = {.type = DIRT, .p = {180, 180}};
    make_ground();
    draw_material_pass();
}

void ground_add_more(Vec2 p) {
    int x = (int)p.x;
    int y = (int)p.y;
    GroundType target = ground.mat[y * world_x + x];
    ground_sample[n_ground_sample++] = {.type = target, .p = p};
    make_ground();
    draw_material_pass();
}

void world_init() {
    int player_id = add_player();
    controlling = player_id;

    add_tree({15, 10});

    camera.position = {0, 0};
    camera.size_x = 100;
    camera.size_y = 100;
    camera.res_x = gfx_width();
    camera.res_y = gfx_height();

    sun.angle = M_PI / 2.f;

    texture[GRASS] = texture_grass_create();
    texture[DIRT] = texture_dirt_create();

    rendering.normal = (Vec3*)malloc(world_size*sizeof(Vec3));
    rendering.albedo = (Vec3*)malloc(world_size*sizeof(Vec3));
    ground.mat = (GroundType*)malloc(world_size*sizeof(GroundType));
    make_default_ground();
    water_add(20, 20, 50, 50);
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

    Vec2 p = position[controlling] + vel * dt;

    int world_xy = (int)p.y * world_x + (int)p.x;
    if (ground.mat[world_xy] == WATER) {
        p = position[controlling];
    }

    velocity[controlling] = vel;
    position[controlling] = p; // position[controlling] + velocity[controlling] * dt;

    for (int id_a = 0; id_a < n_ids; ++id_a) {
        for (int id_b = id_a + 1; id_b < n_ids; ++id_b) {
            resolve_collision(id_a, id_b);
        }
    }


    camera.position = position[controlling] - Vec2{camera.size_x / 2, camera.size_y / 2};

    sun_update_angle();
    water_update(dt);
}

Vec3 torch_light(Vec2 point_pos) {
    float min_dot = 0.8;
    float max_distance = 50;

    Vec2 torch_pos = position[controlling];
    Vec2 torch_to_point = vec2_sub(point_pos, torch_pos);
    float distance_torch_point = vec2_norm(torch_to_point);

    Vec2 torch_to_point_normalized = vec2_div(torch_to_point, distance_torch_point);
    Vec2 perp = {torch_to_point_normalized.y, -torch_to_point_normalized.x};

    bool in_shadow = false;
    for (int i = 0; i < n_ids; ++i) {
        if (i == controlling)
            continue;

        Shape s = shape[i];
        if (s.type == CIRCLE) {
            Vec2 shape_pos = position[i];

            if (vec2_norm(vec2_sub(shape_pos, point_pos)) < s.radius) {
                in_shadow = true;
                break;
            }

            Vec2 torch_to_shape = vec2_sub(shape_pos, torch_pos);
            if (distance_torch_point < vec2_norm(torch_to_shape))
                continue;

            if (vec2_dot(torch_to_shape, torch_to_point) < 0)
                continue;

            float distance_line_origin = vec2_dot(perp, torch_to_shape);
            if (fabsf(distance_line_origin) < s.radius) {
                // printf("d: %f\n", distance_line_origin);
                in_shadow = true;
                break;
            }
        }
    }
    if (in_shadow)
        return {0, 0, 0};

    Vec2 mouse = multiply_affine_vec2(inverse(camera.view_transform()), controls.mouse);
    Vec2 torch_dir = normalize(mouse - torch_pos);
    float distance_intensity = fmaxf(0, fminf(1, (1 - sqrt(distance_torch_point / max_distance))));
    float d = vec2_dot(torch_to_point_normalized, torch_dir);
    float beam_intensity = fmaxf(0, sqrt(d - min_dot));
    float intensity = clampf(3 * beam_intensity * distance_intensity, 0, 1);
    return vec3_scale({1, 1, 1}, intensity);
}

bool is_occluded(Vec3 light_pos, Vec3 point_pos) {
    // Optimization: look at objects within a margin of viewing region
    Vec3 point_to_light = vec3_sub(light_pos, point_pos);
    float dist_point_light = vec3_norm(point_to_light);
    Vec3 point_to_light_normalized = vec3_div(point_to_light, dist_point_light);

    for (int i = 0; i < n_ids; ++i) {
        // Considering every object as a sphere
        Shape s = shape[i];

        Vec2 shape_pos_2d = position[i];
        Vec3 shape_pos = {shape_pos_2d.x, shape_pos_2d.y, 0};

        if (vec3_norm(vec3_sub(shape_pos, point_pos)) < s.radius) {
            continue;
        }

        Vec3 point_to_shape = vec3_sub(shape_pos, point_pos);
        float dist_shape_light = vec3_norm(vec3_sub(shape_pos, light_pos));
        if (dist_point_light < dist_shape_light) continue;

        Vec3 shape_on_line = vec3_project_onto(point_to_shape, point_to_light_normalized);

        // Use a p-norm to simulate a cube instead of a sphere, square shadows look more realistic (and
        // more compatible with a lot of different shapes)
        float dist_shape_line = vec3_pnorm(8, vec3_sub(point_to_shape, shape_on_line));
        if (dist_shape_line < s.radius)
            return true;
    }

    return false;
}

// void render_ground() {
//     Vec3 ambient_light = vec3_scale({1, 1, 1}, 0.1);
//     Vec3 sun_vec = {cos(sun.angle), 0, sin(sun.angle)};
//     RGB sun_color_rgb = kelvin_to_color(sun_temperature_from_angle(sun.angle));
//     Vec3 sun_color = {sun_color_rgb.r, sun_color_rgb.g, sun_color_rgb.b};
//     Vec3 sun_pos_3d = vec3_scale(sun_vec, 1000);
//     for (int y = 0; y < world_y; ++y)
//         for (int x = 0; x < world_x; ++x) {
//             Vec2 world_pos = {(float)x, (float)y};

//             Vec3 illumination = ambient_light;
//             illumination = vec3_add(illumination, torch_light(world_pos));

//             int xy = (int)world_pos.y * world_x + (int)world_pos.x;
//             Vec3 normal = rendering.normal[xy];
//             Vec3 point_3d = {world_pos.x, world_pos.y, 0};
//             if (!is_occluded(sun_pos_3d, point_3d)) {
//                 float diffuse = fmax(0, vec3_dot(normal, sun_vec));
//                 Vec3 sun_light = vec3_scale(sun_color, diffuse);
//                 illumination = vec3_add(illumination, sun_light);
//             }

//             Vec3 albedo = rendering.albedo[xy];
//             Vec3 shaded = vec3_mul(illumination, albedo);

//             image_set(&ground_sprite, x, y, rgba_clamp({shaded.x, shaded.y, shaded.z, 1}));
//         }
// }

void world_draw() {
    Affine t = camera.view_transform();
    Affine ti = inverse(t);
    
    // render_ground();
    // gfx_draw_sprite(&ground_sprite, t, false);

    Vec3 ambient_light = vec3_scale({1, 1, 1}, 0.1);
    Vec3 sun_vec = {cos(sun.angle), 0, sin(sun.angle)};
    RGB sun_color_rgb = kelvin_to_color(sun_temperature_from_angle(sun.angle));
    Vec3 sun_color = {sun_color_rgb.r, sun_color_rgb.g, sun_color_rgb.b};
    Vec3 sun_pos_3d = vec3_scale(sun_vec, 1000);
    for (int y = 0; y < camera.res_y; ++y)
        for (int x = 0; x < camera.res_x; ++x) {
            Vec2 world_pos = multiply_affine_vec2(ti, {(float)x, (float)y});

            if (world_pos.x < 0 || world_pos.y < 0 || world_pos.x >= world_x || world_pos.y >= world_y)
                continue;

            Vec3 illumination = ambient_light;

            illumination = vec3_add(illumination, torch_light(world_pos));

            int xy = (int)world_pos.y * world_x + (int)world_pos.x;
            Vec3 normal = rendering.normal[xy];
            // vec3_print("normal", normal);
            Vec3 point_3d = {world_pos.x, world_pos.y, 0};
            if (!is_occluded(sun_pos_3d, point_3d)) {
                float diffuse = fmax(0, vec3_dot(normal, sun_vec));
                Vec3 sun_light = vec3_scale(sun_color, diffuse);
                illumination = vec3_add(illumination, sun_light);
            }

            Vec3 albedo = rendering.albedo[xy];
            Vec3 shaded = vec3_mul(illumination, albedo);

            gfx_set(x, y, rgba_clamp({shaded.x, shaded.y, shaded.z, 1}));
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

void world_mouse_cursor_position(float xpos, float ypos) {
    controls.mouse = {xpos * camera.res_x, ypos * camera.res_y};
}

void world_mouse_button(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        Affine t = inverse(camera.view_transform());
        Vec2 p = multiply_affine_vec2(t, controls.mouse);

        int world_xy = (int)p.y * world_x + (int)p.x;
        if (ground.mat[world_xy] == WATER) {
            water_touch(p);
        } else {
            printf("ok\n");
            ground_add_more(p);
        }
    }
}

void world_scroll_input(float xoffset, float yoffset) {
    float delta = -yoffset * 20;
    camera.size_x += delta;
    camera.size_y += delta;
    const float min_size = 10;
    if (camera.size_x < min_size || camera.size_y < min_size) {
        camera.size_x -= delta;
        camera.size_y -= delta;
    }
}


// NEXT:
// Water rendering with heightmap and specular highlights
