#include "terrain.hpp"

#include "math.hpp"
#include "gfx.hpp"

// Grass
// Rocks
// Heightmap (mountains and valleys)
// Water
// Sand
// River
// Trees
// Snow


struct TerrainTile {
    float grass = 0;
    float water = 0;
    float trees = 0;
};


struct Terrain {
    int w;
    int h;
    TerrainTile* terrain;
};

static Terrain t;
static Sprite s;
int scale = 10;


float randf() {
    return (float)rand() / RAND_MAX;
}


void terrain_init() {
    t.h = 20;
    t.w = 20;
    t.terrain = (TerrainTile*)malloc(t.h*t.w*sizeof(TerrainTile));

    // grass
    for (int i = 0; i < t.w*t.h; ++i) t.terrain[i] = {.grass = 1, .water = 0, .trees = 0};
    
    // water
    for (int y = 10; y < 15; ++y)
        for (int x = 10; x < 15; ++x) {
            t.terrain[y*t.w + x] = {.water = 1};
        }

    
    // trees
    for (int i = 0; i < 50; ++i) {
        int x = randf() * t.w;
        int y = randf() * t.h;
        if (t.terrain[y*t.w + x].water == 0) {
            t.terrain[y*t.w + x] = {.grass = 1, .trees = randf()*10};
        } else {
            i--;
        }
    }

    s.h = scale*t.h;
    s.w = scale*t.w;
    s.data = (RGBA*)malloc(s.h*s.w*sizeof(RGBA));
}


void terrain_draw() {
    for (int terrain_y = 0; terrain_y < t.h; ++terrain_y) {
        for (int terrain_x = 0; terrain_x < t.w; ++terrain_x) {
            TerrainTile tt = t.terrain[terrain_y*t.w + terrain_x];
            
            if (tt.grass > 0) {
                for (int sprite_y = terrain_y*scale; sprite_y < terrain_y*scale + scale; ++sprite_y) 
                    for (int sprite_x = terrain_x*scale; sprite_x < terrain_x*scale + scale; ++sprite_x) {
                        s.data[sprite_y*s.w + sprite_x] = {0, 0.6, 0, 1};
                    }
            }

            if (tt.water > 0) {
                for (int sprite_y = terrain_y*scale; sprite_y < terrain_y*scale + scale; ++sprite_y) 
                    for (int sprite_x = terrain_x*scale; sprite_x < terrain_x*scale + scale; ++sprite_x) {
                        s.data[sprite_y*s.w + sprite_x] = {0, 0, 0.7, 1};
                    }
            }

            int n_trees = tt.trees * 5;
            for (int i = 0; i < n_trees; ++i) {
                int x = terrain_x*scale + randf() * scale;
                int y = terrain_y*scale + randf() * scale;
                s.data[y*s.w + x] = {0.66, 0.28, 0, 1};
            }
        }
    }

    int w = gfx_width();
    gfx_draw_sprite(s, from_scale((float)w/(float)s.w), false);
}
