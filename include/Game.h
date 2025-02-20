#pragma once
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "Player.h"

class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    bool running;
    bool gameOver;
    int botCount;
    float botRespawnTime;
    std::string map;
    std::vector<std::unique_ptr<Player>> players;
    const int screenWidth;
    const int screenHeight;
    const int mapWidth;
    const int mapHeight;
    const float FOV;
    const float depth;
    float gameTimer;         // Track game time
    const float GAME_DURATION = 120.0f;  // 2 minutes in seconds
    int botsKilled;          // Track number of bots killed
    const int BOTS_TO_WIN = 10;  // Number of bots needed to kill to win
    float botSpawnTimer;     // Timer for spawning new bots
    const float BOT_SPAWN_INTERVAL = 15.0f;  // Spawn new bot every 15 seconds
    Mix_Music* backgroundMusic;
    Mix_Chunk* shootSound;

    enum class GameState {
        MENU,
        RULES,
        PLAYING,
        PAUSED,
        QUIT_CONFIRM,
        GAME_OVER
    };
    
    GameState gameState;
    void initializeMap();
    void renderView();
    float castRay(float angle, const Vector2D& start);
    void renderMinimap();
    void renderBullets();
    void renderPlayers();
    void renderHealthBar();
    void renderGameOver();
    void spawnBots(int count);
    void updateBots(float deltaTime);
    void checkBulletCollisions();
    void restart();
    void renderMenu();
    void renderRules();
    void renderPauseScreen();
    void renderTimer();
    void renderQuitConfirm();
    void initializeAudio();
    void cleanupAudio();

public:
    Game();
    ~Game();
    bool initialize();
    void handleInput(float deltaTime);
    void update(float deltaTime);
    void render();
    void run();
};
