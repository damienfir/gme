#include "gfx.hpp"
#include <GL/glu.h>
#include "img.h"


struct GLTexture {
    GLuint id;
    int w;
    int h;
    unsigned char* data;
};

static GLTexture tex;
static Img framebuffer;

void gfx_init(int w, int h) {
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    tex.w = w;
    tex.h = h;
    tex.data = (unsigned char*)malloc(w * h * 3 * sizeof(unsigned char));

    framebuffer = img_create(w, h);
}

void gfx_clear() {
    img_solid(&framebuffer, {});
}

Img* gfx_get_framebuffer() {
    return &framebuffer;
}

void gfx_draw() {
    for (int y = 0; y < tex.h; ++y)
        for (int x = 0; x < tex.w; ++x) {
            RGBA color = img_get(&framebuffer, x, y);
            tex.data[y * tex.w * 3 + x * 3 + 0] = color.r * 255;
            tex.data[y * tex.w * 3 + x * 3 + 1] = color.g * 255;
            tex.data[y * tex.w * 3 + x * 3 + 2] = color.b * 255;
        }

    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.w, tex.h, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.data);

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
