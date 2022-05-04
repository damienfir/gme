#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>

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
        if (ms > 0) {
            std::cout << "fps: " << (1000.0/ms) << std::endl;
        }
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


const int MAX_WALLS = 32;

struct Room {
    Wall walls[MAX_WALLS];
    int n_walls;
};

struct Controls {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
};

struct Player {
    Vec2 pos;
    float size;
};

struct GameState {
    Room room;
    Player player;
    Controls controls;
};


const int coord_width = 100;
const int coord_height = 100;

Vec2 to_viewport(Vec2 v) {
    float x = 2 * v.x / (float)coord_width - 1;
    float y = - (2 * v.y / (float)coord_height - 1);
    return  Vec2{x,y};
}

void draw_rect(Rect rect) {
    float x0 = 2 * rect.x / (float)coord_width - 1;
    float y0 = - (2 * rect.y / (float)coord_height - 1);
    float x1 = 2 * (rect.x+rect.w)/(float)coord_width - 1;
    float y1 = - (2 * (rect.y+rect.h)/(float)coord_height - 1);

    glBegin(GL_LINE_LOOP);
    glColor3f(1, 1, 1);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
}

void draw_wall(Wall wall) {
    Vec2 a = to_viewport(wall.a);
    Vec2 b = to_viewport(wall.b);

    glBegin(GL_LINES);
    glColor3f(1,1,1);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void draw_player(Player player) {
    Vec2 q = to_viewport(player.pos);
    glPointSize(player.size);
    glBegin(GL_POINTS);
    glColor3f(0,0,1);
    glVertex2f(q.x, q.y);
    glEnd();
}

void draw(GameState state) {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1., 1., -1., 1., 1., 20.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

    for (int i = 0; i < state.room.n_walls; ++i) {
        draw_wall(state.room.walls[i]);
    }

    draw_player(state.player);
}

void update(GameState *state, float dt) {
    Vec2 player = state->player.pos;
    float d_player = 30;
    if (state->controls.left) {
        player.x -= d_player * dt;
    }
    if (state->controls.right) {
        player.x += d_player * dt;
    }
    if (state->controls.up) {
        player.y -= d_player * dt;
    }
    if (state->controls.down) {
        player.y += d_player * dt;
    }


    for (int i = 0; i < state->room.n_walls; ++i) {
        Wall w = state->room.walls[i];

        Vec2 BA = w.b - w.a;
        Vec2 PA = player - w.a;

        if (dot(BA, PA) < 0 || dot(-BA, player - w.b) < 0) {
            continue;
        }

        Vec2 perp = Vec2{BA.y, -BA.x};
        float dist_prev = dot((state->player.pos - w.a), perp);
        float dist_next = dot(PA, perp);
        if ((dist_prev > 0 && dist_next <= 0) || 
                (dist_prev < 0 && dist_next >= 0)) {
            player = state->player.pos;
            break;
        }
    }

    state->player.pos = player;
}

int main(int argc, char** argv) {

    Display* d = XOpenDisplay(nullptr);
    if (d == nullptr) {
        std::cout << "error" << std::endl;
    }
    
    const int width = 1024;
    const int height = 1024;

    int s = XDefaultScreen(d);
    std::cout << "all planes: " << std::bitset<sizeof(unsigned long)*8>(XAllPlanes()) << std::endl;
    std::cout << "Default depth: " << XDefaultDepth(d, s) << std::endl;

    Window root = DefaultRootWindow(d);

    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo* vi = glXChooseVisual(d, 0, att);
    if (vi == NULL) {
        std::cerr << "Error initializing visualinfo\n";
        return 1;
    }

    Colormap cmap = XCreateColormap(d, root, vi->visual, AllocNone);

    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask;

    Window w = XCreateWindow(d, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XMapWindow(d, w);
    XStoreName(d, w, "Awesome game");

    GLXContext glc = glXCreateContext(d, vi, NULL, GL_TRUE);
    glXMakeCurrent(d, w, glc);

    GameState state{
        .room = Room{
            .walls = {
                Wall{Vec2{10, 10}, Vec2{60, 10}},
                Wall{Vec2{60, 10}, Vec2{60, 60}},
                Wall{Vec2{60, 60}, Vec2{10, 60}},
                Wall{Vec2{10, 60}, Vec2{10, 10}},
            },
            .n_walls = 5,
        },
        .player = Player{
            .pos = Vec2{15, 15},
            .size = 10,
        },
    };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);

    Timer timer_dt;
    timer_dt.start();

    XEvent e;
    for (;;) {
        while (XPending(d) > 0) {
            XNextEvent(d, &e);

            if (e.type == KeyPress) {
                KeySym keysym;
                XLookupString(&e.xkey, NULL, 0, &keysym, NULL);

                switch (keysym) {
                    case XK_Escape: return 0; break;
                    case XK_Left: state.controls.left = true; break;
                    case XK_Right: state.controls.right = true; break;
                    case XK_Up: state.controls.up = true; break;
                    case XK_Down: state.controls.down = true; break;
                }

                std::cout << "Key down: " << XKeysymToString(keysym) << std::endl;

            } else if (e.type == KeyRelease) {
                KeySym keysym;
                XLookupString(&e.xkey, NULL, 0, &keysym, NULL);

                switch (keysym) {
                    case XK_Left: state.controls.left = false; break;
                    case XK_Right: state.controls.right = false; break;
                    case XK_Up: state.controls.up = false; break;
                    case XK_Down: state.controls.down = false; break;
                }

                std::cout << "Key up:   " << XKeysymToString(keysym) << std::endl;
            }
        }

        float dt = timer_dt.tick();
        update(&state, dt);

        XWindowAttributes gwa;
        XGetWindowAttributes(d, w, &gwa);
        glViewport(0, 0, gwa.width, gwa.height);
        draw(state);
        glXSwapBuffers(d, w);
    }

    return 0;
}
