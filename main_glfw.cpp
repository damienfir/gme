#include <GLFW/glfw3.h>
// #include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <bitset>
#include <thread>
#include <vector>

#include "math.hpp"
#include "timer.hpp"
#include "image.hpp"


void print_vec(const char* name, Vec2 v) {
    printf("%s: %.2f, %.2f\n", name, v.x, v.y);
}

void print_affine(const char* name, Affine m) {
    printf("%s:\n", name);
    printf("%.2f %.2f %.2f\n", m.m.m00, m.m.m01, m.t.x);
    printf("%.2f %.2f %.2f\n", m.m.m10, m.m.m11, m.t.y);
}

void print_rgba(const char* name, RGBA c) {
    printf("%s: %.2f, %.2f, %.2f, %.2f\n", name, c.r, c.g, c.b, c.a);
}

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

    Vec2 get_size() {
        return Vec2{(float)size_x, (float)size_y};
    }

    Vec2 to_viewport(Vec2 v) {
        return Vec2{
            .x = (v.x - position.x) * res_x / size_x,
            .y = (v.y - position.y) * res_y / size_y
        };
    }

    float to_viewport(float a) {
        return a * res_x / size_x;
    }

    Affine view_transform() {
        Affine translation = from_translation(-position);
        Affine scale = from_scale(res_x / size_x);
        return mul(scale, translation);
    }

    void set_target_position(Vec2 new_position, float duration) {
        pos_animation_x.start(position.x, new_position.x, duration);
        pos_animation_y.start(position.y, new_position.y, duration);
    }

    void set_target_size(float new_size_x, float new_size_y, float duration) {
        float aspect_ratio = (float)res_x/(float)res_y;
        if (new_size_x / new_size_y < aspect_ratio) {
            new_size_x = new_size_y / aspect_ratio;
        } else {
            new_size_y = new_size_x / aspect_ratio;
        }
        size_animation_x.start(size_x, new_size_x, duration);
        size_animation_y.start(size_y, new_size_y, duration);
    }

    void set_target_position_and_view(Vec2 new_position, float new_size_x, float new_size_y) {
        set_target_position(new_position, 0.5);
        set_target_size(new_size_x, new_size_y, 0.5);
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
    Vec2 a;
    Vec2 b;
    int room0;
    int room1;
    bool closed;
};

struct DoorAnimation {
    Animation anim;
    int door_id;
};

struct Room {
    Vec2 bbox[2];

    Vec2 center() {
        return (bbox[1] - bbox[0]) / 2.f;
    }
};

struct ActionTrigger {
    bool active = false;

    bool get_and_deactivate() {
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
    ActionTrigger action;
    ActionTrigger push;
    bool use_js;
    Vec2 js;
    Vec2 mouse;
};

struct Sprite {
    int w;
    int h;
    RGBA* data;

    RGBA at(int x, int y) {
        return data[y * w + x];
    }
};

struct BoundingBox {
    Vec2 tl;
    Vec2 br;
};

struct Player {
    Vec2 pos;
    Vec2 vel;
    float scale;
    Sprite sprite;
    float rotation;
    int near_button_index;
    bool is_in_space;

    Vec2 get_offset() {
        return Vec2 {
            .x = scale * (sprite.w/2.f),
            .y = scale * (sprite.h/2.f)
        };
    }

    float collision_radius() {
        return norm(scale * Vec2{(float)sprite.w, (float)sprite.h} / 2.f);
    }

    Affine model_transform() {
        Vec2 offset = get_offset();
        Affine m = from_scale(scale);
        m = mul(from_translation(-offset), m);
        m = mul(from_rotation(rotation), m);
        m = mul(from_translation(offset), m);
        return mul(from_translation(pos - offset), m);
    }
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

    void add_onto(int x, int y, RGBA c) {
        data[y*w+x] = clamp01(c + (1-c.a)*data[y*w+x]);
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

struct Button {
    Vec2 pos;
    int door_id;
};

struct Animations {
    std::vector<DoorAnimation> door;
};

struct GameState {
    std::vector<Wall> walls;
    std::vector<Room> rooms;
    std::vector<Door> doors;
    std::vector<Button> buttons;
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

void open_or_close_door(int id) {
    if (!state.animations.door.empty()) return;

    DoorAnimation da = {.door_id = id};
    Door* d = &state.doors[id];
    if (d->closed) {
        da.anim.start(0, 1, 1);
        d->closed = false;
    } else {
        da.anim.start(1, 0, 1);
        d->closed = true;
    }
    state.animations.door.push_back(da);
}

void set_door_openness(int id, float openness) {
    Door d = state.doors[id];
    Wall* w = &state.walls[d.door_id];
    Vec2 slide_vector = openness * (d.a - d.b);
    w->a = d.a + slide_vector;
    w->b = d.b + slide_vector;
}

int add_door(Door d) {
    int id = state.walls.size();
    d.door_id = id;
    Wall w = {.a = d.a, .b = d.b};
    state.walls.push_back(w);
    int door_id = state.doors.size();
    state.doors.push_back(d);
    set_door_openness(door_id, d.closed ? 0 : 1);
    return door_id;
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

bool in_buffer(int x, int y) {
    return x >= 0 and y >= 0 and x < state.tex.image.w and y < state.tex.image.h;
}

void draw_line(Vec2 a, Vec2 b, RGBA color, float thickness) {
    Image* buf = &state.tex.image;
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

            buf->add_onto(x, y, value * color);
        }
}

void draw_point(Vec2 pos, RGBA color, float radius) {
    Image* buf = &state.tex.image;
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

            buf->add_onto(x, y, value * color);
        }
}

void draw_rectangle(Vec2 tl, Vec2 br, RGBA color) {
    Image* buf = &state.tex.image;
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

            buf->add_onto(x, y, value * color);
        }
}

RGBA lookup_nearest(Sprite s, Vec2 p) {
    int x = p.x;
    int y = p.y;
    if (x < 0 or x >= s.w or y < 0 or y >= s.h) {
        return {};
    }
    return s.data[y*s.w + x];
}

void draw_sprite(Sprite s, Affine t) {
    Image* buf = &state.tex.image;

    Vec2 tl = mul(t, Vec2{0, 0});
    Vec2 tr = mul(t, Vec2{s.w-1.f, 0});
    Vec2 br = mul(t, Vec2{s.w-1.f, s.h-1.f});
    Vec2 bl = mul(t, Vec2{0, s.h-1.f});
    int y0 = std::max(0.f, std::min(tl.y, std::min(tr.y, std::min(br.y, bl.y))));
    int x0 = std::max(0.f, std::min(tl.x, std::min(tr.x, std::min(br.x, bl.x))));
    int y1 = std::min(buf->h-1.f, std::ceil(std::max(tl.y, std::max(tr.y, std::max(br.y, bl.y)))));
    int x1 = std::min(buf->w-1.f, std::ceil(std::max(tl.x, std::max(tr.x, std::max(br.x, bl.x)))));

    Affine ti = inverse(t);
    for (int tex_y = y0; tex_y <= y1; ++tex_y)
        for (int tex_x = x0; tex_x <= x1; ++tex_x) {
            Vec2 sprite_xy = mul(ti, Vec2{(float)tex_x, (float)tex_y});
            buf->add_onto(tex_x, tex_y, lookup_nearest(s, sprite_xy));
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
    init_solid({0.1, 0.1, 0.1, 1});

    Camera c = state.camera;
    
    Affine t = mul(c.view_transform(), state.player.model_transform());
    draw_sprite(state.player.sprite, t);

    for (const auto& door : state.doors) {
        draw_line(c.to_viewport(door.a), c.to_viewport(door.b), {0.3,0.3,0.3,1}, state.camera.to_viewport(0.1));
    }

    for (int i = 0; i < (int)state.walls.size(); ++i) {
        Wall wall = state.walls[i];
        if (is_door(i)) {
            draw_line(c.to_viewport(wall.a), c.to_viewport(wall.b), {0.8,0.8,0.8,1}, c.to_viewport(0.5));
        } else {
            draw_line(c.to_viewport(wall.a), c.to_viewport(wall.b), {0.5,0.5,0.5,1}, c.to_viewport(0.5));
        }
    }

    for (int button_index = 0; button_index < (int)state.buttons.size(); ++button_index) {
        Vec2 p = state.buttons[button_index].pos;
        RGBA color = {0, 0.5, 0, 1};
        if (state.player.near_button_index == button_index) {
            color = RGBA{0, 1, 0, 1};
        }
        draw_rectangle(c.to_viewport(Vec2{p.x-0.5f, p.y-0.5f}), c.to_viewport(Vec2{p.x+0.5f,p.y+0.5f}), color);
    }

    // draw_gradient();
    // draw_water();
    draw_texture();
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

void update_camera_position(Player* player, bool instant=false) {
    player->is_in_space = false;

    for (Room room : state.rooms) {
        if (player_in_room(*player, room)) {
            float margin = 3;
            float target_size_x = room.bbox[1].x - room.bbox[0].x + 2*margin;
            float target_size_y = room.bbox[1].y - room.bbox[0].y + 2*margin;
            Vec2 new_pos = room.bbox[0] - margin;
            if (instant) {
                state.camera.position = new_pos;
                state.camera.size_x = target_size_x;
                state.camera.size_y = target_size_y;
            } else {
                state.camera.set_target_position_and_view(new_pos, target_size_x, target_size_y);
            }
            return;
        }
    }

    player->is_in_space = true;
    player->vel = {};
}

Vec2 get_wall_normal_towards(Wall w, Vec2 p) {
    Vec2 AB = normalize(w.b - w.a);
    Vec2 normal = Vec2{AB.y, -AB.x};
    if (dot(normal, p-w.a) < 0) {
        return -normal;
    }
    return normal;
}

void update(float dt) {
    Player player = state.player;

    if (player.is_in_space) {

        float player_acc = 5;
        Vec2 thrust = {};
        if (state.controls.left) {
            thrust.x = -1;
        }
        if (state.controls.right) {
            thrust.x = 1;
        }
        if (state.controls.up) {
            thrust.y = -1;
        }
        if (state.controls.down) {
            thrust.y = 1;
        }

        if (!is_zero(thrust)) {
            player.rotation = std::atan2(thrust.y, thrust.x);
            thrust = normalize(thrust) * player_acc;
        }

        player.vel = player.vel + thrust * dt;

    } else {

        const float speed = 20;
        player.vel = {};

        if (state.controls.left) {
            player.vel.x = -1;
        }

        if (state.controls.right) {
            player.vel.x = 1;
        }

        if (state.controls.up) {
            player.vel.y = -1;
        }

        if (state.controls.down) {
            player.vel.y = 1;
        }

        if (!is_zero(player.vel)) {
            player.vel = normalize(player.vel) * speed;
            player.rotation = std::atan2(player.vel.y, player.vel.x);
        }

    }

    player.pos = player.pos + player.vel * dt;


    if (state.controls.action.get_and_deactivate()) {
        if (player.near_button_index >= 0) {
            open_or_close_door(state.buttons[player.near_button_index].door_id);
        }
    }

    player.near_button_index = -1;
    for (int button_index = 0; button_index < (int)state.buttons.size(); ++button_index) {
        Button b = state.buttons[button_index];
        float dist = norm(player.pos - b.pos);
        if (dist < player.collision_radius() + 0.5) {
            player.near_button_index = button_index;
            break;
        }
    }

    for (int wall_index = 0; wall_index < (int)state.walls.size(); ++wall_index) {
        Wall w = state.walls[wall_index];
        if (crossed_line2(state.player.pos, player.pos, player.collision_radius(), w.a, w.b)) {
            player.pos = state.player.pos;
            player.vel = {0, 0};
            break;
        }
    }

    for (auto door : state.doors) {
        if (crossed_line2(state.player.pos, player.pos, player.collision_radius(), door.a, door.b)) {
            printf("Changed room\n");
            update_camera_position(&player);
            break;
        }
    }

    state.player = player;

    if (state.player.is_in_space) {
        state.camera.set_target_position(state.player.pos - state.camera.get_size() / 2.f, 0.1);
    }

    if (!state.animations.door.empty()) {
        DoorAnimation* da = &state.animations.door[0];
        da->anim.update(dt);
        set_door_openness(da->door_id, da->anim.value);

        if (!da->anim.animating) {
            state.animations.door.clear();
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
            case GLFW_KEY_SPACE: state.controls.action.set(); break;
            case GLFW_KEY_ENTER: state.controls.push.set(); break;
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

struct Crop {
    int x;
    int y;
    int w;
    int h;
};

Sprite sprite_from_png(PngImage im, Crop roi) {
    Sprite s;
    s.w = roi.w;
    s.h = roi.h;
    s.data = (RGBA*)malloc(s.w*s.h*sizeof(RGBA));

    for (int crop_y = roi.y; crop_y < roi.y + roi.w; ++crop_y) 
        for (int crop_x = roi.x; crop_x < roi.x + roi.w; ++crop_x) {
            RGBA c;
            c.r = im.data[crop_y * im.width * 4 + crop_x*4 + 0];
            c.g = im.data[crop_y * im.width * 4 + crop_x*4 + 1];
            c.b = im.data[crop_y * im.width * 4 + crop_x*4 + 2];
            c.a = im.data[crop_y * im.width * 4 + crop_x*4 + 3];
            int sprite_x = crop_x - roi.x;
            int sprite_y = crop_y - roi.y;
            s.data[sprite_y * roi.w + sprite_x] = c;
        }

    return s;
}

Sprite load_player_sprite() {
    return sprite_from_png(read_png("resources/sprites.png"), Crop{.x = 0, .y = 0, .w = 16, .h = 16});
}


int main(int argc, char** argv) {
    std::srand(std::time(0));

    if (!glfwInit()) {
        printf("Cannot initialize GLFW\n");
        abort();
    }

    const int window_width = 1024;
    const int window_height = 1024;
    GLFWwindow * window = glfwCreateWindow(window_width, window_height, "Awesome game", NULL, NULL);
    if (!window) {
        printf("Cannot open window\n");
        abort();
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
            .scale = 0.2,
            .sprite = load_player_sprite(),
        },
        .camera = {
            .position = Vec2{20, 10},
            .size_x = 50,
            .size_y = 50,
            .res_x = 1024,
            .res_y = 1024
        },
    };

    int door_id = add_door(Door{.a = Vec2{30,20}, .b = Vec2{30, 15}, .closed = false});
    state.buttons.push_back(Button{.pos = Vec2{11, 18}, .door_id = door_id });

    int sas_id = add_door(Door{.a = Vec2{15,10}, .b = Vec2{20, 10}, .closed = false});
    state.buttons.push_back(Button{.pos = Vec2{23, 11}, .door_id = sas_id });

    init_texture();
    // init_water();
    generate_star_field(1024, 1024);
    update_camera_position(&state.player, true);

    Timer timer_dt;
    timer_dt.start();
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    
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
// - graphics look like blueprints (but appropriate for game)
// - use mouse or joystick to aim trust vector
// - stars and galaxies in background
// - button connects to a door via a line on the flow
// - button to deactivate magnetism (the player floats and moves via handles on floor and walls, bumps on walls like a real collision)
//
// Development:
// - design a nice spaceship
//
// Implemented:
// - can grab handles on the walls to move alongside
// - button to open door (can be away from the door)
// - import sprites
// - can exit the spaceship/spacestation (how to move? maybe tether or truster)
