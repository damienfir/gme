#ifndef GFX_HPP
#define GFX_HPP

#include "img.h"

void gfx_init(int w, int h);
void gfx_clear();
Img* gfx_get_framebuffer();
void gfx_draw();

#endif /* GFX_HPP */
