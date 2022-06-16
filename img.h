#ifndef IMG_H
#define IMG_H

#include "math.hpp"

struct RGBA {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;
};

inline RGBA rgba_from_hex(int hex) {
    int r = (hex >> 16) & 0xff;
    int g = (hex >> 8) & 0xff;
    int b = hex & 0xff;
    return {r / 255.f, g / 255.f, b / 255.f, 1.f};
}

inline RGBA rgba_add(RGBA a, RGBA b) {
    return RGBA{a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
}

inline RGBA rgba_scale(RGBA v, float a) {
    return {v.r * a, v.g * a, v.b * a, v.a * a};
}

inline RGBA rgba_mul(RGBA a, RGBA b) {
    return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}

RGBA rgba_clamp(RGBA a);

struct Img {
    int w;
    int h;
    RGBA* data;
};

inline bool img_in_bounds(Img* img, int x, int y) {
    return x >= 0 and y >= 0 and x < img->w and y < img->h;
}

inline RGBA img_get(Img* img, int x, int y) {
    return img->data[y * img->w + x];
}

inline RGBA img_get_maybe(Img* img, int x, int y, RGBA otherwise) {
    if (!img_in_bounds(img, x, y)) return otherwise;
    return img_get(img, x, y);
}

inline void img_set(Img* img, int x, int y, RGBA color) {
    assert(img_in_bounds(img, x, y));
    img->data[y * img->w + x] = color;
}

inline void img_add_onto(Img* img, int x, int y, RGBA c) {
    c = rgba_add(c, rgba_scale(img_get(img, x, y), 1 - c.a));
    img_set(img, x, y, rgba_clamp(c));
}

inline RGBA img_lookup_bilinear(Img* img, Vec2 p) {
    int x0 = p.x;
    int y0 = p.y;
    float fx = p.x - (float)x0;
    float fy = p.y - (float)y0;
    RGBA tl = img_get_maybe(img, x0, y0, {});
    RGBA tr = img_get_maybe(img, x0 + 1, y0, {});
    RGBA br = img_get_maybe(img, x0 + 1, y0 + 1, {});
    RGBA bl = img_get_maybe(img, x0, y0 + 1, {});

    RGBA left = rgba_add(rgba_scale(tl, 1-fy), rgba_scale(bl, fy));
    RGBA right = rgba_add(rgba_scale(tr, 1-fy), rgba_scale(br, fy));
    return rgba_add(rgba_scale(left, 1-fx), rgba_scale(right, fx));
}

inline RGBA img_lookup_nearest(Img* img, Vec2 p) {
    int x = floor(p.x);
    int y = floor(p.y);
    if (!img_in_bounds(img, x, y))
        return {};
    return img_get(img, x, y);
}

Img img_create(int w, int h);
void img_destroy(Img*);
void img_solid(Img*, RGBA);
void img_draw_img(Img* img, Img* other, Affine t, bool bilinear);
void img_draw_line(Img* img, Vec2 a, Vec2 b, RGBA color, float thickness);
void img_draw_point(Img* img, Vec2 pos, RGBA color, float radius);
void img_multiply_scalar(Img* img, float s);

#endif /* IMG_H */
