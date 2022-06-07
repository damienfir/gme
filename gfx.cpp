#include "gfx.hpp"

#include "utility.hpp"

static Image image;

Image image_create(int w, int h) {
    return {
        .w = w,
        .h = h,
        .data = (RGBA*)malloc(w * h * sizeof(RGBA)),
    };
}

void image_set(Image* im, int x, int y, RGBA c) {
    im->data[y * im->w + x] = c;
}

RGBA image_get(Image* im, int x, int y) {
    return im->data[y * im->w + x];
}

void gfx_init(int w, int h) {
    image = image_create(w, h);
}

RGBA gfx_get(int x, int y) {
    return image_get(&image, x, y);
}

void gfx_set(int x, int y, RGBA c) {
    image_set(&image, x, y, c);
}

int gfx_width() {
    return image.w;
}

int gfx_height() {
    return image.h;
}

bool in_buffer(int x, int y) {
    return x >= 0 and y >= 0 and x < image.w and y < image.h;
}

void add_onto(int x, int y, RGBA c) {
    gfx_set(x, y, clamp01(c + (1 - c.a) * gfx_get(x, y)));
}

void gfx_clear_solid(RGBA c) {
    for (int i = 0; i < image.w * image.h; ++i)
        image.data[i] = c;
}

void gfx_draw_line(Vec2 a, Vec2 b, RGBA color, float thickness) {
    float t = thickness / 2;
    int x0 = floor(fmin(a.x, b.x) - t);
    int y0 = floor(fmin(a.y, b.y) - t);
    int x1 = ceil(fmax(a.x, b.x) + t);
    int y1 = ceil(fmax(a.y, b.y) + t);
    Vec2 ba = normalize(b - a);
    Vec2 normal = Vec2{-ba.y, ba.x};
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(x, y))
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

            add_onto(x, y, value * color);
        }
}

void gfx_draw_point(Vec2 pos, RGBA color, float radius) {
    int x0 = floor(pos.x - radius);
    int y0 = floor(pos.y - radius);
    int x1 = ceil(pos.x + radius);
    int y1 = ceil(pos.y + radius);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(x, y))
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

            add_onto(x, y, value * color);
        }
}

void gfx_draw_rectangle(Vec2 tl, Vec2 br, RGBA color) {
    int x0 = floor(tl.x);
    int y0 = floor(tl.y);
    int x1 = ceil(br.x);
    int y1 = ceil(br.y);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(x, y))
                continue;

            const int ss = 4;
            const float factor = 1.f / (ss * ss);
            const float offset = 1.f / (ss * 2);
            float value = 0;
            Vec2 p{(float)x, (float)y};  // no 0.5 offset here
            for (int yi = 0; yi < ss; ++yi)
                for (int xi = 0; xi < ss; ++xi) {
                    Vec2 xy = p + Vec2{offset + xi * 2.f * offset, offset + yi * 2.f * offset};
                    if (xy.x >= tl.x and xy.x <= br.x and xy.y >= tl.y and xy.y < br.y) {
                        value += factor;
                    }
                }

            add_onto(x, y, value * color);
        }
}

inline RGBA lookup_bilinear(Image* s, Vec2 p) {
    int x0 = p.x;
    int y0 = p.y;
    float fx = p.x - (float)x0;
    float fy = p.y - (float)y0;
    RGBA tl = image_get(s, x0, y0);
    RGBA tr = image_get(s, x0 + 1, y0);
    RGBA br = image_get(s, x0 + 1, y0 + 1);
    RGBA bl = image_get(s, x0, y0 + 1);
    return (1 - fx) * ((1 - fy) * tl + fy * bl) + fx * ((1 - fy) * tr + fy * br);
}

inline RGBA lookup_nearest(Image* s, Vec2 p) {
    int x = floor(p.x);
    int y = floor(p.y);
    if (x < 0 || y < 0 || x >= s->w || y >= s->h)
        return {};
    return image_get(s, x, y);
}

void gfx_draw_sprite(Image* s, Affine t, bool bilinear) {
    Vec2 tl = mul(t, Vec2{0.f, 0.f});
    Vec2 tr = mul(t, Vec2{(float)s->w, 0.f});
    Vec2 br = mul(t, Vec2{(float)s->w, (float)s->h});
    Vec2 bl = mul(t, Vec2{0.f, (float)s->h});
    // print_vec("br", br);
    // print_vec("bl", bl);
    int x0 = fmax(0.f, fmin(tl.x, fmin(tr.x, fmin(br.x, bl.x))));
    int y0 = fmax(0.f, fmin(tl.y, fmin(tr.y, fmin(br.y, bl.y))));
    int x1 = fmin((float)image.w, ceil(fmax(tl.x, fmax(tr.x, fmax(br.x, bl.x)))));
    int y1 = fmin((float)image.h, ceil(fmax(tl.y, fmax(tr.y, fmax(br.y, bl.y)))));
    // printf("x0: %d, x1: %d, y0: %d, y1: %d\n", x0, x1, y0, y1);

    Affine ti = inverse(t);
    for (int tex_y = y0; tex_y < y1; ++tex_y)
        for (int tex_x = x0; tex_x < x1; ++tex_x) {
            Vec2 sprite_xy = mul(ti, Vec2{(float)tex_x, (float)tex_y});
            // add_onto(tex_x, tex_y, lookup_nearest(s, sprite_xy));
            RGBA c = bilinear ? lookup_bilinear(s, sprite_xy) : lookup_nearest(s, sprite_xy);
            add_onto(tex_x, tex_y, c);
        }
}
