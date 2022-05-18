#include <GLFW/glfw3.h>
// #include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>
#include <vector>
#include <png.h>

#include "math.h"

struct Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> previous_timestamp;

    void start() {
        previous_timestamp = std::chrono::high_resolution_clock::now();
    }

    float tick() {
        auto now = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - previous_timestamp).count();
        previous_timestamp = now;
        // if (ms > 0) {
        //     std::cout << "fps: " << (1000.0/ms) << std::endl;
        // }
        return ms/1000.f;
    }

    void sync(int ms) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - previous_timestamp;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms) - elapsed);
        }
        previous_timestamp = now;
    }
};

void print_vec(const char* name, Vec2 v) {
    printf("%s: %.2f, %.2f\n", name, v.x, v.y);
}

struct Rect {
    float x;
    float y;
    float w;
    float h;
};

struct Animation {
    bool animating = false;
    float value_start;
    float value_end;
    float value;
    float animation_time;
    float animation_duration;

    // https://en.wikipedia.org/wiki/Smoothstep
    float smootherstep(float x) {
        return x * x * x * (x * (x * 6 - 15) + 10);
    }

    void start(float begin, float end, float duration) {
        value_start = begin;
        value_end = end;
        animation_time = 0;
        animation_duration = duration;
        animating = true;
    }

    void update(float dt) {
        if (animating) {
            animation_time += dt;
            float x = smootherstep(animation_time / animation_duration);
            value = value_start * (1-x) + value_end * x;

            if (x >= 1) {
                value = value_end;
                animating = false;
                return;
            }
        }
    }
};


struct Camera {
    Vec2 position;  // offset from game grid top-left (0,0)
    float size_x;  // size on game grid
    float size_y;
    int res_x;  // texture resolution
    int res_y;

    Animation pos_animation_x;
    Animation pos_animation_y;
    Animation size_animation_x;
    Animation size_animation_y;

    Vec2 to_viewport(Vec2 v) {
        return Vec2{
            .x = (v.x - position.x) * res_x / size_x,
            .y = (v.y - position.y) * res_y / size_y
        };
    }

    float to_viewport(float a) {
        return a * res_x / size_x;
    }

    Vec2 from_viewport(Vec2 v) {
        return Vec2{
            .x = (v.x * size_x / res_x) + position.x,
            .y = (v.y * size_y / res_y) + position.y,
        };
    }

    void set_target_view(Vec2 new_position, float new_size_x, float new_size_y) {
        pos_animation_x.start(position.x, new_position.x, 0.5);
        pos_animation_y.start(position.y, new_position.y, 0.5);
        size_animation_x.start(size_x, new_size_x, 0.5);
        size_animation_y.start(size_y, new_size_y, 0.5);
    }
    
    void update(float dt) {
        pos_animation_x.update(dt);
        pos_animation_y.update(dt);
        size_animation_x.update(dt);
        size_animation_y.update(dt);
        if (pos_animation_x.animating)
            position.x = pos_animation_x.value;
        if (pos_animation_y.animating)
            position.y = pos_animation_y.value;
        if (size_animation_x.animating)
            size_x = size_animation_x.value;
        if (size_animation_y.animating)
            size_y = size_animation_y.value;
    }
};


struct Door {
    int door_id = -1;
    int animation_id = -1;
    Vec2 a;
    Vec2 b;
    int room0;
    int room1;
    bool closed;
};

struct Room {
    Vec2 bbox[2];

    Vec2 center() {
        return (bbox[1] - bbox[0]) / 2.f;
    }
};

struct Trigger {
    bool active = false;

    bool get() {
        if (active) {
            active = false;
            return true;
        }
        return false;
    }

    void set() {
        active = true;
    }
};

struct Controls {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    Trigger open_door;
    bool use_js;
    Vec2 js;
    Vec2 mouse;
};

struct Player {
    enum State {
        PLAYER_FLYING,
        PLAYER_ON_WALL
    };

    Vec2 pos;
    Vec2 vel;
    float size;
    State state;
};

struct Sprite {
    GLuint tex;
    float x0;
    float y0;
    float x1;
    float y1;
};

struct Buffer {
    int w;
    int h;
    unsigned char* data;
};

struct Image {
    int w;
    int h;
    RGBA* data;

    RGBA& at(int x, int y) {
        return data[y * w + x];
    }
};

struct Texture {
    GLuint id;
    Buffer buffer;
    Image image;
};

struct StarField {
    int n_stars;
    Vec2* position;
    float* size;
    float* brightness;
};

struct Water {
    int w;
    int h;
    float* heightmap;
    float* vel;
    float* other;

    float& at(int x, int y) {
        return heightmap[y * w + x];
    }

    float& other_at(int x, int y) {
        return other[y * w + x];
    }

    float& vel_at(int x, int y) {
        return vel[y * w + x];
    }
};

struct Wall {
    Vec2 a;
    Vec2 b;
};

struct Animations {
    std::vector<Animation> anims;
    std::vector<int> free;

    int add() {
        while (!free.empty()) {
            int id = free.back();
            free.pop_back();
            if (id < (int)anims.size()) {
                return id;
            }
        }

        anims.push_back(Animation{});
        return anims.size()-1;
    }

    void remove(int id) {
        free.push_back(id);
        while (!active(anims.size()-1)) {
            anims.pop_back();
        }
    }

    bool active(int id) {
        return id < (int)anims.size() and anims[id].animating;
    }

    Animation* get(int id) {
        return &anims[id];
    }

    int size() {
        return anims.size();
    }
};

struct GameState {
    std::vector<Wall> walls;
    std::vector<Room> rooms;
    std::vector<Door> doors;
    Player player;
    Controls controls;
    bool running;
    Texture tex;
    StarField stars;
    Water water;
    Camera camera;
    Animations animations;
};

GameState state;

void open_or_close_door(Door* d) {
    if (d->animation_id >= 0) return;

    int id = state.animations.add();
    d->animation_id = id;
    Animation* a = state.animations.get(id);
    if (d->closed) {
        a->start(0, 1, 1);
        d->closed = false;
    } else {
        a->start(1, 0, 1);
        d->closed = true;
    }
}

void set_door_openness(Door d, float openness) {
    Wall* w = &state.walls[d.door_id];
    Vec2 slide_vector = openness * (d.a - d.b);
    w->a = d.a + slide_vector;
    w->b = d.b + slide_vector;
}

void add_door(Door d) {
    int id = state.walls.size();
    d.door_id = id;
    Wall w = {.a = d.a, .b = d.b};
    state.walls.push_back(w);
    state.doors.push_back(d);
    set_door_openness(d, d.closed ? 0 : 1);
}

bool is_door(int wall_id) {
    for (auto& d : state.doors) {
        if (d.door_id == wall_id)
            return true;
    }

    return false;
}

float randf() {
    return (float)rand() / RAND_MAX;
}

void generate_star_field(int width, int height) {
    StarField& f = state.stars;

    f.n_stars = 500;

    f.position = (Vec2*)malloc(f.n_stars*sizeof(Vec2));
    f.brightness = (float*)malloc(f.n_stars*sizeof(float));
    f.size = (float*)malloc(f.n_stars*sizeof(float));

    for (int i = 0; i < f.n_stars; ++i) {
        f.position[i] = Vec2{randf()*width, randf()*height};
        f.size[i] = randf()*10;
        f.brightness[i] = randf();
    }
}


void init_texture() {
    glGenTextures(1, &state.tex.id);
    glBindTexture(GL_TEXTURE_2D, state.tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int w = state.camera.res_x;
    int h = state.camera.res_y;
    state.tex.buffer = {
        .w = w,
        .h = h,
        .data = (unsigned char*)malloc(w*h*3*sizeof(unsigned char)),
    };

    state.tex.image = {
        .w = w,
        .h = h,
        .data = (RGBA*)malloc(w*h*sizeof(RGBA)),
    };
}

void init_solid(RGBA c = {}) {
    Image* im = &state.tex.image;
    for (int i = 0; i < im->w * im->h; ++i) im->data[i] = c;
}


void init_water(int width, int height) {
    state.water.h = height;
    state.water.w = width;
    state.water.heightmap = (float*)malloc(height*width*sizeof(float));
    state.water.other = (float*)malloc(height*width*sizeof(float));
    state.water.vel = (float*)malloc(height*width*sizeof(float));
    for (int i = 0; i < width*height; ++i) {
        state.water.heightmap[i] = 1;
        state.water.vel[i] = 0;
    }

    state.water.at(50, 50) = 4;
}

void update_water(float dt) {
    Water& w = state.water;
    for (int y = 0; y < w.h; ++y)
        for (int x = 0; x < w.w; ++x) {
            const float c2 = 100;

            int x0 = clamp(x-1, 0, w.w-1);
            int x1 = clamp(x+1, 0, w.w-1);
            int y0 = clamp(y-1, 0, w.h-1);
            int y1 = clamp(y+1, 0, w.h-1);

            float sq2 = 1/std::sqrt(2);
            float s = 4*sq2 + 4;

            float f = c2 * (
                    sq2 * w.at(x0, y0)
                    + sq2 * w.at(x0, y1)
                    + sq2 * w.at(x1, y1)
                    + sq2 * w.at(x1, y0)
                    + w.at(x0, y)
                    + w.at(x1, y)
                    + w.at(x, y0)
                    + w.at(x, y1)
                    - s * w.at(x, y)
                    ) / s;
            w.vel_at(x, y) += f * dt;
            w.vel_at(x, y) *= 0.99;
            w.other_at(x, y) = w.at(x, y) + w.vel_at(x, y) * dt;
        }

    std::swap(w.heightmap, w.other);
}

void update_texture(float dt) {
    // int w = state.tex.buffer.w;
    // int h = state.tex.buffer.h;
    // unsigned char* data = state.tex.buffer.data;
    // int shift = dt*100;
    // printf("shift: %d\n", shift);
    // for (int y = 0; y < h; ++y)
    //     for (int x = 0; x < w; ++x) {
    //         data[y*w*3+x*3+0] = (data[y*w*3+x*3+0] + shift*2) % 255;
    //         data[y*w*3+x*3+1] = (data[y*w*3+x*3+1] - shift) % 255;
    //         data[y*w*3+x*3+2] = (data[y*w*3+x*3+2] + shift) % 255;
    //     }
}

bool in_buffer(int x, int y) {
    return x >= 0 and y >= 0 and x < state.tex.image.w and y < state.tex.image.h;
}

void draw_line(Vec2 a, Vec2 b, RGBA color, float thickness) {
    Image* buf = &state.tex.image;
    float t = thickness/2;
    int x0 = std::floor(std::min(a.x, b.x) - t + 0.5);
    int y0 = std::floor(std::min(a.y, b.y) - t + 0.5);
    int x1 = std::ceil(std::max(a.x, b.x) + t - 0.5);
    int y1 = std::ceil(std::max(a.y, b.y) + t - 0.5);
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

            RGBA col = value * color;
            buf->at(x, y) = clamp01(col + (1-col.a)*buf->at(x, y));
        }
}

void draw_point(Vec2 pos, RGBA color, float radius) {
    Image* buf = &state.tex.image;
    float t = radius/2;
    int x0 = std::floor(pos.x - t + 0.5);
    int y0 = std::floor(pos.y - t + 0.5);
    int x1 = std::ceil(pos.x + t - 0.5);
    int y1 = std::ceil(pos.y + t - 0.5);
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
                    if (distance < t) {
                        value += factor;
                    }
                }

            RGBA c = value * color;
            RGBA output = c + (1-c.a) * buf->at(x, y);
            buf->at(x, y) = clamp01(output);
        }
}

void draw_texture() {
    auto& tex = state.tex;
    for (int y = 0; y < tex.buffer.h; ++y)
        for (int x = 0; x < tex.buffer.w; ++x) {
            tex.buffer.data[y*tex.buffer.w*3 + x*3 + 0] = tex.image.data[y*tex.image.w + x].r * 255;
            tex.buffer.data[y*tex.buffer.w*3 + x*3 + 1] = tex.image.data[y*tex.image.w + x].g * 255;
            tex.buffer.data[y*tex.buffer.w*3 + x*3 + 2] = tex.image.data[y*tex.image.w + x].b * 255;
        }

    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        tex.buffer.w,
        tex.buffer.h,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        tex.buffer.data
    );

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2d(0, 1); glVertex2d(-1, -1);
    glTexCoord2d(1, 1); glVertex2d(1, -1);
    glTexCoord2d(1, 0); glVertex2d(1, 1);
    glTexCoord2d(0, 0); glVertex2d(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void draw_starfield() {
    auto sf = state.stars;
    for (int i = 0; i < sf.n_stars; ++i) {
        float b = sf.brightness[i];
        RGBA c = RGBA{b, b, b, b};
        float size = 1 * sf.size[i];
        draw_point(sf.position[i], c, size);
    }
}

void draw_water() {
    Image* im = &state.tex.image;
    Water* w = &state.water;
    for (int y = 0; y < im->h; ++y)
        for (int x = 0; x < im->w; ++x) {
            // printf("water %d, %d: %f\n", x, y, w->at(x, y));
            im->at(x, y) = clamp01(0.5 * w->at(x, y) * RGBA{0, 0, 1, 1});
        }
}

void draw_gradient() {
    Image* im = &state.tex.image;
    RGBA tl{1, 0, 0, 1};
    RGBA tr{0, 1, 0, 1};
    RGBA br{0, 0, 1, 1};
    RGBA bl{1, 0, 1, 1};

    for (int y = 0; y < im->h; ++y)
        for (int x = 0; x < im->w; ++x) {
            float fx = x/(float)im->w;
            float fy = y/(float)im->h;
            im->at(x, y) = (1 - fx) * ((1-fy) * tl + fy * bl) + fx * ((1-fy) * tr + fy * br);
        }
}

void draw() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    init_solid();

    Camera c = state.camera;
    
    draw_point(c.to_viewport(state.player.pos), {0, 0, 1, 1}, c.to_viewport(state.player.size));

    for (const auto& door : state.doors) {
        draw_line(c.to_viewport(door.a), c.to_viewport(door.b), {0.3,0.3,0.3,1}, state.camera.to_viewport(0.1));
    }

    for (int i = 0; i < (int)state.walls.size(); ++i) {
        Wall wall = state.walls[i];
        if (is_door(i)) {
            draw_line(c.to_viewport(wall.a), c.to_viewport(wall.b), {0.8,0.8,0.8,1}, c.to_viewport(0.5));
        } else {
            draw_line(c.to_viewport(wall.a), c.to_viewport(wall.b), {0.5,0.5,0.5,1}, c.to_viewport(1));
        }
    }

    draw_point(c.to_viewport(c.from_viewport(state.controls.mouse)), {1, 0, 0, 1}, 10);

    // draw_gradient();
    // draw_water();
    draw_texture();
}

struct DistanceResult {
    float distance;
    bool within_segment;
};

DistanceResult distance_point_line(Vec2 p, Vec2 a, Vec2 b) {
    // print_vec("a", a);
    // print_vec("b", b);
    // print_vec("p", p);
    Vec2 AB = normalize(b - a);
    Vec2 AP = p - a;
    Vec2 BP = p - b;
    // print_vec("AB", AB);
    // print_vec("AP", AP);
    // print_vec("BP", BP);

    DistanceResult res;
    res.within_segment = false;

    Vec2 perp = Vec2{AB.y, -AB.x};
    res.distance = dot(AP, perp);

    if (dot(AB, AP) < 0) {
        // res.distance = norm(AP);
        // printf("dot(AB, AP) < 0\n");
        // printf("distance: %f\n", res.distance);
        return res;
    }

    if (dot(-AB, BP) < 0) {
        // res.distance = norm(BP);
        // printf("dot(-AB, BP) < 0\n");
        // printf("distance: %f\n", res.distance);
        return res;
    }

    res.within_segment = true;

    // printf("distance: %f\n", res.distance);
    return res;
}

bool crossed_line(Vec2 pos_prev, Vec2 pos_next, Vec2 line_a, Vec2 line_b) {
    Vec2 BA = normalize(line_b - line_a);
    Vec2 PA = normalize(pos_next - line_a);
    Vec2 PB = normalize(pos_next - line_b);

    if (dot(BA, PA) < 0 || dot(-BA, PB) < 0) {
        return false;
    }

    Vec2 perp = Vec2{BA.y, -BA.x};
    float dist_prev = dot((pos_prev - line_a), perp);
    float dist_next = dot(PA, perp);
    if ((dist_prev > 0 && dist_next <= 0) || 
            (dist_prev < 0 && dist_next >= 0)) {
        return true;
    }

    return false;
}

bool crossed_line2(Vec2 pos_prev, Vec2 pos_next, float radius, Vec2 a, Vec2 b) {
    DistanceResult d_prev = distance_point_line(pos_prev, a, b);
    DistanceResult d_next = distance_point_line(pos_next, a, b);

    if (!d_prev.within_segment and !d_next.within_segment) {
        printf("false1\n");
        return false;
    }

    float d0 = d_prev.distance;
    float d1 = d_next.distance;
    printf("prev_d: %f, next_d: %f\n", d0, d1);
    // if ((d0 > radius and d1 > radius)
    //         or (d0 < -radius and d1 < -radius))
    //     return false;
    if (d0 > 0 and d1 <= 0) {
        printf("true1\n");
        return true;
    }
    if (d0 <= 0 and d1 > 0) {
        printf("true2\n");
        return true;
    }
    if (std::abs(d0) >= radius and std::abs(d1) < radius) {
        printf("true3\n");
        return true;
    }

    printf("false2\n");
    return false;
}

void update_player_with_gravity(float dt) {
    Player player = state.player;
    if (state.controls.use_js) {
        player.vel.x = state.controls.js.x;
        player.vel.y = state.controls.js.y;
    } else {
        float player_speed = 30;
        if (state.controls.left) {
            player.pos.x -= player_speed * dt;
        }
        if (state.controls.right) {
            player.pos.x += player_speed * dt;
        }
        if (state.controls.up) {
            player.pos.y -= player_speed * dt;
        }
        if (state.controls.down) {
            player.pos.y += player_speed * dt;
        }
    }
}

bool player_in_room(Player p, Room r) {
    return p.pos.x >= r.bbox[0].x
        and p.pos.y >= r.bbox[0].y
        and p.pos.x <= r.bbox[1].x
        and p.pos.y <= r.bbox[1].y;
}

void update_camera_position(Player player, bool instant=false) {
    for (Room room : state.rooms) {
        if (player_in_room(player, room)) {
            float margin = 3;
            float target_size_x = room.bbox[1].x - room.bbox[0].x + 2*margin;
            float target_size_y = room.bbox[1].y - room.bbox[0].y + 2*margin;
            Vec2 new_pos = room.bbox[0] - margin;
            if (instant) {
                state.camera.position = new_pos;
                state.camera.size_x = target_size_x;
                state.camera.size_y = target_size_y;
            } else {
                state.camera.set_target_view(new_pos, target_size_x, target_size_y);
            }
            return;
        }
    }
}

void update(float dt) {
    Player player = state.player;

    if (player.state == Player::State::PLAYER_ON_WALL) {
        // float player_acc = 20;
        // if (state.controls.left) {
        //     player.vel.x -= player_acc * dt;
        // }
        // if (state.controls.right) {
        //     player.vel.x += player_acc * dt;
        // }
        // if (state.controls.up) {
        //     player.vel.y -= player_acc * dt;
        // }
        // if (state.controls.down) {
        //     player.vel.y += player_acc * dt;
        // }

        Vec2 push;
        if (state.controls.left) {
            push = {-1, 0}; } else if (state.controls.right) {
            push = {1, 0};
        } else if (state.controls.up) {
            push = {0, -1};
        } else if (state.controls.down) {
            push = {0, 1};
        }

        if (push.x != 0 or push.y != 0) {
            bool pushed_torwards = false;
            bool pushed_away = false;
            for (auto w : state.walls) {
                if (crossed_line2(state.player.pos, player.pos + push, player.size, w.a, w.b)) {
                    pushed_torwards = true;
                    break;
                }
                if (crossed_line2(state.player.pos, player.pos - push, player.size, w.a, w.b)) {
                    pushed_away = true;
                    break;
                }
            }

            if (pushed_torwards) {
                player.vel = -push * 10;
                player.state = Player::State::PLAYER_FLYING;
                state.controls.left = false;
                state.controls.right = false;
                state.controls.up = false;
                state.controls.down = false;
            } else if (!pushed_away) {
                player.vel = push * 5;
            }
        } else {
            player.vel = {};
        }
    }

    player.pos = player.pos + player.vel * dt;

    if (state.controls.open_door.get()) {
        for (int i = 0; i < (int)state.doors.size(); ++i) {
            Door* d = &state.doors[i];
            float dist = distance_point_line(state.player.pos, d->a, d->b).distance;
            if (std::abs(dist) <= state.player.size + 0.5) {
                open_or_close_door(d);
            }
        }
    }

    for (auto w : state.walls) {
        if (crossed_line2(state.player.pos, player.pos, player.size, w.a, w.b)) {
            player.pos = state.player.pos;
            player.vel = {0, 0};
            player.state = Player::State::PLAYER_ON_WALL;
            break;
        }
    }

    // distance_point_line(state.camera.from_viewport(state.controls.mouse), state.walls[0].a, state.walls[0].b);
    // Vec2 m = state.camera.from_viewport(state.controls.mouse);
    // crossed_line2(m, m, 1, state.walls[0].a, state.walls[0].b);

    for (auto door : state.doors) {
        if (crossed_line2(state.player.pos, player.pos, player.size, door.a, door.b)) {
            // if (state.current_room == door.room0) {
            //     state.current_room = door.room1;
            // } else {
            //     state.current_room = door.room0;
            // }

            printf("Changed room\n");
            update_camera_position(player);
            break;
        }
    }

    state.player = player;

    for (int i = 0; i < (int)state.animations.size(); ++i) {
        Animation* a = state.animations.get(i);
        a->update(dt);
        printf("animation %d value: %f \n", i, a->value);
        if (!state.animations.active(i)) {
            state.animations.remove(i);
        }
    }

    for (auto &d : state.doors) {
        if (d.animation_id >= 0) {
            if (!state.animations.active(d.animation_id)) {
                d.animation_id = -1;
                continue;
            }

            Animation* a = state.animations.get(d.animation_id);
            set_door_openness(d, a->value);
        }
    }

    state.camera.update(dt);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE: state.running = false; break;
            case GLFW_KEY_LEFT: state.controls.left = true; break;
            case GLFW_KEY_RIGHT: state.controls.right = true; break;
            case GLFW_KEY_UP: state.controls.up = true; break;
            case GLFW_KEY_DOWN: state.controls.down = true; break;
            case GLFW_KEY_SPACE: state.controls.open_door.set(); break;
        }

    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_LEFT: state.controls.left = false; break;
            case GLFW_KEY_RIGHT: state.controls.right = false; break;
            case GLFW_KEY_UP: state.controls.up = false; break;
            case GLFW_KEY_DOWN: state.controls.down = false; break;
        }
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    state.controls.mouse.x = xpos;
    state.controls.mouse.y = ypos;
}


float thresholded(float x, float t) {
    if (x < t && x > -t) return 0;
    return x;
}

void joystick_control(int jid) {
    GLFWgamepadstate gp;
    if (glfwGetGamepadState(jid, &gp)) {
        float x = gp.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        float y = gp.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        printf("x: %.3f, y: %.3f\n", x, y);

        state.controls.js.x = thresholded(x, 0.1) * 30;
        state.controls.js.y = thresholded(y, 0.1) * 30;
    }
}


int main(int argc, char** argv) {
    std::srand(std::time(0));

    if (!glfwInit()) {
        std::cerr << "Cannot initialize GLFW\n";
        return 1;
    }

    const int window_width = 1024;
    const int window_height = 1024;
    GLFWwindow * window = glfwCreateWindow(window_width, window_height, "Awesome game", NULL, NULL);
    if (!window) {
        std::cerr << "Cannot open window\n";
        return 1;
    }

    glfwMakeContextCurrent(window);

    state = {
        .walls = {
            Wall{Vec2{10, 10}, Vec2{15, 10}},
            Wall{Vec2{20, 10}, Vec2{30, 10}},
            Wall{Vec2{30, 10}, Vec2{30, 15}},
            Wall{Vec2{30, 20}, Vec2{30, 30}},
            Wall{Vec2{30, 30}, Vec2{10, 30}},
            Wall{Vec2{10, 30}, Vec2{10, 10}},

            Wall{Vec2{30, 10}, Vec2{90, 10}},
            Wall{Vec2{90, 10}, Vec2{90, 30}},
            Wall{Vec2{90, 30}, Vec2{30, 30}},
            Wall{Vec2{30, 30}, Vec2{30, 20}},
            Wall{Vec2{30, 15}, Vec2{30, 10}},
        },
        .rooms = {
            Room{.bbox = {Vec2{10, 10}, Vec2{30, 30}}},
            Room{.bbox = {Vec2{30, 10}, Vec2{90, 30}}},
        },
        .player = Player{
            .pos = Vec2{15, 18},
            .vel = Vec2{-10, 0},
            .size = 2
        },
        .camera = {
            .position = Vec2{0, 0},
            .size_x = 30,
            .size_y = 30,
            .res_x = 1024,
            .res_y = 1024
        }
    };

    add_door(Door{.a = Vec2{30,20}, .b = Vec2{30, 15}, .room0 = 0, .room1 = 1, .closed = false});

    init_texture();
    // init_water();
    generate_star_field(1024, 1024);
    update_camera_position(state.player, true);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);  

    Timer timer_dt;
    timer_dt.start();
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // glViewport(0, 0, gwa.width, gwa.height);
    
    state.running = true;
    while (state.running) {
        if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
            state.controls.use_js = true;
            joystick_control(GLFW_JOYSTICK_1);
        }

        float dt = timer_dt.tick();
        update(dt);

        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}


// Ideas:
// - button to open door (can be away from the door)
// - graphics look like blueprints (but appropriate for game)
// - use mouse or joystick to aim trust vector
// - stars and galaxies in background
// - can exit the spaceship/spacestation
//
// Implemented:
// - can grab handles on the walls to move alongside

