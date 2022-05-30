#ifndef GFX_HPP
#define GFX_HPP

#include "math.hpp"

struct RGBA {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;
};

inline RGBA operator*(float a, RGBA v) {
    return RGBA{v.r*a, v.g*a, v.b*a, v.a*a};
}

inline RGBA operator+(RGBA a, RGBA b) {
    return RGBA{a.r+b.r, a.g+b.g, a.b+b.b, a.a+b.a};
}

inline RGBA& operator+=(RGBA &a, RGBA b) {
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    a.a += b.a;
    return a;
}

inline RGBA clamp01(RGBA a) {
    RGBA b;
    b.r = std::min(1.f, std::max(0.f, a.r));
    b.g = std::min(1.f, std::max(0.f, a.g));
    b.b = std::min(1.f, std::max(0.f, a.b));
    b.a = std::min(1.f, std::max(0.f, a.a));
    return b;
}


struct Sprite {
    int w;
    int h;
    RGBA* data;

    inline RGBA get(int x, int y) {
        if (x < 0 || x >= w || y < 0 || y >= w) {
            return {};
        }
        return data[y*w+x];
    }
};


// Drawable image
struct Image {
    int w;
    int h;
    RGBA* data;
};

void gfx_init(int w, int h);
RGBA gfx_get(int x, int y);
void gfx_set(int x, int y, RGBA c);
int gfx_width();
void gfx_clear_solid(RGBA c = {});
void gfx_draw_line(Vec2 a, Vec2 b, RGBA color, float thickness);
void gfx_draw_point(Vec2 pos, RGBA color, float radius);
void gfx_draw_rectangle(Vec2 tl, Vec2 br, RGBA color);
void gfx_draw_sprite(Sprite s, Affine t, bool bilinear);

#endif /* GFX_HPP */
