#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cstdlib>

#include "gfx.hpp"
#include "math.hpp"
#include "png.hpp"
#include "utility.hpp"
#include "perlin.h"

enum ItemType {
    GRASS,
    WATER,
};

const int grid_w = 100;
const int grid_h = 100;
const int grid_size = grid_w * grid_h;

ItemType type[grid_size];
float height[grid_size];
Vec3 albedo[grid_size];
bool shadow[grid_size];

Vec3 item_albedo[10];
float sun_angle;
unsigned long time_ms;
float time_scaling = 1000;
float start_time_hours = 9.f;
Period period_update_shadow_map = period_every_hour();

const int window_width = 1024;
const int window_height = 1024;
bool running = true;

Vec3 rgba_to_vec3(RGBA c) {
    return {c.r, c.g, c.b};
}

float get_time_of_day() {
    int ms_in_hour = 3600 * 1000;
    int offset = start_time_hours * ms_in_hour;
    unsigned long clock = (time_ms + offset) % (24 * ms_in_hour);
    return clock / (float)ms_in_hour;
}

void update_sun_angle() {
    // TODO: use sunset/sunrise to set the angle
    // const float sunset = 18;
    // const float noon = 12;
    // const float sunrise = 6;
    sun_angle = 2 * M_PI * get_time_of_day() / 24 - M_PI / 2;
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

Vec3 kelvin_to_color(float t) {
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

    return vec3_clamp(vec3_div({red, green, blue}, 255.f), 0, 1);
}

void init() {
    for (int i = 0; i < grid_size; ++i) {
        type[i] = GRASS;
        height[i] = 0;
    }

    // TODO: make with multiple octaves
    float* perlin = create_perlin_grid(grid_h, grid_w, 0.1);
    for (int i = 0; i < grid_size; ++i) {
        printf("%.2f\n", perlin[i]);
        height[i] = perlin[i] * 8;
    }
    free(perlin);

    item_albedo[GRASS] = rgba_to_vec3(rgba_from_hex(0x606c38));
    item_albedo[WATER] = rgba_to_vec3(rgba_from_hex(0x457b9d));
}

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

bool is_occluded(Vec3 light_pos, Vec3 point_pos) {
    Ray ray = {
        .origin = point_pos,
        .direction = vec3_normalize(vec3_sub(light_pos, point_pos)),
    };

    float step_size = 2;
    Vec3 step = vec3_scale(ray.direction, step_size);

    while (vec3_norm(vec3_sub(ray.origin, light_pos)) > 1) {
        Vec3 p = vec3_add(ray.origin, step);

        int x = p.x;
        int y = p.y;
        if (x < 0 || y < 0 || x >= grid_w || y >= grid_h) {
            return false;
        }

        if (height[y * grid_w + x] >= p.z) {
            return true;
        }

        ray.origin = p;
    }

    return false;
}

void update_shadow_map() {
    Vec3 sun_vec = {cos(sun_angle), 0, sin(sun_angle)};
    Vec3 sun_pos_3d = vec3_scale(sun_vec, 1000);
    for (int y = 0; y < grid_h; ++y)
        for (int x = 0; x < grid_w; ++x) {
            int xy = y * grid_w + x;

            Vec3 point_3d = {(float)x, (float)y, height[xy]};
            if (is_occluded(sun_pos_3d, point_3d)) {
                shadow[xy] = true;
                continue;
            }
            shadow[xy] = false;
        }
}

void update(float dt) {
    int dt_ms = time_scaling * dt * 1000;
    time_ms += dt_ms;

    update_sun_angle();

    if (period_update_shadow_map.passed(dt_ms)) {
        update_shadow_map();
    }
}

void pre_rendering() {
    float height_range = 50;
    for (int i = 0; i < grid_size; ++i) {
        float height_color_scale = fmin(10, fmax(0, 1 + (height[i] / height_range)));
        albedo[i] = vec3_scale(item_albedo[type[i]], height_color_scale);
    }
}

void draw(Img* fb) {
    pre_rendering();

    Vec3 ambient_light = vec3_scale({1, 1, 1}, 0.3);
    Vec3 sun_vec = {cos(sun_angle), 0, sin(sun_angle)};
    Vec3 sun_color = kelvin_to_color(sun_temperature_from_angle(sun_angle));
    Vec3 sun_pos_3d = vec3_scale(sun_vec, 1000);
    Vec3 normal = {0, 0, 1};
    for (int y = 0; y < grid_h; ++y)
        for (int x = 0; x < grid_w; ++x) {
            int xy = y * grid_w + x;
            Vec3 illumination = ambient_light;

            // Vec3 point_3d = {(float)x, (float)y, item.height};
            if (!shadow[xy]) {
                float diffuse = fmax(0, vec3_dot(normal, sun_vec));
                Vec3 sun_light = vec3_scale(sun_color, diffuse);
                illumination = vec3_add(illumination, sun_light);
            }

            Vec3 alb = albedo[xy];
            Vec3 shaded = vec3_mul(illumination, alb);

            img_set(fb, x, y, rgba_clamp({shaded.x, shaded.y, shaded.z, 1}));
        }
}

void on_key_input(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        running = false;
        return;
    }
}

int main(int argc, char** argv) {
    std::srand(std::time(0));

    if (!glfwInit()) {
        printf("Cannot initialize GLFW\n");
        abort();
    }

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Awesome game", NULL, NULL);
    if (!window) {
        printf("Cannot open window\n");
        abort();
    }

    glfwMakeContextCurrent(window);

    Timer timer_dt;
    timer_dt.start();

    glfwSetKeyCallback(window, on_key_input);
    // glfwSetCursorPosCallback(window, on_mouse_move);
    // glfwSetMouseButtonCallback(window, on_mouse_click);
    // glfwSetScrollCallback(window, portal2d_mouse_button);

    gfx_init(grid_w, grid_h);
    init();

    while (running) {
        float dt = timer_dt.tick();
        printf("fps: %.2f\n", 1 / dt);
        update(dt);

        gfx_clear();
        draw(gfx_get_framebuffer());
        gfx_draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
