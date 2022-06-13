
Water water[10];

struct Water {
    int w = 0;
    int h = 0;
    int x;
    int y;
    float *heightmap;
    float *velocity;
    float *temp;
    bool* mask;
};

void water_add(int x, int y, int width, int height) {
    int id = 0;
    while (id < 10) {
        if (water[id].h == 0) break;
        id++;
    }
    assert(id < 10);

    Water *w = &water[id];
    w->x = x;
    w->y = y;
    w->h = height;
    w->w = width;
    w->heightmap = (float*)malloc(height*width*sizeof(float));
    w->temp = (float*)malloc(height*width*sizeof(float));
    w->velocity = (float*)malloc(height*width*sizeof(float));
    w->mask = (bool*)malloc(height*width*sizeof(bool));

    for (int i = 0; i < width*height; ++i) w->mask[i] = 1;
    
    // Define boundaries
    for (int y = 0; y < height; ++y) {
        w->mask[y * width] = 0;
        w->mask[y * width + width - 1] = 0;
    }

    for (int x = 0; x < width; ++x) {
        w->mask[x] = 0;
        w->mask[(height-1) * width + x] = 0;
    }

    w->mask[10 * width + 10] = 0;
    w->mask[11 * width + 10] = 0;
    w->mask[12 * width + 10] = 0;
    w->mask[13 * width + 10] = 0;
    w->mask[14 * width + 10] = 0;
    w->mask[15 * width + 10] = 0;
    w->mask[16 * width + 10] = 0;
    w->mask[17 * width + 10] = 0;
    w->mask[18 * width + 10] = 0;

    for (int i = 0; i < width*height; ++i) {
        if (!w->mask[i]) continue;

        w->heightmap[i] = 0;
        w->velocity[i] = 0;

        int xi = i % width;
        int yi = i / width;
        int world_xy = (y + yi) * world_x + (x + xi);
        ground.mat[world_xy] = WATER;
        rendering.normal[world_xy] = {0, 0, 1};
        rendering.albedo[world_xy] = {0, 0, 1};
    }

    w->heightmap[10 * width + 1] = 3;
    w->heightmap[11 * width + 1] = 3;

}

void water_update(float dt) {
    for (int i = 0; i < 10; ++i) {
        if (water[i].h == 0) continue;

        Water* w = &water[i];

        bool* m = w->mask;
        float* h = w->heightmap;

        for (int y = 0; y < w->h; ++y) {
            for (int x = 0; x < w->w; ++x) {
                int xy = y * w->w + x;
                if (!m[xy]) continue;

                int x0y = m[xy - 1] ? xy - 1 : xy;
                int x1y = m[xy + 1] ? xy + 1 : xy;
                int xy0 = m[xy - w->w] ? xy - w->w : xy;
                int xy1 = m[xy + w->w] ? xy + w->w : xy;
                int x0y0 = m[xy0 - 1] ? xy0 - 1 : xy0;
                int x1y0 = m[xy0 + 1] ? xy0 + 1 : xy0;
                int x0y1 = m[xy1 - 1] ? xy1 - 1 : xy1;
                int x1y1 = m[xy1 + 1] ? xy1 + 1 : xy1;

                const float c2 = 100;
                float sq2 = 1/sqrtf(2);
                float s = 4*sq2 + 4;
                float f = c2 * (
                        sq2 * h[x0y0] + sq2 * h[x0y1] + sq2 * h[x1y1] + sq2 * h[x1y0]
                        + h[x0y] + h[x1y] + h[xy0] + h[xy1]
                        - s * h[xy]
                        ) / s;
                w->velocity[xy] += f * dt;
                w->velocity[xy] *= 0.998;
                w->temp[xy] = h[xy] + w->velocity[xy] * dt;

                int world_xy = (w->y + y) * world_x + (w->x + x);
                float nx = (h[x1y] - h[x0y]) / 2;
                float ny = (h[xy1] - h[xy0]) / 2;
                Vec3 normal = vec3_normalize({nx, ny, 1});
                rendering.normal[world_xy] = normal;
            }
        }

        float* swapped = w->heightmap;
        w->heightmap = w->temp;
        w->temp = swapped;
    }
}

void water_touch(Vec2 pos) {
    int x = (int)pos.x;
    int y = (int)pos.y;

    for (int i = 0; i < 10; ++i) {
        if (water[i].h == 0) continue;

        Water w = water[i];
        if (x >= w.x && x < w.x + w.w && y >= w.y && y < w.y + w.h) {
            int water_xy = (y - w.y) * w.w + (x - w.x);
            w.heightmap[water_xy] = 3;
            break;
        }
    }
}
