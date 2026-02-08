// src/OptionsScene.cpp
#include "OptionsScene.h"

#include <SDL2/SDL.h>

OptionsScene::OptionsScene(Game* game) : m_game(game) {}

void OptionsScene::handleEvent(const SDL_Event&) {
  // Placeholder: add toggles later (volume, fullscreen, etc.)
  // Escape behavior is handled globally in Game.cpp.
}

void OptionsScene::update(float) {
  // nothing yet
}

void OptionsScene::render(SDL_Renderer* r) {
  if (!m_game || !r) return;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);

  SDL_SetRenderDrawColor(r, 16, 12, 20, 255);
  SDL_RenderClear(r);

  // draw a centered panel (same as your original placeholder)
  SDL_FRect panel { w * 0.5f - 220.f, h * 0.5f - 140.f, 440.f, 280.f };
  SDL_SetRenderDrawColor(r, 30, 34, 48, 255);
  SDL_RenderFillRectF(r, &panel);
  SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
  SDL_RenderDrawRectF(r, &panel);
}
