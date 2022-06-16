#include "img.h"
#include <stdlib.h>

RGBA rgba_clamp(RGBA a) {
    RGBA b;
    b.r = fminf(1, fmaxf(0, a.r));
    b.g = fminf(1, fmaxf(0, a.g));
    b.b = fminf(1, fmaxf(0, a.b));
    b.a = fminf(1, fmaxf(0, a.a));
    return b;
}

Img img_create(int w, int h) {
    return {
        .w = w,
        .h = h,
        .data = (RGBA*)malloc(w * h * sizeof(RGBA)),
    };
}

void img_destroy(Img* img) {
    free(img->data);
}


void img_solid(Img* img, RGBA color) {
    for (int i = 0; i < img->w*img->h; ++i) img->data[i] = color;
}

void img_draw_img(Img* img, Img* other, Affine t, bool bilinear) {
    Vec2 tl = mul(t, Vec2{0.f, 0.f});
    Vec2 tr = mul(t, Vec2{(float)other->w, 0.f});
    Vec2 br = mul(t, Vec2{(float)other->w, (float)img->h});
    Vec2 bl = mul(t, Vec2{0.f, (float)other->h});
    int x0 = fmax(0.f, fmin(tl.x, fmin(tr.x, fmin(br.x, bl.x))));
    int y0 = fmax(0.f, fmin(tl.y, fmin(tr.y, fmin(br.y, bl.y))));
    int x1 = fmin((float)img->w, ceil(fmax(tl.x, fmax(tr.x, fmax(br.x, bl.x)))));
    int y1 = fmin((float)img->h, ceil(fmax(tl.y, fmax(tr.y, fmax(br.y, bl.y)))));

    Affine ti = inverse(t);
    for (int tex_y = y0; tex_y < y1; ++tex_y)
        for (int tex_x = x0; tex_x < x1; ++tex_x) {
            Vec2 sprite_xy = mul(ti, Vec2{(float)tex_x, (float)tex_y});
            RGBA c = bilinear ? img_lookup_bilinear(other, sprite_xy) : img_lookup_nearest(other, sprite_xy);
            img_add_onto(img, tex_x, tex_y, c);
        }
}

void img_draw_line(Img* img, Vec2 a, Vec2 b, RGBA color, float thickness) {
    float t = thickness / 2;
    int x0 = floor(fmin(a.x, b.x) - t);
    int y0 = floor(fmin(a.y, b.y) - t);
    int x1 = ceil(fmax(a.x, b.x) + t);
    int y1 = ceil(fmax(a.y, b.y) + t);
    Vec2 ba = normalize(b - a);
    Vec2 normal = Vec2{-ba.y, ba.x};
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!img_in_bounds(img, x, y))
                continue;

            const int ss = 4;
            const float factor = 1.f / (ss * ss);
            const float offset = 1.f / (ss * 2);
            float value = 0;
            Vec2 p{x - 0.5f, y - 0.5f};
            for (int yi = 0; yi < ss; ++yi)
                for (int xi = 0; xi < ss; ++xi) {
                    Vec2 xy = p + Vec2{offset + xi * 2.f * offset, offset + yi * 2.f * offset};
                    float distance = fabs(dot(xy - a, normal));
                    if (distance < t) {
                        value += factor;
                    }
                }

            img_add_onto(img, x, y, rgba_scale(color, value));
        }
}

void img_draw_point(Img* img, Vec2 pos, RGBA color, float radius) {
    int x0 = floor(pos.x - radius);
    int y0 = floor(pos.y - radius);
    int x1 = ceil(pos.x + radius);
    int y1 = ceil(pos.y + radius);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!img_in_bounds(img, x, y))
                continue;

            const int ss = 4;
            const float factor = 1.f / (ss * ss);
            const float offset = 1.f / (ss * 2);
            float value = 0;
            Vec2 p{x - 0.5f, y - 0.5f};
            for (int yi = 0; yi < ss; ++yi)
                for (int xi = 0; xi < ss; ++xi) {
                    Vec2 xy = p + Vec2{offset + xi * 2.f * offset, offset + yi * 2.f * offset};
                    float distance = norm(xy - pos);
                    if (distance < radius) {
                        value += factor;
                    }
                }

            img_add_onto(img, x, y, rgba_scale(color, value));
        }
}


void img_multiply_scalar(Img* img, float s) {
    for (int i = 0; i < img->w*img->h; ++i) {
        img->data[i] = rgba_scale(img->data[i], s);
    }
}

