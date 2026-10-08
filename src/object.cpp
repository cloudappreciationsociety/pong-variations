#include "object.h"

#include <iostream>

#include "constants.h"
#include "lib.h"

uint32_t Object::idCounter = 0;

Object::Object(const std::string& texturePath) :
    position(constants::SCREEN_CENTER),
    id(Object::idCounter++) {
    this->texturePath = texturePath;
    this->texture = LoadTexture(this->texturePath.c_str());

    this->uvSize = {
        static_cast<float>(this->texture.width),
        static_cast<float>(this->texture.height)
    };
    this->size = this->uvSize;
}

Object::~Object() {
    UnloadTexture(this->texture);
}

Object::Object(const Object& other) : Object(other.texturePath) {
    this->position = other.position;
    this->size = other.size;
    this->rotation = other.rotation;
    this->tint = other.tint;

    this->uvOrigin = other.uvOrigin;
    this->uvSize = other.uvSize;

    this->shouldRender = other.shouldRender;
}

void Object::render(void) const {
    Rectangle textureArea = {this->uvOrigin.x, this->uvOrigin.y, this->uvSize.x, this->uvSize.y};
    Rectangle destinationArea = {this->position.x, this->position.y, this->size.x, this->size.y};

    Vector2 objectOrigin = {this->size.x / 2.0f, this->size.y / 2.0f};

    DrawTexturePro(
        this->texture,
        textureArea,
        destinationArea,
        objectOrigin,
        this->rotation,
        this->tint
    );
}

void Object::update(float deltaTime) {
    return;
}

void Object::markForDeletion(void) {
    this->shouldDelete = true;
    this->shouldRender = false;
}

// PhysicsObject implementation

PhysicsObject::CollisionResult
PhysicsObject::checkCollision(const PhysicsObject& a, const PhysicsObject& b) {
    float overlapX = ((a.size.x + b.size.x) / 2.0f) - fabs(a.position.x - b.position.x);
    float overlapY = ((a.size.y + b.size.y) / 2.0f) - fabs(a.position.y - b.position.y);

    return PhysicsObject::CollisionResult {overlapX > 0.0f && overlapY > 0.0f, overlapX, overlapY};
}

PhysicsObject::PhysicsObject(const std::string& texturePath) : Object(texturePath) {}

void PhysicsObject::update(float deltaTime) {
    this->position.x = clamp(
        this->position.x + this->velocity.x * deltaTime,
        constants::MAP_RECT.x + this->size.x / 2.0f,
        constants::MAP_RECT.x + constants::MAP_RECT.width - this->size.x / 2.0f
    );

    this->position.y = clamp(
        this->position.y + this->velocity.y * deltaTime,
        constants::MAP_RECT.y + this->size.y / 2.0f,
        constants::MAP_RECT.y + constants::MAP_RECT.height - this->size.y / 2.0f
    );
}

// PlayerCharacter implementation

PlayerCharacter::PlayerCharacter(const std::string& texturePath) :
    PhysicsObject(texturePath),
    moveSpeed(PlayerCharacter::BASE_MOVE_SPEED) {
    this->size = PlayerCharacter::BASE_SIZE;
}

void PlayerCharacter::update(float deltaTime) {
    this->velocity.x = this->moveDirection.x * this->moveSpeed;
    this->velocity.y = this->moveDirection.y * this->moveSpeed;

    PhysicsObject::update(deltaTime);
}

// Puck implementation

Puck::Puck(const std::string& texturePath) : PhysicsObject(texturePath) {}

bool Puck::isCollidingWithMapBoundsY(void) const {
    return (
        this->position.y <= constants::MAP_RECT.y + this->size.y / 2.0f
        || this->position.y
            >= constants::MAP_RECT.y + constants::MAP_RECT.height - this->size.y / 2.0f
    );
}

void Puck::onCollideWithPlayer(
    const PlayerCharacter& player,
    PhysicsObject::CollisionResult collision
) {
    // Hit the top or bottom of the player
    if (collision.overlapY < collision.overlapX) {
        float yDirection = (this->position.y < player.position.y) ? -1.0f : 1.0f;
        this->position.y += yDirection * collision.overlapY;
        this->velocity.y = yDirection * std::fabs(this->velocity.y);

        // Use the player's position relative to the screen center instead of the puck
        // because it could be possible that a puck collides with a player at their
        // goal-facing half
        float xDirection = (player.position.x < constants::SCREEN_CENTER.x) ? 1.0f : -1.0f;
        this->velocity.x = xDirection * std::fabs(this->velocity.x);
    } else { // Hit the front face of the player
        float direction = (this->position.x < player.position.x) ? -1.0f : 1.0f;
        this->position.x += direction * collision.overlapX;
        this->velocity.x = direction * std::fabs(this->velocity.x);
    }
}

void Puck::onCollideWithMapBounds() {
    bool hitTop = this->position.y <= constants::MAP_RECT.y + this->size.y / 2.0f;
    this->velocity.y = (hitTop ? 1.0f : -1.0f) * std::fabs(this->velocity.y);
}