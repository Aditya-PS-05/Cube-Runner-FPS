#include "Game.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iomanip>

Game::Game() : screenWidth(1920), screenHeight(1080),
             mapWidth(16), mapHeight(16),
             FOV(3.14159f / 4.0f), depth(16.0f),
             running(false), botCount(3), 
             botRespawnTime(3.0f), gameOver(false),
             window(nullptr), renderer(nullptr), font(nullptr),
             gameState(GameState::MENU), gameTimer(GAME_DURATION),
             botsKilled(0), botSpawnTimer(BOT_SPAWN_INTERVAL) {
    initializeMap();
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
        screenWidth, screenHeight, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
        
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
    players.push_back(std::make_unique<Player>(renderer));
    spawnBots(botCount);
    
    running = true;
    return true;
}

void Game::spawnBots(int count) {
    for (int i = 0; i < count; i++) {
        float x = 2.0f + static_cast<float>(rand() % 3);
        float y = 11.0f + static_cast<float>(rand() % 3);
        
        while (map[static_cast<int>(x) * mapWidth + static_cast<int>(y)] == '#') {
            x = 2.0f + static_cast<float>(rand() % 3);
            y = 11.0f + static_cast<float>(rand() % 3);
        }
        
        auto bot = std::make_unique<Player>(renderer, x, y, false, true);
        if (bot) {
            players.push_back(std::move(bot));
        }
    }
}

void Game::handleInput(float deltaTime) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (gameState) {
                case GameState::MENU:
                    if (event.key.keysym.sym == SDLK_1) {
                        gameState = GameState::PLAYING;
                        restart();
                    }
                    else if (event.key.keysym.sym == SDLK_2) {
                        gameState = GameState::RULES;
                    }
                    else if (event.key.keysym.sym == SDLK_q) {
                        running = false;
                    }
                    break;
                    
                case GameState::RULES:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        gameState = GameState::MENU;
                    }
                    break;
                    
                case GameState::PLAYING:
                    if (event.key.keysym.sym == SDLK_p) {
                        gameState = GameState::PAUSED;
                    }
                    else if (event.key.keysym.sym == SDLK_k) {
                        players[0]->shoot();
                    }
                    else if (event.key.keysym.sym == SDLK_q) {
                        gameState = GameState::PAUSED;
                    }
                    break;
                    
                case GameState::PAUSED:
                    if (event.key.keysym.sym == SDLK_p) {
                        gameState = GameState::PLAYING;
                    }
                    else if (event.key.keysym.sym == SDLK_m) {
                        gameState = GameState::MENU;
                    }
                    break;
                    
                case GameState::GAME_OVER:
                    if (event.key.keysym.sym == SDLK_r) {
                        restart();
                        gameState = GameState::MENU;
                    }
                    break;
                    
                case GameState::QUIT_CONFIRM:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        gameState = GameState::PLAYING;
                    }
                    else if (event.key.keysym.sym == SDLK_m) {
                        gameState = GameState::MENU;
                    }
                    break;
            }
        }
    }
    
    // Only process movement if in PLAYING state
    if (gameState == GameState::PLAYING) {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        auto& player = players[0];
        float speed = 5.0f;
        float rotationSpeed = 2.0f;  // Reduced from 0.75f * speed to 2.0f
        
        if (state[SDL_SCANCODE_ESCAPE]) running = false;
        
        // Support both WASD and arrow keys with slower rotation
        if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) 
            player->angle -= rotationSpeed * deltaTime;
        if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) 
            player->angle += rotationSpeed * deltaTime;
        
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
}

void Game::update(float deltaTime) {
    if (gameState != GameState::PLAYING) return;

    // Update game timer
    gameTimer -= deltaTime;
    botSpawnTimer -= deltaTime;

    // Check win conditions
    if (gameTimer <= 0 || botsKilled >= BOTS_TO_WIN) {
        gameState = GameState::GAME_OVER;
        return;
    }

    // Spawn new bot every interval
    if (botSpawnTimer <= 0) {
        spawnBots(1);
        botSpawnTimer = BOT_SPAWN_INTERVAL;
    }

    // Update player
    if (players[0]->isDead()) {
        gameState = GameState::GAME_OVER;
        return;
    }
    
    players[0]->update(deltaTime, map, mapWidth);

    // Remove dead bots and update active ones
    players.erase(
        std::remove_if(players.begin() + 1, players.end(),
            [](const std::unique_ptr<Player>& bot) { return bot->isDead(); }),
        players.end()
    );

    // Update remaining bots
    for (size_t i = 1; i < players.size(); i++) {
        if (players[i]->isBot) {
            players[i]->updateBot(deltaTime, *players[0], map, mapWidth);
        }
    }
    
    checkBulletCollisions();
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
                    float damage = shooter->isBot ? 10.0f : 34.0f;
                    target->takeDamage(damage);
                    
                    if (target->isDead() && target->isBot && shooter == players[0]) {
                        shooter->addScore(100);
                        botsKilled++;
                    }
                }
            }
        }
    }
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    switch (gameState) {
        case GameState::MENU:
            renderMenu();
            break;
            
        case GameState::RULES:
            renderRules();
            break;
            
        case GameState::PLAYING:
            renderView();
            renderMinimap();
            renderBullets();
            renderPlayers();
            renderHealthBar();
            renderTimer();
            break;
            
        case GameState::PAUSED:
            renderView();  // Show game state in background
            renderMinimap();
            renderBullets();
            renderPlayers();
            renderHealthBar();
            renderPauseScreen();
            break;
            
        case GameState::GAME_OVER:
            renderGameOver();
            break;
            
        case GameState::QUIT_CONFIRM:
            // Render game state in background
            renderView();
            renderMinimap();
            renderBullets();
            renderPlayers();
            renderHealthBar();
            renderTimer();
            renderQuitConfirm();
            break;
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
        
        // Calculate wall color based on direction and distance
        int wallX = static_cast<int>(player->position.x + sinf(rayAngle) * distanceToWall);
        int wallY = static_cast<int>(player->position.y + cosf(rayAngle) * distanceToWall);
        
        // Create different colors for walls based on their position
        Uint8 r, g, b;
        if (wallX % 2 == 0 && wallY % 2 == 0) {
            // Brown walls
            r = static_cast<Uint8>(139 * (1.0f - distanceToWall/depth));
            g = static_cast<Uint8>(69 * (1.0f - distanceToWall/depth));
            b = static_cast<Uint8>(19 * (1.0f - distanceToWall/depth));
        } else if (wallX % 2 == 0) {
            // Blue walls
            r = static_cast<Uint8>(70 * (1.0f - distanceToWall/depth));
            g = static_cast<Uint8>(130 * (1.0f - distanceToWall/depth));
            b = static_cast<Uint8>(180 * (1.0f - distanceToWall/depth));
        } else if (wallY % 2 == 0) {
            // Purple walls
            r = static_cast<Uint8>(147 * (1.0f - distanceToWall/depth));
            g = static_cast<Uint8>(112 * (1.0f - distanceToWall/depth));
            b = static_cast<Uint8>(219 * (1.0f - distanceToWall/depth));
        } else {
            // Gray walls
            r = static_cast<Uint8>(128 * (1.0f - distanceToWall/depth));
            g = static_cast<Uint8>(128 * (1.0f - distanceToWall/depth));
            b = static_cast<Uint8>(128 * (1.0f - distanceToWall/depth));
        }
        
        // Draw wall
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer, x, ceiling, x, floor);
        
        // Draw floor (darker brown)
        SDL_SetRenderDrawColor(renderer, 40, 20, 0, 255);
        SDL_RenderDrawLine(renderer, x, floor, x, screenHeight);
        
        // Draw ceiling (dark blue)
        SDL_SetRenderDrawColor(renderer, 0, 20, 40, 255);
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
            if (map[y * mapWidth + x] == '#') {
                // Create different colors for walls in minimap
                if (x % 2 == 0 && y % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);  // Brown
                } else if (x % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 70, 130, 180, 255); // Blue
                } else if (y % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 147, 112, 219, 255); // Purple
                } else {
                    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray
                }
            } else {
                SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255); // Dark gray for floor
            }
            
            SDL_Rect rect = {x * cellSize, y * cellSize, cellSize - 1, cellSize - 1};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    
    // Render players on minimap
    for (const auto& player : players) {
        if (player->isBot) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red for bots
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for player
        }
        
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
                if (player->isBot) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red for bot bullets
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  // Yellow for player bullets
                }
                SDL_Rect bulletRect = {
                    static_cast<int>(bullet.position.y * 100/mapWidth) - 1,
                    static_cast<int>(bullet.position.x * 100/mapHeight) - 1,
                    3, 3  // Slightly larger bullets for better visibility
                };
                SDL_RenderFillRect(renderer, &bulletRect);
            }
        }
    }
}

void Game::renderPlayers() {
    for (const auto& player : players) {
        if (player.get() != players[0].get()) {
            player->render(renderer, *players[0], FOV, map, mapWidth, screenWidth, screenHeight);
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

void Game::renderTimer() {
    int minutes = static_cast<int>(gameTimer) / 60;
    int seconds = static_cast<int>(gameTimer) % 60;
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;
    
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* timerSurface = TTF_RenderText_Solid(font, ss.str().c_str(), textColor);
    if (timerSurface) {
        SDL_Texture* timerTexture = SDL_CreateTextureFromSurface(renderer, timerSurface);
        SDL_Rect timerRect = {
            screenWidth - timerSurface->w - 20,  // Position in top-right with 20px margin
            10, 
            timerSurface->w, 
            timerSurface->h
        };
        SDL_RenderCopy(renderer, timerTexture, NULL, &timerRect);
        SDL_DestroyTexture(timerTexture);
        SDL_FreeSurface(timerSurface);
    }
}

void Game::renderGameOver() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect fullScreen = {0, 0, screenWidth, screenHeight};
    SDL_RenderFillRect(renderer, &fullScreen);

    SDL_Color textColor = {255, 255, 255, 255};
    std::string gameOverText;
    if (players[0]->isDead()) {
        gameOverText = "GAME OVER - You Died!";
    } else if (botsKilled >= BOTS_TO_WIN) {
        gameOverText = "VICTORY - You killed 10 bots!";
    } else if (gameTimer <= 0) {
        gameOverText = "VICTORY - You survived 2 minutes!";
    }

    // Render game over text and stats
    SDL_Surface* gameOverSurface = TTF_RenderText_Solid(font, gameOverText.c_str(), textColor);
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, 
        ("Bots Killed: " + std::to_string(botsKilled)).c_str(), textColor);
    SDL_Surface* timeSurface = TTF_RenderText_Solid(font,
        ("Time Survived: " + std::to_string(static_cast<int>(GAME_DURATION - gameTimer)) + "s").c_str(), textColor);
    SDL_Surface* restartSurface = TTF_RenderText_Solid(font, "Press R to Restart", textColor);

    if (gameOverSurface && scoreSurface && timeSurface && restartSurface) {
        SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_Texture* timeTexture = SDL_CreateTextureFromSurface(renderer, timeSurface);
        SDL_Texture* restartTexture = SDL_CreateTextureFromSurface(renderer, restartSurface);

        SDL_Rect gameOverRect = {screenWidth/2 - 100, screenHeight/2 - 60, 200, 40};
        SDL_Rect scoreRect = {screenWidth/2 - 100, screenHeight/2, 200, 40};
        SDL_Rect timeRect = {screenWidth/2 - 100, screenHeight/2 + 60, 200, 40};
        SDL_Rect restartRect = {screenWidth/2 - 100, screenHeight/2 + 120, 200, 40};

        SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);
        SDL_RenderCopy(renderer, restartTexture, NULL, &restartRect);

        SDL_DestroyTexture(gameOverTexture);
        SDL_DestroyTexture(scoreTexture);
        SDL_DestroyTexture(timeTexture);
        SDL_DestroyTexture(restartTexture);
    }

    SDL_FreeSurface(gameOverSurface);
    SDL_FreeSurface(scoreSurface);
    SDL_FreeSurface(timeSurface);
    SDL_FreeSurface(restartSurface);
}

void Game::restart() {
    gameTimer = GAME_DURATION;
    botsKilled = 0;
    botSpawnTimer = BOT_SPAWN_INTERVAL;
    gameOver = false;
    players.clear();
    
    // Initialize player with renderer
    auto player = std::make_unique<Player>(renderer, 14.7f, 5.09f, true, false);
    players.push_back(std::move(player));
    
    // Initialize bots
    spawnBots(botCount);
    
    gameState = GameState::PLAYING;  // Set state to PLAYING
}

void Game::renderMenu() {
    SDL_Color textColor = {255, 255, 255, 255};
    
    // Render title and options
    SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Shadow Ops: Tactical Arena", textColor);
    SDL_Surface* playSurface = TTF_RenderText_Solid(font, "1. Start Game", textColor);
    SDL_Surface* rulesSurface = TTF_RenderText_Solid(font, "2. Game Rules", textColor);
    SDL_Surface* quitSurface = TTF_RenderText_Solid(font, "Q. Quit Game", textColor);  // Add quit option
    
    if (titleSurface && playSurface && rulesSurface && quitSurface) {
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Texture* playTexture = SDL_CreateTextureFromSurface(renderer, playSurface);
        SDL_Texture* rulesTexture = SDL_CreateTextureFromSurface(renderer, rulesSurface);
        SDL_Texture* quitTexture = SDL_CreateTextureFromSurface(renderer, quitSurface);
        
        SDL_Rect titleRect = {screenWidth/2 - 200, screenHeight/4, 400, 60};
        SDL_Rect playRect = {screenWidth/2 - 100, screenHeight/2, 200, 40};
        SDL_Rect rulesRect = {screenWidth/2 - 100, screenHeight/2 + 60, 200, 40};
        SDL_Rect quitRect = {screenWidth/2 - 100, screenHeight/2 + 120, 200, 40};  // Position quit option
        
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_RenderCopy(renderer, playTexture, NULL, &playRect);
        SDL_RenderCopy(renderer, rulesTexture, NULL, &rulesRect);
        SDL_RenderCopy(renderer, quitTexture, NULL, &quitRect);
        
        SDL_DestroyTexture(titleTexture);
        SDL_DestroyTexture(playTexture);
        SDL_DestroyTexture(rulesTexture);
        SDL_DestroyTexture(quitTexture);
    }
    
    SDL_FreeSurface(titleSurface);
    SDL_FreeSurface(playSurface);
    SDL_FreeSurface(rulesSurface);
    SDL_FreeSurface(quitSurface);
}

void Game::renderRules() {
    SDL_Color textColor = {255, 255, 255, 255};
    
    // Load different font sizes
    TTF_Font* titleFont = TTF_OpenFont("../assets/fonts/Arial.TTF", 48);  // Larger for title
    TTF_Font* headingFont = TTF_OpenFont("../assets/fonts/Arial.TTF", 32); // For section headings
    TTF_Font* textFont = TTF_OpenFont("../assets/fonts/Arial.TTF", 24);    // For regular text
    
    if (!titleFont || !headingFont || !textFont) {
        std::cout << "Font loading failed: " << TTF_GetError() << std::endl;
        return;
    }

    // Title
    SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, "Game Rules", textColor);
    if (titleSurface) {
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Rect titleRect = {
            screenWidth/2 - titleSurface->w/2,  // Center horizontally
            50,                                 // Top margin
            titleSurface->w,                    // Use actual text width
            titleSurface->h                     // Use actual text height
        };
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_DestroyTexture(titleTexture);
        SDL_FreeSurface(titleSurface);
    }

    // Controls section
    SDL_Surface* controlsHeadingSurface = TTF_RenderText_Blended(headingFont, "Controls:", textColor);
    if (controlsHeadingSurface) {
        SDL_Texture* controlsTexture = SDL_CreateTextureFromSurface(renderer, controlsHeadingSurface);
        SDL_Rect controlsRect = {
            screenWidth/2 - controlsHeadingSurface->w/2,
            150,
            controlsHeadingSurface->w,
            controlsHeadingSurface->h
        };
        SDL_RenderCopy(renderer, controlsTexture, NULL, &controlsRect);
        SDL_DestroyTexture(controlsTexture);
        SDL_FreeSurface(controlsHeadingSurface);
    }

    // Control items
    const char* controls[] = {
        "WASD or Arrow Keys - Move",
        "Mouse - Aim",
        "Left Click - Shoot",
        "ESC - Pause game"
    };

    int yPos = 200;  // Starting Y position for controls
    for (const char* control : controls) {
        SDL_Surface* textSurface = TTF_RenderText_Blended(textFont, control, textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {
                screenWidth/2 - textSurface->w/2,
                yPos,
                textSurface->w,
                textSurface->h
            };
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
            yPos += textSurface->h + 10;  // Add spacing between lines
        }
    }

    // Objectives section
    SDL_Surface* objHeadingSurface = TTF_RenderText_Blended(headingFont, "Objectives:", textColor);
    if (objHeadingSurface) {
        SDL_Texture* objTexture = SDL_CreateTextureFromSurface(renderer, objHeadingSurface);
        SDL_Rect objRect = {
            screenWidth/2 - objHeadingSurface->w/2,
            yPos + 20,  // Add extra spacing before new section
            objHeadingSurface->w,
            objHeadingSurface->h
        };
        SDL_RenderCopy(renderer, objTexture, NULL, &objRect);
        SDL_DestroyTexture(objTexture);
        SDL_FreeSurface(objHeadingSurface);
    }

    // Objective items
    const char* objectives[] = {
        "- Eliminate all enemy bots",
        "- Avoid getting shot",
        "- Survive as long as possible"
    };

    yPos += objHeadingSurface->h + 40;  // Position for objectives
    for (const char* objective : objectives) {
        SDL_Surface* textSurface = TTF_RenderText_Blended(textFont, objective, textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {
                screenWidth/2 - textSurface->w/2,
                yPos,
                textSurface->w,
                textSurface->h
            };
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
            yPos += textSurface->h + 10;
        }
    }

    // Back instruction
    SDL_Surface* backSurface = TTF_RenderText_Blended(textFont, "Press ESC to return to menu", textColor);
    if (backSurface) {
        SDL_Texture* backTexture = SDL_CreateTextureFromSurface(renderer, backSurface);
        SDL_Rect backRect = {
            screenWidth/2 - backSurface->w/2,
            screenHeight - backSurface->h - 30,  // Position at bottom with margin
            backSurface->w,
            backSurface->h
        };
        SDL_RenderCopy(renderer, backTexture, NULL, &backRect);
        SDL_DestroyTexture(backTexture);
        SDL_FreeSurface(backSurface);
    }

    // Clean up fonts
    TTF_CloseFont(titleFont);
    TTF_CloseFont(headingFont);
    TTF_CloseFont(textFont);
}

void Game::renderPauseScreen() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect fullScreen = {0, 0, screenWidth, screenHeight};
    SDL_RenderFillRect(renderer, &fullScreen);
    
    SDL_Color textColor = {255, 255, 255, 255};
    const char* menuItems[] = {
        "PAUSED",
        "P - Resume Game",
        "M - Return to Main Menu"
    };
    
    int yPos = screenHeight/2 - 100;
    for (const char* item : menuItems) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, item, textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {
                screenWidth/2 - textSurface->w/2,
                yPos,
                textSurface->w,
                textSurface->h
            };
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
            yPos += 50;
        }
    }
}

void Game::renderQuitConfirm() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect fullScreen = {0, 0, screenWidth, screenHeight};
    SDL_RenderFillRect(renderer, &fullScreen);
    
    SDL_Color textColor = {255, 255, 255, 255};
    const char* menuItems[] = {
        "Return to Game?",
        "ESC - Resume Game",
        "M - Return to Main Menu"  // Removed quit option
    };
    
    int yPos = screenHeight/2 - 100;
    for (const char* item : menuItems) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, item, textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {
                screenWidth/2 - textSurface->w/2,
                yPos,
                textSurface->w,
                textSurface->h
            };
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
            SDL_FreeSurface(textSurface);
            yPos += 50;
        }
    }
}
