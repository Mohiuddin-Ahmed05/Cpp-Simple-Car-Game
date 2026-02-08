// src/OptionsScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

class OptionsScene : public Scene {
public:
  explicit OptionsScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  Game* m_game = nullptr; // not owned

  // windowed resolutions to cycle with R
  int m_resIndex = 0;
  static constexpr int RES_COUNT = 4;
  const int m_resolutions[RES_COUNT][2] = {
    { 960,  540 },
    { 1280, 720 },
    { 1600, 900 },
    { 1920, 1080 }
  };

  void cycleResolution();
};
