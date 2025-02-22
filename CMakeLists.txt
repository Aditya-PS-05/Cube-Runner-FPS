cmake_minimum_required(VERSION 3.10)
project(PUBGClone)

set(CMAKE_CXX_STANDARD 17)

# Find SDL2 packages
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Include directories
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    include
)

# Add source files
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Vector2D.cpp
    src/Bullet.cpp
)

# Create executable
add_executable(game ${SOURCES})

# Link libraries
target_link_libraries(game
    SDL2
    SDL2_image
    SDL2_ttf
    SDL2_mixer
)
