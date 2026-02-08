// src/OptionsScene.cpp
#include "OptionsScene.h"

#include <SDL2/SDL.h>
#include <cstdio>

#include "Text.h"

OptionsScene::OptionsScene(Game* game) : m_game(game) {}

void OptionsScene::cycleResolution() {
  if (!m_game) return;

  m_resIndex = (m_resIndex + 1) % RES_COUNT;
  int w = m_resolutions[m_resIndex][0];
  int h = m_resolutions[m_resIndex][1];
  m_game->setWindowedResolution(w, h);
}

void OptionsScene::handleEvent(const SDL_Event& e) {
  if (!m_game) return;

  if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
    switch (e.key.keysym.sym) {
      case SDLK_f: // fullscreen toggle
        m_game->toggleFullscreen();
        break;

      case SDLK_r: // cycle resolution (windowed only)
        cycleResolution();
        break;

      default:
        break;
    }
  }
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

  // centered panel
  SDL_FRect panel { w * 0.5f - 260.f, h * 0.5f - 170.f, 520.f, 340.f };
  SDL_SetRenderDrawColor(r, 30, 34, 48, 255);
  SDL_RenderFillRectF(r, &panel);
  SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
  SDL_RenderDrawRectF(r, &panel);

  // Title
  SDL_FRect titleBox { panel.x, panel.y + 18.f, panel.w, 44.f };
  drawTextCentered(r, m_game->font(), "Options", titleBox);

  // Current mode line
  const char* mode = m_game->isFullscreen() ? "Fullscreen: ON (F to toggle)" : "Fullscreen: OFF (F to toggle)";
  SDL_FRect modeBox { panel.x, panel.y + 86.f, panel.w, 36.f };
  drawTextCentered(r, m_game->font(), mode, modeBox);

  // Resolution line (renderer output size reflects actual size)
  char buf[128];
  std::snprintf(buf, sizeof(buf), "Resolution: %dx%d (R to cycle)", w, h);
  SDL_FRect resBox { panel.x, panel.y + 132.f, panel.w, 36.f };
  drawTextCentered(r, m_game->font(), buf, resBox);

  // Hint
  SDL_FRect hintBox { panel.x, panel.y + 220.f, panel.w, 80.f };
  drawTextCentered(r, m_game->font(), "Press ESC to return to Menu", hintBox);
}
