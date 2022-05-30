#include "gfx.hpp"

#include "utility.hpp"

static Image image;


void gfx_init(int w, int h) {
    image = {
        .w = w,
        .h = h,
        .data = (RGBA*)malloc(w*h*sizeof(RGBA)),
    };
}

RGBA gfx_get(int x, int y) {
    return image.data[y*image.w + x];
}

void gfx_set(int x, int y, RGBA c) {
    image.data[y*image.w + x] = c;
}

int gfx_width() {
    return image.w;
}

bool in_buffer(int x, int y) {
    return x >= 0 and y >= 0 and x < image.w and y < image.h;
}

void add_onto(int x, int y, RGBA c) {
    gfx_set(x, y, clamp01(c + (1-c.a)*gfx_get(x, y)));
}

void gfx_clear_solid(RGBA c) {
    for (int i = 0; i < image.w * image.h; ++i) image.data[i] = c;
}

void gfx_draw_line(Vec2 a, Vec2 b, RGBA color, float thickness) {
    float t = thickness/2;
    int x0 = std::floor(std::min(a.x, b.x) - t);
    int y0 = std::floor(std::min(a.y, b.y) - t);
    int x1 = std::ceil(std::max(a.x, b.x) + t);
    int y1 = std::ceil(std::max(a.y, b.y) + t);
    Vec2 ba = normalize(b - a);
    Vec2 normal = Vec2{-ba.y, ba.x};
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(x, y)) continue;

            const int ss = 4;
            const float factor = 1.f/(ss*ss);
            const float offset = 1.f/(ss*2);
            float value = 0;
            Vec2 p{x - 0.5f, y - 0.5f};
            for (int yi = 0; yi < ss; ++yi) 
                for (int xi = 0; xi < ss; ++xi) {
                    Vec2 xy = p + Vec2{offset + xi*2.f*offset, offset + yi*2.f*offset};
                    float distance = std::abs(dot(xy - a, normal));
                    if (distance < t) {
                        value += factor;
                    }
                }

            add_onto(x, y, value * color);
        }
}

void gfx_draw_point(Vec2 pos, RGBA color, float radius) {
    int x0 = std::floor(pos.x - radius);
    int y0 = std::floor(pos.y - radius);
    int x1 = std::ceil(pos.x + radius);
    int y1 = std::ceil(pos.y + radius);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(x, y)) continue;

            const int ss = 4;
            const float factor = 1.f/(ss*ss);
            const float offset = 1.f/(ss*2);
            float value = 0;
            Vec2 p{x - 0.5f, y - 0.5f};
            for (int yi = 0; yi < ss; ++yi) 
                for (int xi = 0; xi < ss; ++xi) {
                    Vec2 xy = p + Vec2{offset + xi*2.f*offset, offset + yi*2.f*offset};
                    float distance = norm(xy - pos);
                    if (distance < radius) {
                        value += factor;
                    }
                }

            add_onto(x, y, value * color);
        }
}

void gfx_draw_rectangle(Vec2 tl, Vec2 br, RGBA color) {
    int x0 = std::floor(tl.x);
    int y0 = std::floor(tl.y);
    int x1 = std::ceil(br.x);
    int y1 = std::ceil(br.y);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(x, y)) continue;

            const int ss = 4;
            const float factor = 1.f/(ss*ss);
            const float offset = 1.f/(ss*2);
            float value = 0;
            Vec2 p{(float)x, (float)y};  // no 0.5 offset here
            for (int yi = 0; yi < ss; ++yi) 
                for (int xi = 0; xi < ss; ++xi) {
                    Vec2 xy = p + Vec2{offset + xi*2.f*offset, offset + yi*2.f*offset};
                    if (xy.x >= tl.x and xy.x <= br.x and xy.y >= tl.y and xy.y < br.y) {
                        value += factor;
                    }
                }

            add_onto(x, y, value * color);
        }
}


inline RGBA lookup_bilinear(Sprite s, Vec2 p) {
    int x0 = p.x;
    int y0 = p.y;
    float fx = p.x - (float)x0;
    float fy = p.y - (float)y0;
    RGBA tl = s.get(x0, y0);
    RGBA tr = s.get(x0 + 1, y0);
    RGBA br = s.get(x0 + 1, y0 + 1);
    RGBA bl = s.get(x0, y0 + 1);
    return (1 - fx) * ((1-fy) * tl + fy * bl) + fx * ((1-fy) * tr + fy * br);
}

inline RGBA lookup_nearest(Sprite s, Vec2 p) {
    int x = std::floor(p.x);
    int y = std::floor(p.y);
    return s.get(x, y);
}

void gfx_draw_sprite(Sprite s, Affine t, bool bilinear) {
    Vec2 tl = mul(t, Vec2{0, 0});
    Vec2 tr = mul(t, Vec2{s.w, 0});
    Vec2 br = mul(t, Vec2{s.w, s.h});
    Vec2 bl = mul(t, Vec2{0, s.h});
    // print_vec("br", br);
    // print_vec("bl", bl);
    int x0 = std::max(0.f, std::min(tl.x, std::min(tr.x, std::min(br.x, bl.x))));
    int y0 = std::max(0.f, std::min(tl.y, std::min(tr.y, std::min(br.y, bl.y))));
    int x1 = std::min((float)image.w, std::ceil(std::max(tl.x, std::max(tr.x, std::max(br.x, bl.x)))));
    int y1 = std::min((float)image.h, std::ceil(std::max(tl.y, std::max(tr.y, std::max(br.y, bl.y)))));
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
