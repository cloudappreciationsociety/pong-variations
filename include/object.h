#ifndef OBJECT_H
#define OBJECT_H

#include <cstdint>
#include <string>

#include "raylib.h"

class Object {
private:
    static uint32_t idCounter;

private: // Fields
    Texture2D texture;
    std::string texturePath;

public: // Fields
    const uint32_t id;
    Vector2 position = {0};
    Vector2 size = {0};
    float rotation = 0.0f;
    Color tint = WHITE;

    Vector2 uvOrigin = {0};
    Vector2 uvSize = {0};

    bool shouldRender = true;
    bool shouldDelete = false;

public: // Methods
    Object(const std::string& texturePath);
    virtual ~Object();

    Object(const Object& other);

    // No assignment operator needed
    Object& operator=(const Object&) = delete;

    virtual void update(float deltaTime);
    void render(void) const;
    void markForDeletion(void);
};

class PhysicsObject: public Object {
public:
    struct CollisionResult {
        bool collided;
        float overlapX;
        float overlapY;
    };

    static CollisionResult checkCollision(const PhysicsObject& a, const PhysicsObject& b);

public:
    Vector2 velocity = {0};

public:
    PhysicsObject(const std::string& texturePath);

    virtual void update(float deltaTime) override;
};

class PlayerCharacter: public PhysicsObject {
private:
    constexpr static float BASE_MOVE_SPEED = 250.0f;
    constexpr static Vector2 BASE_SIZE = {100.0f, 150.0f};

public:
    Vector2 moveDirection = {0};
    float moveSpeed = 0.0f;

public:
    PlayerCharacter(const std::string& texturePath);

    void update(float deltaTime) override;
};

class Puck: public PhysicsObject {
public:
    Puck(const std::string& texturePath);

    bool isCollidingWithMapBoundsY(void) const;
    void
    onCollideWithPlayer(const PlayerCharacter& player, PhysicsObject::CollisionResult collision);
    void onCollideWithMapBounds();
};

#endif