# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)

add_executable(game
  src/main.cpp
  src/Game.cpp
  src/MenuScene.cpp
  src/PlayScene.cpp
  src/OptionsScene.cpp
  src/Text.cpp
)

target_include_directories(game PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${SDL2TTF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game PRIVATE
  ${SDL2_LIBRARIES}
  ${SDL2TTF_LIBRARIES}
)

# Helps when pkg-config adds special compile flags
target_compile_options(game PRIVATE
  ${SDL2_CFLAGS_OTHER}
  ${SDL2TTF_CFLAGS_OTHER}
)
