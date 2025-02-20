#include "Game.h"
#include <iostream>
#include <chrono>
#include <cmath>

Game::Game() : screenWidth(800), screenHeight(600),
             mapWidth(16), mapHeight(16),
             FOV(3.14159f / 4.0f), depth(16.0f),
             running(false) {
    initializeMap();
    players.push_back(std::make_unique<Player>(nullptr)); // We'll set renderer later
}

Game::~Game() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Game::initialize() {
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

    // Now that we have a renderer, initialize the player model
    players[0] = std::make_unique<Player>(renderer);
    
    running = true;
    return true;
}

void Game::handleInput(float deltaTime) {
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

void Game::update(float deltaTime) {
    for (auto& player : players) {
        player->update(deltaTime, map, mapWidth);
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    renderView();
    renderMinimap();
    renderBullets();
    renderPlayers();
    
    SDL_RenderPresent(renderer);
}

void Game::run() {
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

void Game::initializeMap() {
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

void Game::renderView() {
    auto& player = players[0];
    
    for (int x = 0; x < screenWidth; x++) {
        float rayAngle = (player->angle - FOV/2.0f) + ((float)x / (float)screenWidth) * FOV;
        float distanceToWall = castRay(rayAngle, player->position);
        
        int ceiling = (float)(screenHeight/2.0) - screenHeight / ((float)distanceToWall);
        int floor = screenHeight - ceiling;
        
        Uint8 shade = (1.0f - (distanceToWall / depth)) * 255;
        
        SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
        SDL_RenderDrawLine(renderer, x, ceiling, x, floor);
        
        SDL_SetRenderDrawColor(renderer, 0, shade/2, 0, 255);
        SDL_RenderDrawLine(renderer, x, floor, x, screenHeight);
        
        SDL_SetRenderDrawColor(renderer, shade/2, shade/2, shade/2, 255);
        SDL_RenderDrawLine(renderer, x, 0, x, ceiling);
    }
}

float Game::castRay(float angle, const Vector2D& start) {
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

void Game::renderMinimap() {
    int mapSize = 100;
    int cellSize = mapSize / mapWidth;
    
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

void Game::renderBullets() {
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

void Game::renderPlayers() {
    for (const auto& player : players) {
        if (player.get() != players[0].get()) {  // Don't render the local player
            player->render(renderer, *players[0], FOV);
        }
    }
}
