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
Image trail;

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
        image_multiply_scalar(&trail, 0.99);

        float sensor_angle = M_PI_4;
        float sensor_distance = 5;

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
            
            float c = lookup_nearest(&trail, pos_center).g;
            float l = lookup_nearest(&trail, pos_left).g;
            float r = lookup_nearest(&trail, pos_right).g;

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
            if (image_in_bounds(&trail, x, y)) {
                image_set(&trail, x, y, {1, 1, 1, 1});
            }
        }
    }
}

void draw() {
    gfx_draw_sprite(&trail, affine_eye(), false);
}

void init() {
    for (int i = 0; i < n_particle; ++i) {
        particle[i] = {.angle = randf() * 2*M_PI, .pos = {randf()*512, randf()*512}};
        // particle[i] = {.angle = randf() * 2*M_PI, .pos = {100, 100}};
    }
    trail = image_create(512, 512);
}


struct Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> previous_timestamp;

    void start() { previous_timestamp = std::chrono::high_resolution_clock::now(); }

    float tick() {
        auto now = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - previous_timestamp).count();
        previous_timestamp = now;
        // if (ms > 0) {
        //     std::cout << "fps: " << (1000.0/ms) << std::endl;
        // }
        return ms / 1000.f;
    }

    //     void sync(int ms) {
    //         auto now = std::chrono::high_resolution_clock::now();
    //         auto elapsed = now - previous_timestamp;
    //         if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < ms) {
    //             std::this_thread::sleep_for(std::chrono::milliseconds(ms) - elapsed);
    //         }
    //         previous_timestamp = now;
    //     }
};

struct Buffer {
    int w;
    int h;
    unsigned char* data;
};

struct Texture {
    GLuint id;
    Buffer buffer;
};

static Texture tex;
bool running = true;

void init_texture() {
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int w = 256;
    int h = 256;
    tex.buffer = {
        .w = w,
        .h = h,
        .data = (unsigned char*)malloc(w * h * 3 * sizeof(unsigned char)),
    };

    gfx_init(w, h);
}

void draw_texture() {
    for (int y = 0; y < tex.buffer.h; ++y)
        for (int x = 0; x < tex.buffer.w; ++x) {
            RGBA im = gfx_get(x, y);
            tex.buffer.data[y * tex.buffer.w * 3 + x * 3 + 0] = im.r * 255;
            tex.buffer.data[y * tex.buffer.w * 3 + x * 3 + 1] = im.g * 255;
            tex.buffer.data[y * tex.buffer.w * 3 + x * 3 + 2] = im.b * 255;
        }

    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.buffer.w, tex.buffer.h, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.buffer.data);

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2d(0, 1);
    glVertex2d(-1, -1);
    glTexCoord2d(1, 1);
    glVertex2d(1, -1);
    glTexCoord2d(1, 0);
    glVertex2d(1, 1);
    glTexCoord2d(0, 0);
    glVertex2d(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

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

    init_texture();
    init();

    while (running) {
        // if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
        // state.controls.use_js = true;
        // joystick_control(GLFW_JOYSTICK_1);
        // }

        float dt = timer_dt.tick();
        printf("fps: %.2f\n", 1 / dt);
        update(dt);

        // terrain_draw();
        gfx_clear_solid({});
        draw();
        draw_texture();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
