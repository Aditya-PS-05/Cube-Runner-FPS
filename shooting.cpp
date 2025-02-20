#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <SDL2/SDL.h>
#include <algorithm>
#include <chrono>

class Vector2D {
public:
    float x, y;
    Vector2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    
    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }
    
    Vector2D operator*(float scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }
};

class Bullet {
public:
    Vector2D position;
    Vector2D direction;
    float speed;
    bool active;
    
    Bullet(Vector2D pos, Vector2D dir, float spd = 10.0f)
        : position(pos), direction(dir), speed(spd), active(true) {}
    
    void update(float deltaTime) {
        position = position + direction * (speed * deltaTime);
    }
};

class Player {
public:
    Vector2D position;
    float angle;
    float health;
    std::vector<Bullet> bullets;
    
    Player(float x = 14.7f, float y = 5.09f)
        : position(x, y), angle(0.0f), health(100.0f) {}
    
    void shoot() {
        Vector2D bulletDir(sinf(angle), cosf(angle));
        bullets.emplace_back(position, bulletDir);
    }
    
    void update(float deltaTime, const std::string& map, int mapWidth) {
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
};

class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    std::string map;
    std::vector<std::unique_ptr<Player>> players;
    const int screenWidth;
    const int screenHeight;
    const int mapWidth;
    const int mapHeight;
    const float FOV;
    const float depth;
    
public:
    Game() : screenWidth(800), screenHeight(600),
             mapWidth(16), mapHeight(16),
             FOV(3.14159f / 4.0f), depth(16.0f),
             running(false) {
        initializeMap();
        players.push_back(std::make_unique<Player>());
    }
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow("PUBG-like Shooter",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            screenWidth, screenHeight, SDL_WINDOW_SHOWN);
            
        if (!window) {
            std::cout << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            
        if (!renderer) {
            std::cout << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        running = true;
        return true;
    }
    
    void handleInput(float deltaTime) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    players[0]->shoot();
                }
            }
        }
        
        const Uint8* state = SDL_GetKeyboardState(NULL);
        auto& player = players[0];
        float speed = 5.0f;
        
        if (state[SDL_SCANCODE_ESCAPE]) running = false;
        if (state[SDL_SCANCODE_A]) player->angle -= speed * 0.75f * deltaTime;
        if (state[SDL_SCANCODE_D]) player->angle += speed * 0.75f * deltaTime;
        
        if (state[SDL_SCANCODE_W]) {
            Vector2D newPos = player->position + Vector2D(
                sinf(player->angle) * speed * deltaTime,
                cosf(player->angle) * speed * deltaTime
            );
            if (map[static_cast<int>(newPos.x) * mapWidth + static_cast<int>(newPos.y)] != '#') {
                player->position = newPos;
            }
        }
        
        if (state[SDL_SCANCODE_S]) {
            Vector2D newPos = player->position + Vector2D(
                -sinf(player->angle) * speed * deltaTime,
                -cosf(player->angle) * speed * deltaTime
            );
            if (map[static_cast<int>(newPos.x) * mapWidth + static_cast<int>(newPos.y)] != '#') {
                player->position = newPos;
            }
        }
    }
    
    void update(float deltaTime) {
        for (auto& player : players) {
            player->update(deltaTime, map, mapWidth);
        }
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Render 3D view
        renderView();
        
        // Render minimap
        renderMinimap();
        
        // Render bullets
        renderBullets();
        
        SDL_RenderPresent(renderer);
    }
    
    void run() {
        auto lastTime = std::chrono::system_clock::now();
        
        while (running) {
            auto currentTime = std::chrono::system_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            handleInput(deltaTime);
            update(deltaTime);
            render();
        }
    }
    
    ~Game() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    
private:
    void initializeMap() {
        map += "################";
        map += "#..............#";
        map += "#..............#";
        map += "#..............#";
        map += "#....##........#";
        map += "#....##........#";
        map += "#..............#";
        map += "#..............#";
        map += "#..............#";
        map += "#......####....#";
        map += "#......#.......#";
        map += "#......#.......#";
        map += "#..............#";
        map += "#......########";
        map += "#..............#";
        map += "################";
    }
    
    void renderView() {
        auto& player = players[0];
        
        for (int x = 0; x < screenWidth; x++) {
            float rayAngle = (player->angle - FOV/2.0f) + ((float)x / (float)screenWidth) * FOV;
            
            float distanceToWall = castRay(rayAngle, player->position);
            
            int ceiling = (float)(screenHeight/2.0) - screenHeight / ((float)distanceToWall);
            int floor = screenHeight - ceiling;
            
            // Wall shading
            Uint8 shade = (1.0f - (distanceToWall / depth)) * 255;
            
            SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
            SDL_RenderDrawLine(renderer, x, ceiling, x, floor);
            
            // Floor
            SDL_SetRenderDrawColor(renderer, 0, shade/2, 0, 255);
            SDL_RenderDrawLine(renderer, x, floor, x, screenHeight);
            
            // Ceiling
            SDL_SetRenderDrawColor(renderer, shade/2, shade/2, shade/2, 255);
            SDL_RenderDrawLine(renderer, x, 0, x, ceiling);
        }
    }
    
    float castRay(float angle, const Vector2D& start) {
        float distanceToWall = 0.0f;
        float stepSize = 0.1f;
        
        Vector2D ray(sinf(angle), cosf(angle));
        
        bool hitWall = false;
        while (!hitWall && distanceToWall < depth) {
            distanceToWall += stepSize;
            
            int testX = (int)(start.x + ray.x * distanceToWall);
            int testY = (int)(start.y + ray.y * distanceToWall);
            
            if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight) {
                hitWall = true;
                distanceToWall = depth;
            }
            else if (map[testX * mapWidth + testY] == '#') {
                hitWall = true;
            }
        }
        
        return distanceToWall;
    }
    
    void renderMinimap() {
        int mapSize = 100;
        int cellSize = mapSize / mapWidth;
        
        // Draw map
        for (int x = 0; x < mapWidth; x++) {
            for (int y = 0; y < mapHeight; y++) {
                if (map[y * mapWidth + x] == '#')
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                else
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                
                SDL_Rect rect = {x * cellSize, y * cellSize, cellSize - 1, cellSize - 1};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        
        // Draw players
        for (const auto& player : players) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect playerRect = {
                static_cast<int>(player->position.y * cellSize) - 2,
                static_cast<int>(player->position.x * cellSize) - 2,
                4, 4
            };
            SDL_RenderFillRect(renderer, &playerRect);
        }
    }
    
    void renderBullets() {
        for (const auto& player : players) {
            for (const auto& bullet : player->bullets) {
                if (bullet.active) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_Rect bulletRect = {
                        static_cast<int>(bullet.position.y * 100/mapWidth) - 1,
                        static_cast<int>(bullet.position.x * 100/mapHeight) - 1,
                        2, 2
                    };
                    SDL_RenderFillRect(renderer, &bulletRect);
                }
            }
        }
    }
};

int main() {
    Game game;
    if (game.initialize()) {
        game.run();
    }
    return 0;
}