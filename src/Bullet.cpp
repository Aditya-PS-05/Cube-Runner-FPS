#include "Bullet.h"
#include "Vector2D.h"
#include <string>

Bullet::Bullet(const Vector2D& pos, const Vector2D& dir, float spd, bool bot)
    : position(pos), direction(dir), speed(spd), active(true), isBot(bot) {
}

void Bullet::update(float deltaTime) {
    position = position + direction * (speed * deltaTime);
}
