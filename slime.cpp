#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib>

#include "gfx.hpp"
#include "math.hpp"
#include "utility.hpp"

struct Particle {
    float angle;
    Vec2 pos;
};

const int n_particle = 1000;
Particle particle[n_particle];
Img trail;

float second_passed;

bool every_seconds(float s, float dt) {
    second_passed += dt;
    if (second_passed >= s) {
        second_passed = 0;
        return true;
    }

    return false;
}

void update(float dt) {
    if (every_seconds(0.001, dt)) {
        img_multiply_scalar(&trail, 0.97);

        float sensor_angle = M_PI_4;
        float sensor_distance = 9;

        Vec2 center = {1, 0};
        Vec2 left = rotate(center, -sensor_angle);
        Vec2 right = rotate(center, sensor_angle);

        Vec2 center_sensor = vec2_scale(center, sensor_distance);
        Vec2 left_sensor = vec2_scale(left, sensor_distance);
        Vec2 right_sensor = vec2_scale(right, sensor_distance);

        for (int i = 0; i < n_particle; ++i) {
            Particle p = particle[i];
            Vec2 pos_center = p.pos + rotate(center_sensor, p.angle);
            Vec2 pos_left = p.pos + rotate(left_sensor, p.angle);
            Vec2 pos_right = p.pos + rotate(right_sensor, p.angle);
            
            float c = img_lookup_nearest(&trail, pos_center).g;
            float l = img_lookup_nearest(&trail, pos_left).g;
            float r = img_lookup_nearest(&trail, pos_right).g;

            Vec2 next_pos;
            float next_angle;
            if (c > r && c > l) {
                next_angle = p.angle;
                next_pos = p.pos + rotate(center, next_angle);
            } else if (r > l && r > c) {
                next_angle = p.angle + sensor_angle;
                next_pos = p.pos + rotate(right, next_angle);
            } else if (l > r && l > c) {
                next_angle = p.angle - sensor_angle;
                next_pos = p.pos + rotate(left, next_angle);
            } else {
                bool go_left = randf() > 0.5;
                next_angle = p.angle + (go_left ? -sensor_angle : sensor_angle);
                next_pos = vec2_add(p.pos, rotate(go_left ? left: right, next_angle));
            }

            particle[i].angle = next_angle;
            particle[i].pos = next_pos;
            int x = particle[i].pos.x;
            int y = particle[i].pos.y;
            if (img_in_bounds(&trail, x, y)) {
                img_draw_point(&trail, particle[i].pos, {1, 1, 1, 1}, 1);
            }
        }
    }
}

void draw(Img* fb) {
    img_set(&trail, 100, 100, {1, 1, 1, 1});
    img_draw_img(fb, &trail, affine_eye(), true);
}

void init() {
    for (int i = 0; i < n_particle; ++i) {
        // particle[i] = {.angle = randf() * 2*M_PI, .pos = {randf()*512, randf()*512}};
        particle[i] = {.angle = randf() * 2*M_PI, .pos = {256, 256}};
    }
    trail = img_create(512, 512);
}


bool running = true;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        running = false;
    }

    // world_key_input(action, key);
}

const int window_width = 1024;
const int window_height = 1024;

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // world_mouse_cursor_position(xpos / window_width, ypos / window_height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // world_mouse_button(button, action);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // world_scroll_input(xoffset, yoffset);
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

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    gfx_init(512, 512);
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
