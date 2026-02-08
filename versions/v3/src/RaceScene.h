// src/RaceScene.h
#pragma once

#include "Scene.h"
#include "Game.h"

#include <SDL2/SDL.h>
#include <vector>

// Top-down racing scene (MVP):
// - Road scroll illusion
// - Car steering + speed model
// - Obstacles spawn + collision penalty
// Controls:
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
  struct Car {
    SDL_FRect rect;
    float speed = 0.f;
    float maxSpeed = 900.f;   // px/s
    float accel = 900.f;      // px/s^2
    float brake = 1400.f;     // px/s^2
    float friction = 650.f;   // px/s^2
    float steer = 520.f;      // px/s at full speed factor
  };

  struct Obstacle {
    SDL_FRect rect;
  };

  Game* m_game = nullptr;

  // Road / lanes
  float m_roadWidth = 520.f;
  float m_laneMarkerOffset = 0.f; // scroll offset for dashed markers
  int   m_lanes = 3;

  // Car + obstacles
  Car m_car{};
  std::vector<Obstacle> m_obs;

  // Spawning / difficulty
  float m_spawnTimer = 0.f;
  float m_spawnInterval = 0.85f;  // seconds
  float m_difficultyTimer = 0.f;

  // Collision feedback
  float m_hitCooldown = 0.f;      // seconds of invuln/flash after hit

  // Score / distance
  float m_distance = 0.f;         // pixels traveled equivalent

private:
  void initCar(int w, int h);

  // Helpers
  float roadLeft(int w) const;
  float roadRight(int w) const;
  float laneWidth() const;

  void clampCarToRoad(int w, int h);
  void updateDifficulty(float dt);
  void spawnObstacle(int w, int h);
  bool rectsOverlap(const SDL_FRect& a, const SDL_FRect& b) const;

  // tiny RNG helper (deterministic enough for now)
  int randInt(int minInclusive, int maxInclusive);
};
