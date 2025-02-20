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
    SDL_Texture* playerModel;
    bool isLocal;
    bool isBot;              // Flag for bot
    float moveSpeed;         // Movement speed
    int score;              // Player score
    bool isAlive;           // New: track if player/bot is alive

    Player(SDL_Renderer* renderer, float x = 14.7f, float y = 5.09f, bool local = true, bool bot = false);
    ~Player();
    
    // Core functions
    void shoot();
    void update(float deltaTime, const std::string& map, int mapWidth);
    void render(SDL_Renderer* renderer, const Player& viewingPlayer, float FOV);
    void loadPlayerModel(SDL_Renderer* renderer);
    void takeDamage(float amount) {
        if (isBot) {
            health -= amount;  // Bots take full damage
        } else {
            health -= amount * 0.5f;  // Player takes reduced damage
        }
        
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
    }
    
    // Bot AI methods
    void updateBot(float deltaTime, const Player& target, const std::string& map, int mapWidth);
    void moveTowardsPlayer(const Player& target, float deltaTime, const std::string& map, int mapWidth);
    float getAngleToTarget(const Vector2D& targetPos) const;
    float getDistanceToTarget(const Vector2D& targetPos) const;
    
    // Status methods
    bool isDead() const { return health <= 0; }
    int getScore() const { return score; }
    void addScore(int points) { score += points; }
};