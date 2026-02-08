// src/Text.h
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Draw UTF-8 text centered inside a rectangle (utility for menus, panels, etc.)
void drawTextCentered(
  SDL_Renderer* renderer,
  TTF_Font* font,
  const char* text,
  const SDL_FRect& box
);
