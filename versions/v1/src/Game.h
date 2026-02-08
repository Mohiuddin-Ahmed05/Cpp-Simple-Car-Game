// src/Game.h
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>

// Forward declarations
class Scene;

class Game {
public:
  enum class SceneId { Menu, Play, Options };

  Game(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font);
  ~Game();

  // Runs the main loop until quit is requested
  void run();

  // Called by scenes (indirectly) to change state
  void requestQuit();
  void requestScene(SceneId next);

  // Shared accessors for scenes
  SDL_Renderer* renderer() const { return m_renderer; }
  TTF_Font* font() const { return m_font; }
  void getRenderSize(int& w, int& h) const;

private:
  void handleEvent(const SDL_Event& e);
  void update(float dt);
  void render();

  void setScene(SceneId id);
  std::unique_ptr<Scene> makeScene(SceneId id);

private:
  SDL_Window*   m_window   = nullptr; // not owned
  SDL_Renderer* m_renderer = nullptr; // not owned
  TTF_Font*     m_font     = nullptr; // not owned

  bool m_running = true;

  SceneId m_currentId = SceneId::Menu;
  SceneId m_pendingId = SceneId::Menu;
  bool    m_hasPendingSceneChange = false;

  std::unique_ptr<Scene> m_scene;
};
