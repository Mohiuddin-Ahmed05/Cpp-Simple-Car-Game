// src/RaceScene.cpp
#include "RaceScene.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

RaceScene::RaceScene(Game* game) : m_game(game) {
  int w = 0, h = 0;
  if (m_game) m_game->getRenderSize(w, h);
  if (w <= 0 || h <= 0) { w = 960; h = 540; }
  initCar(w, h);

  // seed rng
  std::srand((unsigned)SDL_GetTicks());
}

void RaceScene::handleEvent(const SDL_Event&) {
  // Gameplay uses keyboard polling in update() for smooth control.
  // ESC is handled globally in Game.cpp.
}

void RaceScene::initCar(int w, int h) {
  // place car near bottom center
  m_car.rect.w = 52.f;
  m_car.rect.h = 82.f;
  m_car.rect.x = (w - m_car.rect.w) * 0.5f;
  m_car.rect.y = h - m_car.rect.h - 48.f;

  m_car.speed = 0.f;
  m_distance = 0.f;

  m_obs.clear();
  m_spawnTimer = 0.f;
  m_spawnInterval = 0.85f;
  m_difficultyTimer = 0.f;
  m_hitCooldown = 0.f;
  m_laneMarkerOffset = 0.f;
}

float RaceScene::roadLeft(int w) const  { return (w - m_roadWidth) * 0.5f; }
float RaceScene::roadRight(int w) const { return roadLeft(w) + m_roadWidth; }
float RaceScene::laneWidth() const      { return m_roadWidth / (float)std::max(1, m_lanes); }

int RaceScene::randInt(int minInclusive, int maxInclusive) {
  if (maxInclusive <= minInclusive) return minInclusive;
  int span = maxInclusive - minInclusive + 1;
  return minInclusive + (std::rand() % span);
}

bool RaceScene::rectsOverlap(const SDL_FRect& a, const SDL_FRect& b) const {
  return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
}

void RaceScene::clampCarToRoad(int w, int) {
  float left = roadLeft(w);
  float right = roadRight(w);

  // small shoulder inside the road
  const float pad = 10.f;
  float minX = left + pad;
  float maxX = right - pad - m_car.rect.w;

  m_car.rect.x = std::max(minX, std::min(m_car.rect.x, maxX));
}

void RaceScene::updateDifficulty(float dt) {
  // Every 10 seconds, slightly increase max speed and spawn frequency
  m_difficultyTimer += dt;
  if (m_difficultyTimer >= 10.f) {
    m_difficultyTimer = 0.f;

    m_car.maxSpeed = std::min(m_car.maxSpeed + 60.f, 1400.f);
    m_spawnInterval = std::max(0.45f, m_spawnInterval - 0.05f);
  }
}

void RaceScene::spawnObstacle(int w, int h) {
  // pick a lane, spawn above view
  int lane = randInt(0, m_lanes - 1);
  float lw = laneWidth();

  float left = roadLeft(w);
  float laneCenter = left + lw * (lane + 0.5f);

  Obstacle o{};
  o.rect.w = 56.f;
  o.rect.h = 56.f;
  o.rect.x = laneCenter - o.rect.w * 0.5f;
  o.rect.y = -o.rect.h - 10.f;

  // ensure not spawning directly stacked on another obstacle too close
  for (const auto& existing : m_obs) {
    if (std::abs(existing.rect.y - o.rect.y) < 120.f && std::abs(existing.rect.x - o.rect.x) < 40.f) {
      o.rect.x += lw * 0.33f; // nudge
      break;
    }
  }

  // clamp inside road just in case
  float minX = roadLeft(w) + 10.f;
  float maxX = roadRight(w) - 10.f - o.rect.w;
  o.rect.x = std::max(minX, std::min(o.rect.x, maxX));

  m_obs.push_back(o);
}

void RaceScene::update(float dt) {
  if (!m_game) return;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);
  if (w <= 0 || h <= 0) return;

  // clamp dt a bit to avoid giant jumps when window is dragged
  dt = std::min(dt, 0.05f);

  // --- Input (polling) ---
  const Uint8* keys = SDL_GetKeyboardState(nullptr);

  bool left  = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
  bool right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
  bool up    = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
  bool down  = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];

  // --- Speed model ---
  if (up) {
    m_car.speed += m_car.accel * dt;
  } else if (down) {
    m_car.speed -= m_car.brake * dt;
  } else {
    // friction toward 0
    if (m_car.speed > 0.f) {
      m_car.speed = std::max(0.f, m_car.speed - m_car.friction * dt);
    } else if (m_car.speed < 0.f) {
      m_car.speed = std::min(0.f, m_car.speed + m_car.friction * dt);
    }
  }

  m_car.speed = std::max(0.f, std::min(m_car.speed, m_car.maxSpeed));

  // --- Steering ---
  float steerDir = 0.f;
  if (left) steerDir -= 1.f;
  if (right) steerDir += 1.f;

  // Steering scales with speed so it feels responsive at higher speed
  float speedFactor = (m_car.maxSpeed > 1.f) ? (m_car.speed / m_car.maxSpeed) : 0.f;
  float steerPxPerSec = 180.f + m_car.steer * (0.35f + 0.65f * speedFactor);
  m_car.rect.x += steerDir * steerPxPerSec * dt;

  clampCarToRoad(w, h);

  // --- World scroll ---
  // Lane markers move down based on speed (illusion of forward movement)
  m_laneMarkerOffset += m_car.speed * dt;
  const float markerPeriod = 80.f;
  if (m_laneMarkerOffset >= markerPeriod) m_laneMarkerOffset = std::fmod(m_laneMarkerOffset, markerPeriod);

  // Obstacles move down relative to player speed
  for (auto& o : m_obs) {
    o.rect.y += m_car.speed * dt;
  }

  // Remove obstacles off screen
  m_obs.erase(
    std::remove_if(m_obs.begin(), m_obs.end(), [&](const Obstacle& o) {
      return o.rect.y > (float)h + 80.f;
    }),
    m_obs.end()
  );

  // Spawn
  m_spawnTimer += dt;
  if (m_spawnTimer >= m_spawnInterval) {
    m_spawnTimer = 0.f;
    spawnObstacle(w, h);
  }

  // Difficulty
  updateDifficulty(dt);

  // Score / distance
  m_distance += m_car.speed * dt;

  // Hit cooldown timer
  if (m_hitCooldown > 0.f) m_hitCooldown = std::max(0.f, m_hitCooldown - dt);

  // Collisions (penalty: drop speed + brief invuln flash)
  if (m_hitCooldown <= 0.f) {
    for (const auto& o : m_obs) {
      if (rectsOverlap(m_car.rect, o.rect)) {
        m_car.speed = std::max(0.f, m_car.speed * 0.35f);
        m_hitCooldown = 0.8f;
        break;
      }
    }
  }
}

void RaceScene::render(SDL_Renderer* r) {
  if (!m_game || !r) return;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);

  // Background
  SDL_SetRenderDrawColor(r, 10, 10, 14, 255);
  SDL_RenderClear(r);

  // Road
  SDL_FRect road { roadLeft(w), 0.f, m_roadWidth, (float)h };
  SDL_SetRenderDrawColor(r, 26, 26, 32, 255);
  SDL_RenderFillRectF(r, &road);

  // Road edge lines
  SDL_SetRenderDrawColor(r, 60, 60, 72, 255);
  SDL_RenderDrawLineF(r, road.x, 0.f, road.x, (float)h);
  SDL_RenderDrawLineF(r, road.x + road.w, 0.f, road.x + road.w, (float)h);

  // Lane markers (dashes)
  SDL_SetRenderDrawColor(r, 210, 210, 220, 220);
  float lw = laneWidth();
  for (int lane = 1; lane < m_lanes; lane++) {
    float x = road.x + lw * lane;
    for (float y = -80.f + m_laneMarkerOffset; y < h + 80.f; y += 80.f) {
      SDL_FRect dash { x - 3.f, y, 6.f, 34.f };
      SDL_RenderFillRectF(r, &dash);
    }
  }

  // Obstacles
  SDL_SetRenderDrawColor(r, 240, 90, 90, 255);
  for (const auto& o : m_obs) {
    SDL_RenderFillRectF(r, &o.rect);
  }

  // Car (flash on hit cooldown)
  bool flash = (m_hitCooldown > 0.f) && (((int)(SDL_GetTicks() / 90)) % 2 == 0);
  if (!flash) {
    SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
    SDL_RenderFillRectF(r, &m_car.rect);

    // tiny windshield accent for readability
    SDL_FRect win { m_car.rect.x + 10.f, m_car.rect.y + 12.f, m_car.rect.w - 20.f, 18.f };
    SDL_SetRenderDrawColor(r, 10, 10, 14, 160);
    SDL_RenderFillRectF(r, &win);
  }
}
