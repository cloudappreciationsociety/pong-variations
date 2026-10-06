#include "object.h"

#include <iostream>

#include "constants.h"
#include "lib.h"

uint32_t Object::idCounter = 0;

Object::Object(const std::string& texturePath) :
    position(Constants::SCREEN_CENTER),
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

bool PhysicsObject::areColliding(const PhysicsObject& a, const PhysicsObject& b) {
    float xDistance = fabs(a.position.x - b.position.x) - ((a.size.x + b.size.x) / 2.0f);
    float yDistance = fabs(a.position.y - b.position.y) - ((a.size.y + b.size.y) / 2.0f);

    return xDistance < 0.0f && yDistance < 0.0f;
}

PhysicsObject::PhysicsObject(const std::string& texturePath) : Object(texturePath) {}

void PhysicsObject::update(float deltaTime) {
    this->position.x = clamp(
        this->position.x + this->velocity.x * deltaTime,
        Constants::MAP_RECT.x + this->size.x / 2.0f,
        Constants::MAP_RECT.x + Constants::MAP_RECT.width - this->size.x / 2.0f
    );

    this->position.y = clamp(
        this->position.y + this->velocity.y * deltaTime,
        Constants::MAP_RECT.y + this->size.y / 2.0f,
        Constants::MAP_RECT.y + Constants::MAP_RECT.height - this->size.y / 2.0f
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
        this->position.y <= Constants::MAP_RECT.y + this->size.y / 2.0f
        || this->position.y
            >= Constants::MAP_RECT.y + Constants::MAP_RECT.height - this->size.y / 2.0f
    );
}

// TODO: Add an epsilon vector?
void Puck::onCollideWithPlayer() {
    this->velocity.x *= -1;
}

void Puck::onCollideWithMapBounds() {
    this->velocity.y *= -1;
}