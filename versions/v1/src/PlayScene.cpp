// src/PlayScene.cpp
#include "PlayScene.h"

#include <SDL2/SDL.h>
#include <algorithm>

PlayScene::PlayScene(Game* game) : m_game(game) {}

void PlayScene::handleEvent(const SDL_Event&) {
  // No event-based controls yet for gameplay.
  // (Escape is handled globally in Game.cpp)
}

void PlayScene::update(float dt) {
  if (!m_game) return;

  // Poll keyboard for smooth movement (matches your original behavior)
  const Uint8* keys = SDL_GetKeyboardState(nullptr);

  m_dx = 0.f;
  m_dy = 0.f;

  if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  m_dx -= 1.f;
  if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) m_dx += 1.f;
  if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    m_dy -= 1.f;
  if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  m_dy += 1.f;

  // normalize diagonal movement
  if (m_dx != 0.f && m_dy != 0.f) {
    m_dx *= 0.7071067f;
    m_dy *= 0.7071067f;
  }

  m_player.x += m_dx * m_speed * dt;
  m_player.y += m_dy * m_speed * dt;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);
  clampToWindow(w, h);
}

void PlayScene::clampToWindow(int w, int h) {
  m_player.x = std::max(0.f, std::min(m_player.x, (float)w - m_player.w));
  m_player.y = std::max(0.f, std::min(m_player.y, (float)h - m_player.h));
}

void PlayScene::render(SDL_Renderer* r) {
  if (!r) return;

  SDL_SetRenderDrawColor(r, 12, 12, 16, 255);
  SDL_RenderClear(r);

  SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
  SDL_RenderFillRectF(r, &m_player);
}
