#pragma once
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

public:
    Game();
    ~Game();
    bool initialize();
    void handleInput(float deltaTime);
    void update(float deltaTime);
    void render();
    void run();
};