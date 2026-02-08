// src/main.cpp
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>

#include "Game.h"

int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::printf("SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  if (TTF_Init() != 0) {
    std::printf("TTF_Init failed: %s\n", TTF_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
    "SDL2 Starter",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    960, 540,
    SDL_WINDOW_SHOWN
  );

  if (!window) {
    std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(
    window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if (!renderer) {
    std::printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  // Load font (run from /Game so assets/... resolves)
  TTF_Font* font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 28);
  if (!font) {
    std::printf("TTF_OpenFont failed: %s\n", TTF_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  {
    Game game(window, renderer, font);
    game.run();
  }

  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
  return 0;
}
