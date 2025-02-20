#include "Player.h"
#include <SDL2/SDL_image.h>
#include <cmath>
#include <algorithm>
#include <SDL2/SDL_mixer.h>

Player::Player(SDL_Renderer* renderer, float x, float y, bool local, bool bot) 
    : position(x, y), angle(0.0f), health(100.0f), isLocal(local), 
      playerModel(nullptr), isBot(bot), moveSpeed(2.5f), score(0), isAlive(true),
      lastShotTime(0.0f), shotCount(0), isActive(true) {
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
    bullets.emplace_back(position, bulletDir, 10.0f, isBot);
    
    // Play shoot sound for bots
    if (isBot) {
        Mix_Chunk* shootSound = Mix_LoadWAV("../assets/audio/gunghot.wav");
        if (shootSound) {
            Mix_PlayChannel(-1, shootSound, 0);
            Mix_FreeChunk(shootSound);
        }
    }
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

    // Update shot cooldown
    lastShotTime += deltaTime;

    // Calculate distance and check if there's a wall between bot and target
    float distance = getDistanceToTarget(target.position);
    bool hasLineOfSight = checkLineOfSight(target.position, map, mapWidth);

    if (hasLineOfSight) {
        // Always move towards player
        moveTowardsPlayer(target, deltaTime, map, mapWidth);

        // Shoot whenever possible and in range
        if (canShoot() && distance < 8.0f) {  // Increased range
            shoot();
            shotCount++;
            
            // Reset after 2 shots
            if (shotCount >= 2) {
                lastShotTime = 0.0f;
                shotCount = 0;
            }
        }
    } else {
        // Always try to find path to player
        findPathToTarget(target.position, deltaTime, map, mapWidth);
    }

    update(deltaTime, map, mapWidth);
}

bool Player::checkLineOfSight(const Vector2D& targetPos, const std::string& map, int mapWidth) {
    Vector2D direction = targetPos - position;
    float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
    direction.x /= distance;
    direction.y /= distance;

    // Check for walls between bot and target
    float stepSize = 0.1f;
    Vector2D currentPos = position;
    
    while (getDistanceToTarget(currentPos) > stepSize) {
        currentPos = currentPos + Vector2D(direction.x * stepSize, direction.y * stepSize);
        
        int mapX = static_cast<int>(currentPos.x);
        int mapY = static_cast<int>(currentPos.y);
        
        if (map[mapX * mapWidth + mapY] == '#') {
            return false;  // Wall detected between bot and target
        }
    }
    
    return true;  // No walls between bot and target
}

void Player::moveTowardsPlayer(const Player& target, float deltaTime, const std::string& map, int mapWidth) {
    // Calculate angle to target
    angle = getAngleToTarget(target.position);

    // Move towards target more aggressively
    float moveSpeed = 2.0f;
    Vector2D newPos = position + Vector2D(
        sinf(angle) * moveSpeed * deltaTime,
        cosf(angle) * moveSpeed * deltaTime
    );

    // Keep bots on the ground level
    int groundY = static_cast<int>(newPos.y);
    if (map[static_cast<int>(newPos.x) * mapWidth + groundY] != '#') {
        position = Vector2D(newPos.x, groundY);  // Lock Y position to integer value
    } else {
        // Try alternative paths while staying on ground
        const float angles[] = {M_PI/4, -M_PI/4, M_PI/2, -M_PI/2};
        for (float angleOffset : angles) {
            Vector2D altPos = position + Vector2D(
                sinf(angle + angleOffset) * moveSpeed * deltaTime,
                cosf(angle + angleOffset) * moveSpeed * deltaTime
            );
            if (map[static_cast<int>(altPos.x) * mapWidth + static_cast<int>(altPos.y)] != '#') {
                position = Vector2D(altPos.x, static_cast<int>(altPos.y));  // Lock Y position
                break;
            }
        }
    }
}

void Player::findPathToTarget(const Vector2D& targetPos, float deltaTime, const std::string& map, int mapWidth) {
    // Simple pathfinding: try to move around obstacles
    Vector2D direction = targetPos - position;
    float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
    direction.x /= distance;
    direction.y /= distance;

    // Try different angles to find a clear path
    const float angles[] = {0, M_PI/4, -M_PI/4, M_PI/2, -M_PI/2};
    
    for (float angleOffset : angles) {
        float testAngle = atan2(direction.y, direction.x) + angleOffset;
        Vector2D testPos = position + Vector2D(
            cosf(testAngle) * moveSpeed * deltaTime,
            sinf(testAngle) * moveSpeed * deltaTime
        );

        int testX = static_cast<int>(testPos.x);
        int testY = static_cast<int>(testPos.y);

        if (map[testX * mapWidth + testY] != '#') {
            position = testPos;
            angle = testAngle;
            break;
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
    // Create a larger surface for better visibility
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 64, 128, 32, 
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    if (!surface) {
        printf("Surface creation failed: %s\n", SDL_GetError());
        return;
    }

    // Set background transparent
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));
    
    // Create more visible human shape
    SDL_Rect head = {24, 8, 16, 16};      // Head
    SDL_Rect body = {20, 24, 24, 40};     // Body
    SDL_Rect leftArm = {8, 24, 12, 32};   // Left arm
    SDL_Rect rightArm = {44, 24, 12, 32}; // Right arm
    SDL_Rect leftLeg = {20, 64, 10, 40};  // Left leg
    SDL_Rect rightLeg = {34, 64, 10, 40}; // Right leg
    
    // Fill with bright colors for better visibility
    Uint32 color;
    if (isBot) {
        color = SDL_MapRGB(surface->format, 255, 50, 50);  // Bright red for bots
    } else {
        color = SDL_MapRGB(surface->format, 50, 255, 50);  // Bright green for player
    }
    
    // Draw the character parts
    SDL_FillRect(surface, &head, color);
    SDL_FillRect(surface, &body, color);
    SDL_FillRect(surface, &leftArm, color);
    SDL_FillRect(surface, &rightArm, color);
    SDL_FillRect(surface, &leftLeg, color);
    SDL_FillRect(surface, &rightLeg, color);
    
    // Create texture from surface
    playerModel = SDL_CreateTextureFromSurface(renderer, surface);
    if (!playerModel) {
        printf("Texture creation failed: %s\n", SDL_GetError());
    }
    
    SDL_FreeSurface(surface);
}

void Player::render(SDL_Renderer* renderer, const Player& viewingPlayer, float FOV, const std::string& map, int mapWidth, int screenWidth, int screenHeight) {
    if (this == &viewingPlayer || isDead()) return;

    // Calculate relative position to viewing player
    Vector2D relativePos = position - viewingPlayer.position;
    
    // Calculate angle relative to viewing player's view
    float relativeAngle = atan2(relativePos.x, relativePos.y) - viewingPlayer.angle;
    float distance = sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y);
    
    // Normalize angle to [-π, π]
    while (relativeAngle > M_PI) relativeAngle -= 2 * M_PI;
    while (relativeAngle < -M_PI) relativeAngle += 2 * M_PI;
    
    // Make bots more visible by adjusting rendering
    if (abs(relativeAngle) < FOV/2) {
        // Calculate screen position with adjusted size
        int screenX = static_cast<int>((0.5f + relativeAngle / FOV) * screenWidth);
        int screenY = screenHeight/2;  // Center vertically
        int size = static_cast<int>(800.0f / distance);  // Adjusted size scaling
        
        SDL_Rect destRect = {
            screenX - size/2,
            screenY - size,  // Position relative to screen center
            size,
            size * 2  // Make height twice the width for better proportions
        };
        
        // Add outline for better visibility
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &destRect);
        
        // Render the bot
        SDL_SetTextureBlendMode(playerModel, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(playerModel, 255);
        SDL_RenderCopy(renderer, playerModel, NULL, &destRect);
    }
}

void Player::takeDamage(float amount) {
    if (isBot) {
        health -= amount;
        hitCount++;
        if (hitCount >= 3) {  // Die after 3 hits
            health = 0;
            isAlive = false;
        }
    } else {
        health -= amount * 0.25f;  // Player takes reduced damage
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
    }
}

void Player::respawn(float x, float y) {
    position.x = x;
    position.y = y;
    health = 100.0f;
    hitCount = 0;
    isAlive = true;
    isActive = true;
    lastShotTime = 0.0f;
    shotCount = 0;
    moveSpeed = 2.5f;
}

bool Player::canShoot() const {
    if (shotCount >= 2) {
        return lastShotTime >= 5.0f;  // 5-second cooldown after 2 shots
    }
    return lastShotTime >= 0.5f;  // 0.5-second cooldown between individual shots
}

void Player::resetAI() {
    isActive = true;
    lastShotTime = 0.0f;
    shotCount = 0;
    moveSpeed = 2.5f;
}
