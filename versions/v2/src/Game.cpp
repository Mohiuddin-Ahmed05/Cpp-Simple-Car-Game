// src/Game.cpp
#include "Game.h"

#include <cstdio>
#include <utility>

#include "Scene.h"
#include "MenuScene.h"
#include "PlayScene.h"
#include "OptionsScene.h"

Game::Game(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font)
  : m_window(window), m_renderer(renderer), m_font(font) {
  setScene(SceneId::Menu);
}

Game::~Game() = default;

void Game::requestQuit() { m_running = false; }

void Game::requestScene(SceneId next) {
  m_pendingId = next;
  m_hasPendingSceneChange = true;
}

void Game::getRenderSize(int& w, int& h) const {
  w = 0; h = 0;
  if (m_renderer) SDL_GetRendererOutputSize(m_renderer, &w, &h);
}

std::unique_ptr<Scene> Game::makeScene(SceneId id) {
  switch (id) {
    case SceneId::Menu:    return std::make_unique<MenuScene>(this);
    case SceneId::Play:    return std::make_unique<PlayScene>(this);
    case SceneId::Options: return std::make_unique<OptionsScene>(this);
    default:               return std::make_unique<MenuScene>(this);
  }
}

void Game::setScene(SceneId id) {
  m_currentId = id;
  m_scene = makeScene(id);
}

// ---------------- NEW: Display controls ----------------

void Game::toggleFullscreen() {
  if (!m_window) return;

  m_isFullscreen = !m_isFullscreen;

  // Use desktop fullscreen for best “native” behavior
  Uint32 flags = m_isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
  if (SDL_SetWindowFullscreen(m_window, flags) != 0) {
    std::printf("SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
    // roll back if it failed
    m_isFullscreen = !m_isFullscreen;
    return;
  }

  // When fullscreen changes, it’s safest to recreate renderer
  m_rendererDirty = true;
}

void Game::setWindowedResolution(int w, int h) {
  if (!m_window) return;
  if (m_isFullscreen) return; // don't resize in fullscreen

  SDL_SetWindowSize(m_window, w, h);
  SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  // Resizing can also require renderer refresh depending on driver
  m_rendererDirty = true;
}

void Game::applyDisplayChanges() {
  if (!m_rendererDirty) return;
  m_rendererDirty = false;

  if (!m_window) return;

  // Recreate renderer with the same flags you used originally
  if (m_renderer) {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }

  m_renderer = SDL_CreateRenderer(
    m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if (!m_renderer) {
    std::printf("SDL_CreateRenderer failed after display change: %s\n", SDL_GetError());
    // If this fails, game can’t render—request quit
    requestQuit();
  }
}

// -------------------------------------------------------

void Game::handleEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT) {
    requestQuit();
    return;
  }

  // Global Escape behavior (matches your original logic)
  if (e.type == SDL_KEYDOWN && e.key.repeat == 0 && e.key.keysym.sym == SDLK_ESCAPE) {
    if (m_currentId == SceneId::Menu) requestQuit();
    else requestScene(SceneId::Menu);
    return;
  }

  if (m_scene) m_scene->handleEvent(e);
}

void Game::update(float dt) {
  if (m_scene) m_scene->update(dt);

  if (m_hasPendingSceneChange) {
    m_hasPendingSceneChange = false;
    setScene(m_pendingId);
  }
}

void Game::render() {
  if (m_scene) m_scene->render(m_renderer);
}

void Game::run() {
  if (!m_renderer) {
    std::printf("Game::run(): renderer is null\n");
    return;
  }

  SDL_Event e{};

  Uint64 prev = SDL_GetPerformanceCounter();
  const double freq = (double)SDL_GetPerformanceFrequency();

  while (m_running) {
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)((now - prev) / freq);
    prev = now;

    while (SDL_PollEvent(&e)) {
      handleEvent(e);
      if (!m_running) break;
    }
    if (!m_running) break;

    // NEW: if display changed, rebuild renderer safely between frames
    applyDisplayChanges();
    if (!m_running || !m_renderer) break;

    update(dt);
    render();

    SDL_RenderPresent(m_renderer);
  }
}
