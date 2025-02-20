#pragma once
#include "Vector2D.h"
#include <string>

class Bullet {
public:
    Vector2D position;
    Vector2D direction;
    float speed;
    bool active;
    bool isBot;
    
    Bullet(const Vector2D& pos, const Vector2D& dir, float spd = 10.0f, bool bot = false);
    void update(float deltaTime);
};
