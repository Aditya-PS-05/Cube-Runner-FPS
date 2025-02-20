#include "Player.h"
#include <SDL2/SDL_image.h>
#include <cmath>
#include <algorithm>

Player::Player(SDL_Renderer* renderer, float x, float y, bool local, bool bot) 
    : position(x, y), angle(0.0f), health(100.0f), isLocal(local), 
      playerModel(nullptr), isBot(bot), moveSpeed(3.0f), score(0), isAlive(true) {
    if (renderer) {
        loadPlayerModel(renderer);
    }
}

Player::~Player() {
    if (playerModel) {
        SDL_DestroyTexture(playerModel);
    }
}

void Player::shoot() {
    Vector2D bulletDir(sinf(angle), cosf(angle));
    bullets.emplace_back(position, bulletDir);
}

void Player::update(float deltaTime, const std::string& map, int mapWidth) {
    // Update bullets
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.update(deltaTime);
            
            // Check wall collision
            int mapX = static_cast<int>(bullet.position.x);
            int mapY = static_cast<int>(bullet.position.y);
            if (map[mapX * mapWidth + mapY] == '#') {
                bullet.active = false;
            }
        }
    }
    
    // Remove inactive bullets
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }),
        bullets.end()
    );
}

void Player::updateBot(float deltaTime, const Player& target, const std::string& map, int mapWidth) {
    if (isDead()) return;

    // Move towards player more aggressively
    moveTowardsPlayer(target, deltaTime, map, mapWidth);

    // Shoot more frequently when closer to the player
    float distance = getDistanceToTarget(target.position);
    float shootChance = 5.0f * (1.0f - (distance / 10.0f));  // More likely to shoot when closer
    
    if (distance < 8.0f && (rand() % 100) < shootChance) {
        shoot();
    }

    update(deltaTime, map, mapWidth);
}

void Player::moveTowardsPlayer(const Player& target, float deltaTime, const std::string& map, int mapWidth) {
    // Calculate angle to target
    angle = getAngleToTarget(target.position);

    // Move towards target with slight randomization for more natural movement
    float randomOffset = (rand() % 100 - 50) / 500.0f;  // Small random angle adjustment
    Vector2D newPos = position + Vector2D(
        sinf(angle + randomOffset) * moveSpeed * deltaTime,
        cosf(angle + randomOffset) * moveSpeed * deltaTime
    );

    // Try to find alternative path if blocked by wall
    if (map[static_cast<int>(newPos.x) * mapWidth + static_cast<int>(newPos.y)] != '#') {
        position = newPos;
    } else {
        // Try moving sideways if blocked
        Vector2D sideStep = Vector2D(
            sinf(angle + 3.14159f/2) * moveSpeed * deltaTime,
            cosf(angle + 3.14159f/2) * moveSpeed * deltaTime
        );
        
        if (map[static_cast<int>(position.x + sideStep.x) * mapWidth + 
               static_cast<int>(position.y + sideStep.y)] != '#') {
            position = position + sideStep;
        }
    }
}

float Player::getAngleToTarget(const Vector2D& targetPos) const {
    return atan2(targetPos.x - position.x, targetPos.y - position.y);
}

float Player::getDistanceToTarget(const Vector2D& targetPos) const {
    float dx = position.x - targetPos.x;
    float dy = position.y - targetPos.y;
    return sqrt(dx*dx + dy*dy);
}

void Player::loadPlayerModel(SDL_Renderer* renderer) {
    // Create a larger human-shaped texture
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 64, 128, 32, 0, 0, 0, 0);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));  // Black background
    
    // Create more visible human shape
    SDL_Rect head = {24, 0, 16, 16};     // Larger head
    SDL_Rect body = {16, 16, 32, 64};    // Larger body
    SDL_Rect arms = {0, 32, 64, 16};     // Larger arms
    SDL_Rect legs = {16, 80, 32, 48};    // Larger legs
    
    // Fill with darker color for better visibility
    SDL_FillRect(surface, &head, SDL_MapRGB(surface->format, 30, 30, 30));
    SDL_FillRect(surface, &body, SDL_MapRGB(surface->format, 40, 40, 40));
    SDL_FillRect(surface, &arms, SDL_MapRGB(surface->format, 35, 35, 35));
    SDL_FillRect(surface, &legs, SDL_MapRGB(surface->format, 45, 45, 45));
    
    playerModel = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void Player::render(SDL_Renderer* renderer, const Player& viewingPlayer, float FOV) {
    if (this == &viewingPlayer || isDead()) return;

    // Calculate relative position to viewing player
    Vector2D relativePos = position + Vector2D(-viewingPlayer.position.x, -viewingPlayer.position.y);
    
    // Calculate angle relative to viewing player's view
    float relativeAngle = atan2(relativePos.y, relativePos.x) - viewingPlayer.angle;
    float distance = sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y);
    
    // Make bots more visible by adjusting rendering
    if (abs(relativeAngle) < FOV/2) {
        // Calculate screen position with adjusted size
        int screenX = (relativeAngle + FOV/2) * 800 / FOV;
        int screenY = 300 - (800 / distance);  // Increased vertical offset
        int height = 1600 / distance;          // Increased size
        
        SDL_Rect destRect = {
            screenX - height/3,     // Wider sprite
            screenY, 
            static_cast<int>(height/1.5),  // Fix narrowing conversion
            height
        };
        
        // Add outline for better visibility
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &destRect);
        
        // Render the bot
        SDL_RenderCopy(renderer, playerModel, NULL, &destRect);
    }
}