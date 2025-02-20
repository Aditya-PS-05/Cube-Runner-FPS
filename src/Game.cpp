#include "Game.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <sstream>

Game::Game() : screenWidth(800), screenHeight(600),
             mapWidth(16), mapHeight(16),
             FOV(3.14159f / 4.0f), depth(16.0f),
             running(false), botCount(3), 
             botRespawnTime(3.0f), gameOver(false),
             window(nullptr), renderer(nullptr), font(nullptr) {
    initializeMap();
    players.push_back(std::make_unique<Player>(nullptr));
}

Game::~Game() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Game::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cout << "TTF initialization failed: " << TTF_GetError() << std::endl;
        return false;
    }
    
    window = SDL_CreateWindow("Shadow Ops: Tactical Arena",
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

    // Load font
    font = TTF_OpenFont("../assets/fonts/Arial.TTF", 24);  // Adjust path and size as needed
    if (!font) {
        std::cout << "Font loading failed: " << TTF_GetError() << std::endl;
        return false;
    }

    // Initialize player and bots
    players[0] = std::make_unique<Player>(renderer);
    spawnBots(botCount);
    
    running = true;
    return true;
}

void Game::spawnBots(int count) {
    for (int i = 0; i < count; i++) {
        float x = 2.0f + static_cast<float>(rand() % (mapWidth - 4));
        float y = 2.0f + static_cast<float>(rand() % (mapHeight - 4));
        
        while (map[static_cast<int>(x) * mapWidth + static_cast<int>(y)] == '#') {
            x = 2.0f + static_cast<float>(rand() % (mapWidth - 4));
            y = 2.0f + static_cast<float>(rand() % (mapHeight - 4));
        }
        
        players.push_back(std::make_unique<Player>(renderer, x, y, false, true));
    }
}

void Game::handleInput(float deltaTime) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (!gameOver) {
                    players[0]->shoot();
                }
            }
        }
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_r && gameOver) {
                restart();  // Restart game when 'R' is pressed during game over
            }
        }
    }
    
    if (gameOver) return;  // Don't process movement if game is over
    
    const Uint8* state = SDL_GetKeyboardState(NULL);
    auto& player = players[0];
    float speed = 5.0f;
    
    if (state[SDL_SCANCODE_ESCAPE]) running = false;
    
    // Support both WASD and arrow keys
    if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) 
        player->angle -= speed * 0.75f * deltaTime;
    if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) 
        player->angle += speed * 0.75f * deltaTime;
    
    if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W]) {
        Vector2D newPos = player->position + Vector2D(
            sinf(player->angle) * speed * deltaTime,
            cosf(player->angle) * speed * deltaTime
        );
        if (map[static_cast<int>(newPos.x) * mapWidth + static_cast<int>(newPos.y)] != '#') {
            player->position = newPos;
        }
    }
    
    if (state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S]) {
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
    // Update player
    if (players[0]->isDead()) {
        gameOver = true;
        return;
    }
    
    // Update player
    if (!players[0]->isDead()) {
        players[0]->update(deltaTime, map, mapWidth);
    }

    // Update bots and handle respawning
    updateBots(deltaTime);
    
    // Check collisions
    checkBulletCollisions();
}

void Game::updateBots(float deltaTime) {
    for (size_t i = 1; i < players.size(); i++) {
        auto& bot = players[i];
        if (bot->isBot && !bot->isDead()) {
            // Make bots move faster and more aggressively
            bot->moveSpeed = 4.0f;  // Increased speed
            bot->updateBot(deltaTime, *players[0], map, mapWidth);
        }
    }
}

void Game::checkBulletCollisions() {
    for (auto& shooter : players) {
        for (auto& bullet : shooter->bullets) {
            if (!bullet.active) continue;
            
            for (auto& target : players) {
                if (shooter == target || target->isDead()) continue;
                
                float dx = bullet.position.x - target->position.x;
                float dy = bullet.position.y - target->position.y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if (distance < 0.5f) {
                    bullet.active = false;
                    target->takeDamage(20.0f);
                    
                    if (target->isDead()) {
                        if (shooter == players[0]) {
                            shooter->addScore(100);  // Player killed a bot
                        }
                    }
                }
            }
        }
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    renderView();
    renderMinimap();
    renderBullets();
    renderPlayers();
    renderHealthBar();

    if (gameOver) {
        renderGameOver();
    }
    
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

void Game::renderHealthBar() {
    // Draw health bar background
    SDL_Rect bgRect = {10, screenHeight - 40, 200, 20};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw current health
    float healthPercent = players[0]->health / 100.0f;
    SDL_Rect healthRect = {10, screenHeight - 40, 
                          static_cast<int>(200 * healthPercent), 20};
    
    // Color changes from green to red as health decreases
    Uint8 red = static_cast<Uint8>(255 * (1.0f - healthPercent));
    Uint8 green = static_cast<Uint8>(255 * healthPercent);
    SDL_SetRenderDrawColor(renderer, red, green, 0, 255);
    SDL_RenderFillRect(renderer, &healthRect);
}

void Game::renderGameOver() {
    // Darken the screen
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect fullScreen = {0, 0, screenWidth, screenHeight};
    SDL_RenderFillRect(renderer, &fullScreen);

    // Render game over text
    SDL_Color textColor = {255, 0, 0, 255};
    SDL_Surface* gameOverSurface = TTF_RenderText_Solid(font, "GAME OVER", textColor);
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, 
        ("Score: " + std::to_string(players[0]->getScore())).c_str(), textColor);
    SDL_Surface* restartSurface = TTF_RenderText_Solid(font, "Press R to Restart", textColor);

    if (gameOverSurface && scoreSurface && restartSurface) {
        SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_Texture* restartTexture = SDL_CreateTextureFromSurface(renderer, restartSurface);

        SDL_Rect gameOverRect = {screenWidth/2 - 100, screenHeight/2 - 60, 200, 40};
        SDL_Rect scoreRect = {screenWidth/2 - 100, screenHeight/2, 200, 40};
        SDL_Rect restartRect = {screenWidth/2 - 100, screenHeight/2 + 60, 200, 40};

        SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        SDL_RenderCopy(renderer, restartTexture, NULL, &restartRect);

        SDL_DestroyTexture(gameOverTexture);
        SDL_DestroyTexture(scoreTexture);
        SDL_DestroyTexture(restartTexture);
    }

    SDL_FreeSurface(gameOverSurface);
    SDL_FreeSurface(scoreSurface);
    SDL_FreeSurface(restartSurface);
}

void Game::restart() {
    // Reset game state
    gameOver = false;
    
    // Reset player
    players.clear();
    players.push_back(std::make_unique<Player>(renderer));
    
    // Respawn bots
    spawnBots(botCount);
}
