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

struct Img {
    int w;
    int h;
    RGBA* data;
};

Img img_create(int w, int h);
void img_destroy(Img*);
RGBA img_get(Img*, int x, int y);
void img_solid(RGBA);
void img_set(Img* img, int x, int y, RGBA color);
void img_draw_img(Img* img, Img* other, struct Affine t, bool bilinear);
void img_draw_line(Img* img, struct Vec2 a, struct Vec2 b, RGBA color, float thickness);
void img_draw_point(Img* img, struct Vec2 pos, RGBA color, float radius);
