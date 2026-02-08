// src/RaceScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

#include <SDL2/SDL.h>
#include <vector>

// Top-down racing (LEVEL-BASED):
// - Crash on obstacle = FAIL
// - Each level requires reaching a target distance
// - Press Enter to advance level / retry same level
//
// Controls (Racing):
// - Steer: A/D or Left/Right
// - Accelerate: W or Up
// - Brake: S or Down
class RaceScene : public Scene {
public:
  explicit RaceScene(Game* game);

  void handleEvent(const SDL_Event& e) override;
  void update(float dt) override;
  void render(SDL_Renderer* r) override;

private:
  enum class State { Racing, LevelComplete, GameOver };

  struct LevelConfig {
    float targetDistance;     // "meters" in px-equivalent
    float roadWidth;
    int   lanes;

    float maxSpeed;
    float spawnInterval;      // seconds
    float minGapY;            // minimum vertical gap between obstacles (px)

    float obstacleW;
    float obstacleH;
  };

  struct Car {
    SDL_FRect rect;
    float speed = 0.f;

    // tunables
    float maxSpeed = 900.f;   // px/s
    float accel = 900.f;      // px/s^2
    float brake = 1400.f;     // px/s^2
    float friction = 650.f;   // px/s^2
    float steer = 520.f;      // px/s at full speed factor
  };

  struct Obstacle {
    SDL_FRect rect;
    int lane = 0;
  };

  Game* m_game = nullptr;

  // State / Level
  State m_state = State::Racing;
  int   m_level = 1;

  LevelConfig m_cfg{};
  float m_levelDistance = 0.f;

  // Road visuals
  float m_laneMarkerOffset = 0.f;

  // Car + obstacles
  Car m_car{};
  std::vector<Obstacle> m_obs;

  // Spawning
  float m_spawnTimer = 0.f;
  float m_lastSpawnY = -10000.f; // last spawned obstacle y (world space in screen coords)
  int   m_lastLane = -1;

private:
  // Level helpers
  LevelConfig getConfigForLevel(int level) const;
  void applyLevel(int level, bool resetProgress);

  // Scene helpers
  void initCar(int w, int h);
  float roadLeft(int w) const;
  float roadRight(int w) const;
  float laneWidth() const;

  void clampCarToRoad(int w, int h);

  void spawnObstacle(int w, int h);
  bool rectsOverlap(const SDL_FRect& a, const SDL_FRect& b) const;

  int randInt(int minInclusive, int maxInclusive);
};
