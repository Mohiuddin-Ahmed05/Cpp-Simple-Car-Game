// src/PlayScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

class PlayScene : public Scene {
public:
  explicit PlayScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  Game* m_game = nullptr; // not owned

  SDL_FRect m_player { 100.f, 100.f, 60.f, 60.f };
  float m_speed = 320.f;

  // movement intent (set via keyboard state polling in update)
  float m_dx = 0.f;
  float m_dy = 0.f;

  void clampToWindow(int w, int h);
};
