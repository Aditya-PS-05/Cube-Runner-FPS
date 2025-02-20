# Shadow Ops: Tactical Arena

A 2D top-down shooter game built with C++ and SDL2. Players must survive against AI-controlled bots in an enclosed arena, either eliminating 10 bots or surviving for 2 minutes to win.

## Features

- First-person perspective shooting gameplay
- AI-controlled enemy bots with tactical behavior
- Dynamic bot spawning system
- Health and damage system
- Timer-based gameplay
- Fullscreen immersive experience
- Pause menu and game state management

## Prerequisites

To build and run the game, you need:

- CMake (3.10 or higher)
- C++ compiler with C++11 support
- SDL2 development libraries
- SDL2_ttf development libraries

### Installing Dependencies on Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install cmake g++ libsdl2-dev libsdl2-ttf-dev
```
### Installing Dependencies on macOS

```bash
brew install cmake sdl2 sdl2_ttf
```

## Building the Game

1. Clone the repository:

```bash
git clone https://github.com/yourusername/shadow-ops.git
cd shadow-ops
```


2. Create and navigate to the build directory:
```bash
mkdir build
cd build
```

3. Generate build files and compile:
```bash
cmake ..
make
```

## Running the Game
```bash
./game
```


## Controls

- WASD or Arrow Keys: Move player
- Mouse: Aim
- K: Shoot
- P: Pause game
- M: Return to main menu
- Q (in main menu): Quit game

## Game Rules

- Eliminate 10 bots or survive for 2 minutes to win
- Each bot requires 3 hits to eliminate
- Bots respawn periodically
- Player has a health bar that depletes when hit by bot bullets
- Game ends if player's health reaches zero

## Project Structure

- `src/`: Source files
- `include/`: Header files
- `assets/`: Game assets (fonts, etc.)
- `CMakeLists.txt`: CMake build configuration