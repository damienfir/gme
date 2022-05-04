#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>


// Use OpenGL: https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib

void draw() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1., 1., -1., 1., 1., 20.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

    glBegin(GL_QUADS);
    glColor3f(1., 0., 0.); glVertex3f(-.75, -.75, 0.);
    glColor3f(0., 1., 0.); glVertex3f( .75, -.75, 0.);
    glColor3f(0., 0., 1.); glVertex3f( .75,  .75, 0.);
    glColor3f(1., 1., 0.); glVertex3f(-.75,  .75, 0.);
    glEnd();
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
    swa.event_mask = ExposureMask | KeyPressMask;

    Window w = XCreateWindow(d, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XMapWindow(d, w);
    XStoreName(d, w, "Awesome game");

    GLXContext glc = glXCreateContext(d, vi, NULL, GL_TRUE);
    glXMakeCurrent(d, w, glc);
    // Visual* vis = XDefaultVisual(d, s);
    // Window w = XCreateSimpleWindow(d, XDefaultRootWindow(d), 0, 0, width, height, 1, XBlackPixel(d, s), XWhitePixel(d, s));
    // XSelectInput(d, w,
    //         ExposureMask
    //         | KeyPressMask
    //         | KeyReleaseMask);
    // XMapWindow(d, w);

    // XGCValues xgc_values;
    // GC gc = XCreateGC(d, w, 0, &xgc_values);

    glEnable(GL_DEPTH_TEST);

    int fps_60 = 1000/60;
    auto previous_timestamp = std::chrono::high_resolution_clock::now();
    XEvent e;
    for (;;) {
        while (XPending(d) > 0) {
            XNextEvent(d, &e);

            if (e.type == KeyPress) {
                KeySym keysym;
                XLookupString(&e.xkey, NULL, 0, &keysym, NULL);

                if (keysym == XK_Escape) {
                    return 0;
                }
            } else if (e.type == KeyRelease) {
                KeySym keysym;
                XLookupString(&e.xkey, NULL, 0, &keysym, NULL);
            }
        }

        XWindowAttributes gwa;
        XGetWindowAttributes(d, w, &gwa);
        glViewport(0, 0, gwa.width, gwa.height);
        draw();
        glXSwapBuffers(d, w);

        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - previous_timestamp;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < fps_60) {
            std::this_thread::sleep_for(std::chrono::milliseconds(fps_60) - elapsed);
        }
        previous_timestamp = std::chrono::high_resolution_clock::now();
    }

    return 0;
}
