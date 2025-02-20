#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include "Vector2D.h"
#include "Bullet.h"

class Player {
public:
    Vector2D position;
    float angle;
    float health;
    std::vector<Bullet> bullets;
    SDL_Texture* playerModel;  // Player model texture
    bool isLocal;             // Is this the local player?

    Player(SDL_Renderer* renderer, float x = 14.7f, float y = 5.09f, bool local = true);
    ~Player();
    void shoot();
    void update(float deltaTime, const std::string& map, int mapWidth);
    void render(SDL_Renderer* renderer, const Player& viewingPlayer, float FOV);
    void loadPlayerModel(SDL_Renderer* renderer);
};