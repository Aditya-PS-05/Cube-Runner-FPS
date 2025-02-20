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
    int hitCount = 0;  // Track number of hits taken
    float lastShotTime;  // Track time since last shot
    int shotCount;       // Track number of shots fired
    void resetAI();  // Add this method declaration

    Player(SDL_Renderer* renderer, float x = 14.7f, float y = 5.09f, bool local = true, bool bot = false);
    ~Player();
    
    // Core functions
    void shoot();
    void update(float deltaTime, const std::string& map, int mapWidth);
    void render(SDL_Renderer* renderer, const Player& viewingPlayer, float FOV, const std::string& map, int mapWidth, int screenWidth, int screenHeight);
    void loadPlayerModel(SDL_Renderer* renderer);
    void takeDamage(float amount);

    // Add these new method declarations
    bool checkLineOfSight(const Vector2D& targetPos, const std::string& map, int mapWidth);
    void findPathToTarget(const Vector2D& targetPos, float deltaTime, const std::string& map, int mapWidth);
    
    // Bot AI methods
    void updateBot(float deltaTime, const Player& target, const std::string& map, int mapWidth);
    void moveTowardsPlayer(const Player& target, float deltaTime, const std::string& map, int mapWidth);
    float getAngleToTarget(const Vector2D& targetPos) const;
    float getDistanceToTarget(const Vector2D& targetPos) const;
    
    // Status methods
    bool isDead() const { return health <= 0; }
    int getScore() const { return score; }
    void addScore(int points) { score += points; }
    void respawn(float x, float y);
    int getHitCount() const { return hitCount; }
    bool canShoot() const;  // Add this new method

private:
    bool isActive;   // Add this member
};
