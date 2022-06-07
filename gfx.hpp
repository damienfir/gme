#ifndef GFX_HPP
#define GFX_HPP

#include <stdio.h>
#include "math.hpp"

struct RGB {
    float r = 0;
    float g = 0;
    float b = 0;
};

inline RGB rgb_div(RGB c, float d) {
    return {c.r / d, c.g / d, c.b / d};
}

inline RGB rgb_clamp(RGB c) {
    return {
        fminf(1, fmaxf(0, c.r)),
        fminf(1, fmaxf(0, c.b)),
        fminf(1, fmaxf(0, c.g)),
    };
}

struct RGBA {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;
};

inline RGBA operator*(float a, RGBA v) {
    return RGBA{v.r * a, v.g * a, v.b * a, v.a * a};
}

inline RGBA operator*(RGBA a, RGBA b) {
    return RGBA{a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}

inline RGBA operator/(RGBA v, float a) {
    return RGBA{v.r / a, v.g / a, v.b / a, v.a / a};
}

inline RGBA operator+(RGBA a, RGBA b) {
    return RGBA{a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
}

inline RGBA rgba_add(RGBA a, RGBA b) {
    return RGBA{a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
}

inline RGBA& operator+=(RGBA& a, RGBA b) {
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    a.a += b.a;
    return a;
}

inline RGBA rgba_clamp(RGBA a) {
    RGBA b;
    b.r = fminf(1, fmaxf(0, a.r));
    b.g = fminf(1, fmaxf(0, a.g));
    b.b = fminf(1, fmaxf(0, a.b));
    b.a = fminf(1, fmaxf(0, a.a));
    return b;
}

inline RGBA clamp01(RGBA a) {
    return rgba_clamp(a);
}

inline RGBA rgba_from_hex(int hex) {
    int r = (hex >> 16) & 0xff;
    int g = (hex >> 8) & 0xff;
    int b = hex & 0xff;
    return {r / 255.f, g / 255.f, b / 255.f, 1.f};
}

struct Sprite {
    int w;
    int h;
    RGBA* data;

    inline RGBA get(int x, int y) {
        if (x < 0 || x >= w || y < 0 || y >= w) {
            return {};
        }
        return data[y * w + x];
    }
};

// Drawable image
struct Image {
    int w;
    int h;
    RGBA* data;
};

Image image_create(int w, int h);
void image_set(Image*, int x, int y, RGBA);
RGBA image_get(Image*, int x, int y);

void gfx_init(int w, int h);
RGBA gfx_get(int x, int y);
void gfx_set(int x, int y, RGBA c);
int gfx_width();
int gfx_height();
void gfx_clear_solid(RGBA c = {});
void gfx_draw_line(Vec2 a, Vec2 b, RGBA color, float thickness);
void gfx_draw_point(Vec2 pos, RGBA color, float radius);
void gfx_draw_rectangle(Vec2 tl, Vec2 br, RGBA color);
void gfx_draw_sprite(Image* s, Affine t, bool bilinear);

#endif /* GFX_HPP */
