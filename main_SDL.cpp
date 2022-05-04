#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Game", 0, 0, 1024, 1024, SDL_VIDEO_OPENGL);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    SDL_Quit();

    return 0;
}
