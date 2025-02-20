#include "Bullet.h"
#include "Vector2D.h"
#include <string>

Bullet::Bullet(Vector2D pos, Vector2D dir, float spd)
    : position(pos), direction(dir), speed(spd), active(true) {}

void Bullet::update(float deltaTime) {
    position = position + direction * (speed * deltaTime);
}