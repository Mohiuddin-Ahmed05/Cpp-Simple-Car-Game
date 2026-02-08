// src/RaceScene.cpp
#include "RaceScene.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>

// Small local text helper (so we don't need to change Text.h/Text.cpp)
static void drawTextAt(SDL_Renderer* r, TTF_Font* font, const char* text, float x, float y) {
  if (!r || !font || !text) return;

  SDL_Color color{230, 235, 245, 255};
  SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
  if (!surf) return;

  SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
  if (!tex) { SDL_FreeSurface(surf); return; }

  SDL_FRect dst{ x, y, (float)surf->w, (float)surf->h };

  SDL_FreeSurface(surf);
  SDL_RenderCopyF(r, tex, nullptr, &dst);
  SDL_DestroyTexture(tex);
}

RaceScene::RaceScene(Game* game) : m_game(game) {
  int w = 0, h = 0;
  if (m_game) m_game->getRenderSize(w, h);
  if (w <= 0 || h <= 0) { w = 960; h = 540; }

  std::srand((unsigned)SDL_GetTicks());

  applyLevel(1, /*resetProgress=*/true);
  initCar(w, h);
}

void RaceScene::handleEvent(const SDL_Event& e) {
  if (!m_game) return;

  if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
    const SDL_Keycode key = e.key.keysym.sym;

    // Level transitions / retry
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
      if (m_state == State::LevelComplete) {
        applyLevel(m_level + 1, /*resetProgress=*/true);
        int w = 0, h = 0;
        m_game->getRenderSize(w, h);
        initCar(w, h);
      } else if (m_state == State::GameOver) {
        // restart SAME level
        applyLevel(m_level, /*resetProgress=*/true);
        int w = 0, h = 0;
        m_game->getRenderSize(w, h);
        initCar(w, h);
      }
      return;
    }
  }

  // Gameplay uses keyboard polling in update() for smooth control.
  // ESC is handled globally in Game.cpp.
}

RaceScene::LevelConfig RaceScene::getConfigForLevel(int level) const {
  // Simple tuning table via formula (stable + predictable).
  // You can swap this to a vector table later if you want per-level handcrafted values.
  LevelConfig c{};

  const int L = std::max(1, level);

  c.lanes = (L < 4) ? 3 : (L < 8 ? 4 : 5);

  // Road narrows a bit over time (but not too much)
  c.roadWidth = std::max(420.f, 560.f - 10.f * (float)(L - 1));

  // Target distance increases each level
  c.targetDistance = 4200.f + 900.f * (float)(L - 1);

  // Speed increases each level (cap it)
  c.maxSpeed = std::min(900.f + 70.f * (float)(L - 1), 1450.f);

  // Spawn interval decreases each level (cap it)
  c.spawnInterval = std::max(0.42f, 0.90f - 0.05f * (float)(L - 1));

  // Minimum vertical gap grows with speed (keeps it fair at higher speed)
  // Think of this as "reaction time window" expressed in pixels.
  float speedFactor = std::min(c.maxSpeed / 1200.f, 1.3f);
  c.minGapY = 140.f + 90.f * speedFactor;

  // Obstacles slightly bigger over time
  c.obstacleW = std::min(72.f, 56.f + 2.f * (float)(L - 1));
  c.obstacleH = c.obstacleW;

  return c;
}

void RaceScene::applyLevel(int level, bool resetProgress) {
  m_level = std::max(1, level);
  m_cfg = getConfigForLevel(m_level);

  // Apply config to runtime parameters
  m_car.maxSpeed = m_cfg.maxSpeed;

  // Keep car "feel" mostly constant; you can scale these too if desired
  m_car.accel = 900.f + 20.f * (float)(m_level - 1);
  m_car.brake = 1400.f;
  m_car.friction = 650.f;
  m_car.steer = 520.f;

  if (resetProgress) {
    m_levelDistance = 0.f;
    m_spawnTimer = 0.f;
    m_laneMarkerOffset = 0.f;
    m_lastSpawnY = -10000.f;
    m_lastLane = -1;
    m_obs.clear();
    m_car.speed = 0.f;
    m_state = State::Racing;
  }
}

void RaceScene::initCar(int w, int h) {
  m_car.rect.w = 52.f;
  m_car.rect.h = 82.f;
  m_car.rect.x = (w - m_car.rect.w) * 0.5f;
  m_car.rect.y = h - m_car.rect.h - 48.f;
  m_car.speed = 0.f;

  // Clamp immediately in case road got narrower
  clampCarToRoad(w, h);
}

float RaceScene::roadLeft(int w) const  { return (w - m_cfg.roadWidth) * 0.5f; }
float RaceScene::roadRight(int w) const { return roadLeft(w) + m_cfg.roadWidth; }
float RaceScene::laneWidth() const      { return m_cfg.roadWidth / (float)std::max(1, m_cfg.lanes); }

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

  const float pad = 10.f;
  float minX = left + pad;
  float maxX = right - pad - m_car.rect.w;

  m_car.rect.x = std::max(minX, std::min(m_car.rect.x, maxX));
}

void RaceScene::spawnObstacle(int w, int h) {
  (void)h;

  // Always spawn at least one lane path remains (we spawn single obstacles only).
  // Fairness is handled with minGapY + avoiding extreme lane jumps repeatedly.
  const int lanes = std::max(1, m_cfg.lanes);

  // Build lane choices with a tiny bias against repeating same lane too much
  int lane = randInt(0, lanes - 1);
  if (m_lastLane >= 0 && lanes > 1) {
    // 60% chance: choose a different lane than last time
    if ((std::rand() % 10) < 6) {
      int tries = 0;
      while (lane == m_lastLane && tries++ < 6) lane = randInt(0, lanes - 1);
    }
  }

  float lw = laneWidth();
  float left = roadLeft(w);
  float laneCenter = left + lw * (lane + 0.5f);

  Obstacle o{};
  o.lane = lane;
  o.rect.w = m_cfg.obstacleW;
  o.rect.h = m_cfg.obstacleH;
  o.rect.x = laneCenter - o.rect.w * 0.5f;
  o.rect.y = -o.rect.h - 10.f;

  // Clamp inside road just in case
  float minX = roadLeft(w) + 10.f;
  float maxX = roadRight(w) - 10.f - o.rect.w;
  o.rect.x = std::max(minX, std::min(o.rect.x, maxX));

  m_obs.push_back(o);
  m_lastSpawnY = o.rect.y; // top of screen
  m_lastLane = lane;
}

void RaceScene::update(float dt) {
  if (!m_game) return;

  int w = 0, h = 0;
  m_game->getRenderSize(w, h);
  if (w <= 0 || h <= 0) return;

  dt = std::min(dt, 0.05f);

  // If not racing, freeze gameplay (render overlay only)
  if (m_state != State::Racing) return;

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
    if (m_car.speed > 0.f) {
      m_car.speed = std::max(0.f, m_car.speed - m_car.friction * dt);
    }
  }

  m_car.speed = std::max(0.f, std::min(m_car.speed, m_car.maxSpeed));

  // --- Steering ---
  float steerDir = 0.f;
  if (left) steerDir -= 1.f;
  if (right) steerDir += 1.f;

  float speedFactor = (m_car.maxSpeed > 1.f) ? (m_car.speed / m_car.maxSpeed) : 0.f;
  float steerPxPerSec = 200.f + m_car.steer * (0.35f + 0.65f * speedFactor);
  m_car.rect.x += steerDir * steerPxPerSec * dt;

  clampCarToRoad(w, h);

  // --- World scroll ---
  m_laneMarkerOffset += m_car.speed * dt;
  const float markerPeriod = 80.f;
  if (m_laneMarkerOffset >= markerPeriod) m_laneMarkerOffset = std::fmod(m_laneMarkerOffset, markerPeriod);

  for (auto& o : m_obs) o.rect.y += m_car.speed * dt;

  // Remove obstacles off screen
  m_obs.erase(
    std::remove_if(m_obs.begin(), m_obs.end(), [&](const Obstacle& o) {
      return o.rect.y > (float)h + 120.f;
    }),
    m_obs.end()
  );

  // --- Spawn logic ---
  m_spawnTimer += dt;

  // Additional fairness: require enough vertical spacing between consecutive obstacles
  // (since they spawn above screen at similar y, spacing is effectively time-based)
  // We check the "highest" (smallest y) obstacle currently alive.
  float highestY = 999999.f;
  for (const auto& o : m_obs) highestY = std::min(highestY, o.rect.y);

  bool spacingOK = (m_obs.empty()) || (highestY > m_cfg.minGapY);

  if (m_spawnTimer >= m_cfg.spawnInterval && spacingOK) {
    m_spawnTimer = 0.f;
    spawnObstacle(w, h);
  }

  // --- Progress ---
  m_levelDistance += m_car.speed * dt;
  if (m_levelDistance >= m_cfg.targetDistance) {
    m_state = State::LevelComplete;
    // Freeze speed for nicer finish
    m_car.speed = 0.f;
  }

  // --- Collisions (FAIL) ---
  for (const auto& o : m_obs) {
    if (rectsOverlap(m_car.rect, o.rect)) {
      m_state = State::GameOver;
      m_car.speed = 0.f;
      break;
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
  SDL_FRect road { roadLeft(w), 0.f, m_cfg.roadWidth, (float)h };
  SDL_SetRenderDrawColor(r, 26, 26, 32, 255);
  SDL_RenderFillRectF(r, &road);

  // Road edge lines
  SDL_SetRenderDrawColor(r, 60, 60, 72, 255);
  SDL_RenderDrawLineF(r, road.x, 0.f, road.x, (float)h);
  SDL_RenderDrawLineF(r, road.x + road.w, 0.f, road.x + road.w, (float)h);

  // Lane markers
  SDL_SetRenderDrawColor(r, 210, 210, 220, 220);
  float lw = laneWidth();
  for (int lane = 1; lane < m_cfg.lanes; lane++) {
    float x = road.x + lw * lane;
    for (float y = -80.f + m_laneMarkerOffset; y < h + 80.f; y += 80.f) {
      SDL_FRect dash { x - 3.f, y, 6.f, 34.f };
      SDL_RenderFillRectF(r, &dash);
    }
  }

  // Obstacles
  SDL_SetRenderDrawColor(r, 240, 90, 90, 255);
  for (const auto& o : m_obs) SDL_RenderFillRectF(r, &o.rect);

  // Car
  SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
  SDL_RenderFillRectF(r, &m_car.rect);
  SDL_FRect win { m_car.rect.x + 10.f, m_car.rect.y + 12.f, m_car.rect.w - 20.f, 18.f };
  SDL_SetRenderDrawColor(r, 10, 10, 14, 160);
  SDL_RenderFillRectF(r, &win);

  // HUD
  TTF_Font* font = m_game->font();
  if (font) {
    char hud[192];
    std::snprintf(
      hud, sizeof(hud),
      "Level %d   Distance: %d / %d",
      m_level,
      (int)m_levelDistance,
      (int)m_cfg.targetDistance
    );

    // subtle panel behind HUD
    SDL_FRect hudPanel { 16.f, 12.f, 520.f, 44.f };
    SDL_SetRenderDrawColor(r, 12, 12, 16, 180);
    SDL_RenderFillRectF(r, &hudPanel);
    SDL_SetRenderDrawColor(r, 60, 60, 72, 220);
    SDL_RenderDrawRectF(r, &hudPanel);

    drawTextAt(r, font, hud, hudPanel.x + 14.f, hudPanel.y + 10.f);
  }

  // Overlays
  if (font && (m_state == State::GameOver || m_state == State::LevelComplete)) {
    SDL_FRect overlay { (w - 520.f) * 0.5f, (h - 220.f) * 0.5f, 520.f, 220.f };
    SDL_SetRenderDrawColor(r, 12, 12, 16, 220);
    SDL_RenderFillRectF(r, &overlay);
    SDL_SetRenderDrawColor(r, 80, 180, 255, 255);
    SDL_RenderDrawRectF(r, &overlay);

    const char* title = (m_state == State::GameOver) ? "CRASHED!" : "LEVEL COMPLETE!";
    const char* hint  = (m_state == State::GameOver)
      ? "Press Enter to retry this level"
      : "Press Enter to start next level";

    drawTextAt(r, font, title, overlay.x + 150.f, overlay.y + 50.f);
    drawTextAt(r, font, hint,  overlay.x + 85.f,  overlay.y + 130.f);
  }
}
