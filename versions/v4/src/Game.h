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

  void run();

  void requestQuit();
  void requestScene(SceneId next);

  SDL_Renderer* renderer() const { return m_renderer; }
  TTF_Font* font() const { return m_font; }
  void getRenderSize(int& w, int& h) const;

  // NEW: window access for display settings
  SDL_Window* window() const { return m_window; }

  // NEW: display controls
  bool isFullscreen() const { return m_isFullscreen; }
  void toggleFullscreen();            // desktop fullscreen
  void setWindowedResolution(int w, int h); // only applies if not fullscreen
  void applyDisplayChanges();         // rebuild renderer when needed

private:
  void handleEvent(const SDL_Event& e);
  void update(float dt);
  void render();

  void setScene(SceneId id);
  std::unique_ptr<Scene> makeScene(SceneId id);

private:
  SDL_Window*   m_window   = nullptr; // not owned
  SDL_Renderer* m_renderer = nullptr; // not owned (but we may recreate it)
  TTF_Font*     m_font     = nullptr; // not owned

  bool m_running = true;

  SceneId m_currentId = SceneId::Menu;
  SceneId m_pendingId = SceneId::Menu;
  bool    m_hasPendingSceneChange = false;

  std::unique_ptr<Scene> m_scene;

  // NEW: display state
  bool m_isFullscreen = false;
  bool m_rendererDirty = false;
};
