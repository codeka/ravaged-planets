#include <iostream>
#include <SDL.h>

#include "framework/settings.h"
#include "framework/logging.h"

namespace rp {
  void settings_initialize(int argc, char** argv);
}

int main(int argc, char** argv) {
  rp::settings_initialize(argc, argv);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  fw::logging_initialize();
  fw::debug << "Hello World!" << std::endl;

  SDL_Window *win = SDL_CreateWindow("Ravaged Planets", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
  if (win == nullptr) {
    std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == nullptr) {
    SDL_DestroyWindow(win);
    std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Event e;
  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
      if (e.type == SDL_KEYDOWN) {
        quit = true;
      }
      if (e.type == SDL_MOUSEBUTTONDOWN) {
        quit = true;
      }
    }

    SDL_RenderClear(ren);
    SDL_RenderPresent(ren);
  }

  SDL_Quit();
  return 0;
}
