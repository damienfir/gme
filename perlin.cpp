#include "perlin.h"

#include "math.hpp"
#include "debug.h"

Vec2 random_unit_vector() {
    float angle = rand() % 360 * 3.1415f / 180.f;
    return {std::cos(angle), std::sin(angle)};
}

Vec2* create_random_gradient_grid(int w, int h) {
    Vec2* grid = (Vec2*)malloc(w * h * sizeof(Vec2));

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            grid[y * w + x] = random_unit_vector();
        }

    return grid;
}

float smoothstep(float x) {
    return x * x * (3 - 2 * x);
}

float lerp(float a, float b, float f) {
    return (1 - f) * a + f * b;
}

float perlin_noise(Vec2* gradients, int w, float x, float y) {
    float x0 = floor(x);
    float x1 = x0 + 1;
    float y0 = floor(y);
    float y1 = y0 + 1;
    float fx = smoothstep(x - x0);
    float fy = smoothstep(y - y0);
    float dot00 = vec2_dot({x - x0, y - y0}, gradients[(int)y0 * w + (int)x0]);
    float dot10 = vec2_dot({x - x1, y - y0}, gradients[(int)y0 * w + (int)x1]);
    float dot01 = vec2_dot({x - x0, y - y1}, gradients[(int)y1 * w + (int)x0]);
    float dot11 = vec2_dot({x - x1, y - y1}, gradients[(int)y1 * w + (int)x1]);
    float xa = lerp(dot00, dot10, fx);
    float xb = lerp(dot01, dot11, fx);
    return lerp(xa, xb, fy);
}

float* create_perlin_grid(int w, int h, float scale) {
    float* grid = (float*)malloc(w * h * sizeof(float));
    int gradients_w = ceil(w * scale);
    int gradients_h = ceil(h * scale);
    Vec2* gradients = create_random_gradient_grid(gradients_w, gradients_h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            grid[y * w + x] = perlin_noise(gradients, gradients_w, x * scale, y * scale);
        }
    free(gradients);
    return grid;
}

