#pragma once
#include "Vector2D.h"
#include <string>

class Bullet {
public:
    Vector2D position;
    Vector2D direction;
    float speed;
    bool active;
    
    Bullet(Vector2D pos, Vector2D dir, float spd = 10.0f);
    void update(float deltaTime);
};