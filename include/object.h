#ifndef OBJECT_H
#define OBJECT_H

#include <cstdint>
#include <map>
#include <memory>
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
    // Returns a map of all currently active objects
    // std::map over std::unordered_map to guarantee rendering order so BG renders
    // behind everything else
    static std::map<uint32_t, std::unique_ptr<Object>>& registry();

    template<typename T>
    static T* create(T* object) {
        auto ptr = std::unique_ptr<T>(object);
        T* outPtr = ptr.get();
        Object::registry().emplace(ptr->id, std::move(ptr));
        return outPtr;
    }

    Object(const std::string& texturePath);
    virtual ~Object();

    // No copy constructor or assignment operator needed
    Object(const Object& other) = delete;
    Object& operator=(const Object&) = delete;

    virtual void update(float deltaTime);
    virtual void render(void) const;
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
public:
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
    uint32_t points = 1;

public:
    Puck(const std::string& texturePath);

    bool isCollidingWithMapBoundsY(void) const;
    void
    onCollideWithPlayer(const PlayerCharacter& player, PhysicsObject::CollisionResult collision);
    void onCollideWithMapBounds(void);
    virtual void onScore(void);
};

class ExplosivePuck: public Puck {
public:
    class Explosion: public Object {
    private:
        float lifetime = 0.25f;
        float sizeScale = 0.15f;
        Vector2 baseSize = {0};
        float alpha = 255.0f / 2;

    public:
        Explosion(Vector2 position, float radius);

        void update(float deltaTime) override;
    };

public:
    float fuseTime = 5.0f;
    float explosionRadius = 300.0f;
    float explosionVelocityMultiplier = 1.5f;
    uint32_t points = 2;

private:
    void explode(void);

public:
    ExplosivePuck(const std::string& texturePath);

    void update(float deltaTime) override;
    void render(void) const override;
    void onScore(void) override;
};

#endif