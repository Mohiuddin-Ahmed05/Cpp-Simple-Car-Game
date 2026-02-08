// src/Game.cpp
#include "Game.h"

#include <cstdio>
#include <utility>

// Scenes (you'll create these next)
#include "Scene.h"
#include "MenuScene.h"
#include "PlayScene.h"
#include "OptionsScene.h"

Game::Game(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font)
  : m_window(window), m_renderer(renderer), m_font(font) {
  setScene(SceneId::Menu);
}

Game::~Game() = default;

void Game::requestQuit() {
  m_running = false;
}

void Game::requestScene(SceneId next) {
  m_pendingId = next;
  m_hasPendingSceneChange = true;
}

void Game::getRenderSize(int& w, int& h) const {
  w = 0; h = 0;
  if (m_renderer) SDL_GetRendererOutputSize(m_renderer, &w, &h);
}

std::unique_ptr<Scene> Game::makeScene(SceneId id) {
  // Pass Game* so scenes can request transitions/quit.
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

void Game::handleEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT) {
    requestQuit();
    return;
  }

  // Global Escape behavior (matches your current logic)
  if (e.type == SDL_KEYDOWN && e.key.repeat == 0 && e.key.keysym.sym == SDLK_ESCAPE) {
    if (m_currentId == SceneId::Menu) requestQuit();
    else requestScene(SceneId::Menu);
    return;
  }

  if (m_scene) m_scene->handleEvent(e);
}

void Game::update(float dt) {
  if (m_scene) m_scene->update(dt);

  // Apply pending scene change AFTER update (safe point)
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
    // dt
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)((now - prev) / freq);
    prev = now;

    // events
    while (SDL_PollEvent(&e)) {
      handleEvent(e);
      if (!m_running) break;
    }
    if (!m_running) break;

    update(dt);
    render();

    SDL_RenderPresent(m_renderer);
  }
}
