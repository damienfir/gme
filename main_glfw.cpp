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


struct Rect {
    float x;
    float y;
    float w;
    float h;
};

struct Wall {
    Vec2 a;
    Vec2 b;
};

struct Animation {
    bool animating;
    float value_start;
    float value_end;
    float value;
    float animation_time;
    float animation_duration;

    void start(float begin, float end) {
        value_start = begin;
        value_end = end;
        animation_time = 0;
        animation_duration = 1;
        animating = true;
    }

    void update(float dt) {
        if (animating) {
            animation_time += dt;
            float x = animation_time / animation_duration;
            value = value_start * (1-x) + value_end * x;

            if (x >= 1) {
                value = value_end;
                animating = false;
            }
        }
    }
};

struct Door {
    Vec2 hinge;
    float length;
    float angle;
    int room0;
    int room1;
    
    Animation animation;

    Vec2 end() {
        return hinge + length * rotate(Vec2{1, 0}, angle);
    }
};

struct Sas {
    Vec2 a;
    Vec2 b;
};

struct Room {
    std::vector<Wall> walls;
};

struct Controls {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool use_js;
    Vec2 js;
};

struct Player {
    Vec2 pos;
    Vec2 vel;
    float size;
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
    std::vector<Vec2> position;
    std::vector<float> size;
    std::vector<float> brightness;
};

struct GameState {
    std::vector<Room> rooms;
    int current_room;
    std::vector<Door> doors;
    Sas sas;
    Player player;
    Controls controls;
    bool in_space;
    bool running;
    Texture tex;
    StarField stars;
};

const int coord_width = 100;
const int coord_height = 100;
GameState state;


StarField generate_star_field() {
    StarField f;

    f.position = {
        Vec2{10, 10},
        Vec2{20, 30},
        Vec2{55, 23},
    };

    f.size = {
        1, 3, 2,
    };

    f.brightness = {0.3, 0.7, 0.5};

    return f;
}


void init_gradient(Image* buf) {
    RGBA tl{1, 0, 0, 1};
    RGBA tr{0, 1, 0, 1};
    RGBA br{0, 0, 1, 1};
    RGBA bl{1, 0, 1, 1};

    for (int y = 0; y < buf->h; ++y)
        for (int x = 0; x < buf->w; ++x) {
            float fx = x/(float)buf->w;
            float fy = y/(float)buf->h;
            buf->at(x, y) = (1 - fx) * ((1-fy) * tl + fy * bl) + fx * ((1-fy) * tr + fy * br);
        }
}

Texture make_texture() {
    Texture tex;
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int w = 256;
    int h = 256;
    tex.buffer = {
        .w = w,
        .h = h,
        .data = (unsigned char*)malloc(w*h*3*sizeof(unsigned char)),
    };

    tex.image = {
        .w = w,
        .h = h,
        .data = (RGBA*)malloc(w*h*sizeof(RGBA)),
    };


    // for (int i = 0; i < h*w; ++i) tex.image.data[i] = {};

    return tex;
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

bool in_buffer(Image *buf, int x, int y) {
    return x >= 0 and y >= 0 and x < buf->w and y < buf->h;
}

void draw_line(Image* buf, Vec2 a, Vec2 b, RGBA color, float thickness) {
    float t = thickness/2;
    int x0 = std::floor(std::min(a.x, b.x) - t + 0.5);
    int y0 = std::floor(std::min(a.y, b.y) - t + 0.5);
    int x1 = std::ceil(std::max(a.x, b.x) + t - 0.5);
    int y1 = std::ceil(std::max(a.y, b.y) + t - 0.5);
    Vec2 ba = normalize(b - a);
    Vec2 normal = Vec2{-ba.y, ba.x};
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(buf, x, y)) continue;

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

void draw_point(Image* buf, Vec2 pos, RGBA color, float radius) {
    float t = radius/2;
    int x0 = std::floor(pos.x - t + 0.5);
    int y0 = std::floor(pos.y - t + 0.5);
    int x1 = std::ceil(pos.x + t - 0.5);
    int y1 = std::ceil(pos.y + t - 0.5);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!in_buffer(buf, x, y)) continue;

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
            printf("alpha: %f\n", c.a);
            RGBA output = c + (1-c.a) * buf->at(x, y);
            buf->at(x, y) = clamp01(output);
        }
}

void draw_texture(Texture tex) {
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

void init_solid(Image *im, RGBA c = {}) {
    for (int i = 0; i < im->w * im->h; ++i) im->data[i] = c;
}

void draw_starfield(Image *buf, StarField sf) {
    for (size_t i = 0; i < sf.position.size(); ++i) {
        float b = sf.brightness[i];
        RGBA c = RGBA{b, b, b, b};
        float size = 1 * sf.size[i];
        draw_point(buf, sf.position[i], c, size);
    }
}

void draw(GameState state) {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // init_solid(&state.tex.image, RGBA{1,0,0,1});

    init_gradient(&state.tex.image);

    // for (Room room : state.rooms) {
    //     for (Wall w : room.walls) {
    //         draw_line(&state.tex.image, w.a, w.b, RGBA{1, 1, 1, 1}, 1);
    //     }
    // }


    // for (Door door : state.doors) {
    //     draw_door(door);
    // }

    // draw_point(&state.tex.image, state.player.pos, RGBA{0, 0, 1, 1}, state.player.size);

    draw_line(&state.tex.image, Vec2{10, 10}, Vec2{30, 27}, RGBA{0.8, 0.0, 0.8, 0.8}, 2);
    draw_starfield(&state.tex.image, state.stars);
    
    draw_texture(state.tex);
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

void update(GameState *state, float dt) {
    Player player = state->player;

    if (!state->in_space) {
        if (state->controls.use_js) {
            player.vel.x = state->controls.js.x;
            player.vel.y = state->controls.js.y;
        } else {
            float player_speed = 30;
            if (state->controls.left) {
                player.pos.x -= player_speed * dt;
            }
            if (state->controls.right) {
                player.pos.x += player_speed * dt;
            }
            if (state->controls.up) {
                player.pos.y -= player_speed * dt;
            }
            if (state->controls.down) {
                player.pos.y += player_speed * dt;
            }
        }

    } else {
        if (state->controls.use_js) {
            player.vel.x += state->controls.js.x * 0.03;
            player.vel.y += state->controls.js.y * 0.03;
        } else {
            float player_acc = 20;
            if (state->controls.left) {
                player.vel.x -= player_acc * dt;
            }
            if (state->controls.right) {
                player.vel.x += player_acc * dt;
            }
            if (state->controls.up) {
                player.vel.y -= player_acc * dt;
            }
            if (state->controls.down) {
                player.vel.y += player_acc * dt;
            }
        }
    }
    player.pos = player.pos + player.vel * dt;


    for (auto & room : state->rooms) {
        for (int i = 0; i < (int)room.walls.size(); ++i) {
            Wall w = room.walls[i];
            if (crossed_line(state->player.pos, player.pos, w.a, w.b)) {
                player.pos = state->player.pos;
                break;
            }
        }
    }

    // for (Door door : state->doors) {

    //     if (crossed_line(state->player.pos, player.pos, door.hinge, door.end())) {
    //         if (state->current_room == door.room0) {
    //             state->current_room = door.room1;
    //         } else {
    //             state->current_room = door.room0;
    //         }
    //     }
    // }

    // if (crossed_line(state->player.pos, player.pos, state->sas.a, state->sas.b)) {
    //     state->in_space = !state->in_space;
    //     if (!state->in_space) {
    //         player.vel = Vec2{0, 0};
    //     }
    // }

    // printf("current room: %d\n", state->current_room);
    // printf("in space: %d\n", state->in_space);

    state->player = player;

    update_texture(dt);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE: state.running = false; break;
            case GLFW_KEY_LEFT: state.controls.left = true; break;
            case GLFW_KEY_RIGHT: state.controls.right = true; break;
            case GLFW_KEY_UP: state.controls.up = true; break;
            case GLFW_KEY_DOWN: state.controls.down = true; break;
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

    if (!glfwInit()) {
        std::cerr << "Cannot initialize GLFW\n";
        return 1;
    }

    const int width = 1024;
    const int height = 1024;

    GLFWwindow * window = glfwCreateWindow(width, height, "Awesome game", NULL, NULL);
    if (!window) {
        std::cerr << "Cannot open window\n";
        return 1;
    }

    glfwMakeContextCurrent(window);

    state = {
        .rooms = {
            Room{
                .walls = {
                    Wall{Vec2{10, 10}, Vec2{15, 10}},
                    Wall{Vec2{20, 10}, Vec2{30, 10}},
                    Wall{Vec2{30, 10}, Vec2{30, 15}},
                    Wall{Vec2{30, 20}, Vec2{30, 30}},
                    Wall{Vec2{30, 30}, Vec2{10, 30}},
                    Wall{Vec2{10, 30}, Vec2{10, 10}},
                }
            },
            Room{
                .walls = {
                    Wall{Vec2{30, 10}, Vec2{60, 10}},
                    Wall{Vec2{60, 10}, Vec2{60, 30}},
                    Wall{Vec2{60, 30}, Vec2{30, 30}},
                    Wall{Vec2{30, 30}, Vec2{30, 20}},
                    Wall{Vec2{30, 15}, Vec2{30, 10}},
                }
            },
        },
        .current_room = 0,
        .doors = {
            Door{.hinge = Vec2{30,20}, .length = 5, .angle = M_PI_2, .room0 = 0, .room1 = 1}
        },
        .sas = Sas{Vec2{15, 10}, Vec2{20, 10}},
        .player = Player{
            .pos = Vec2{15, 15},
            .size = 3,
        },
    };

    state.tex = make_texture();
    state.stars = generate_star_field();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);  

    Timer timer_dt;
    timer_dt.start();
    
    glfwSetKeyCallback(window, key_callback);

    // glViewport(0, 0, gwa.width, gwa.height);
    
    state.running = true;
    while (state.running) {
        if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
            state.controls.use_js = true;
            joystick_control(GLFW_JOYSTICK_1);
        }

        float dt = timer_dt.tick();
        update(&state, dt);

        draw(state);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}


#if 0

Game features
- Limited vision
- Sound effect (going through a door)
- Animations (door opening)
- Star behind
- Generate rooms procedurally
- Gamepad control
- Sprites

Done
- No gravity outdoor (use SAS, have to use environment or propellant to move)
- Go to other room

Game story
- In a spaceship, explore
- Magnetic floor (simulate gravity)
- Can go outdoor

#endif

