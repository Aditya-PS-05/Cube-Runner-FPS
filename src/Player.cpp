#include "Player.h"
#include <SDL2/SDL_image.h>
#include <cmath>
#include <algorithm>

Player::Player(SDL_Renderer* renderer, float x, float y, bool local) 
    : position(x, y), angle(0.0f), health(100.0f), isLocal(local), playerModel(nullptr) {
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

void Player::loadPlayerModel(SDL_Renderer* renderer) {
    // Create a simple human-shaped texture
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 32, 64, 32, 0, 0, 0, 0);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));  // Black silhouette
    
    // Create basic human shape
    SDL_Rect head = {12, 0, 8, 8};    // Head
    SDL_Rect body = {8, 8, 16, 32};   // Body
    SDL_Rect arms = {0, 8, 32, 8};    // Arms
    SDL_Rect legs = {8, 40, 16, 24};  // Legs
    
    SDL_FillRect(surface, &head, SDL_MapRGB(surface->format, 50, 50, 50));
    SDL_FillRect(surface, &body, SDL_MapRGB(surface->format, 50, 50, 50));
    SDL_FillRect(surface, &arms, SDL_MapRGB(surface->format, 50, 50, 50));
    SDL_FillRect(surface, &legs, SDL_MapRGB(surface->format, 50, 50, 50));
    
    playerModel = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void Player::render(SDL_Renderer* renderer, const Player& viewingPlayer, float FOV) {
    if (this == &viewingPlayer) return;  // Don't render viewing player

    // Calculate relative position to viewing player
    Vector2D relativePos = position + Vector2D(-viewingPlayer.position.x, -viewingPlayer.position.y);
    
    // Calculate angle relative to viewing player's view
    float relativeAngle = atan2(relativePos.y, relativePos.x) - viewingPlayer.angle;
    float distance = sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y);
    
    // Check if player is in view
    if (abs(relativeAngle) < FOV/2) {
        // Calculate screen position
        int screenX = (relativeAngle + FOV/2) * 800 / FOV;
        int screenY = 300 - (600 / distance);
        int height = 1200 / distance;
        
        SDL_Rect destRect = {screenX - height/4, screenY, height/2, height};
        SDL_RenderCopy(renderer, playerModel, NULL, &destRect);
    }
}