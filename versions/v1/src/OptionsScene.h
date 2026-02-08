// src/OptionsScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

// Placeholder options scene (panel screen) — extend later with toggles/sliders
class OptionsScene : public Scene {
public:
  explicit OptionsScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  Game* m_game = nullptr; // not owned
};
