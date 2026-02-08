// src/MenuScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

// Minimal menu scene: Start / Options / Quit
class MenuScene : public Scene {
public:
  explicit MenuScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  Game* m_game = nullptr; // not owned
  int   m_index = 0;

  float pulse() const; // simple highlight animation
};
