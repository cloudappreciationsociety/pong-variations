/**
* Author: Alan Chen
* Assignment: Pong Clone
* Date due: 10/14/2026
*
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include <map>
#include <memory>
#include <random>

#include "constants.h"
#include "lib.h"
#include "object.h"

enum class GameConfig { SINGLEPLAYER, MULTIPLAYER };

// =============================================================================
// App-level globals
// =============================================================================
AppStatus gAppStatus = AppStatus::RUNNING;
float gPreviousTimestampSec = 0.0f;
float gGameStartTime = 0.0f;
float gGameElapsedTime = 0.0f;
Color gBackgroundColor = BLACK;
GameConfig gGameConfig = GameConfig::MULTIPLAYER;

// std::map over std::unordered_map to guarantee rendering order so BG renders
// behind everything else
std::map<uint32_t, std::unique_ptr<Object>> gObjects;

PlayerCharacter* gRedPlayer = nullptr;
PlayerCharacter* gBluePlayer = nullptr;
Object* gRinkBg = nullptr;

// Displayed next to blue's score when blue is controlled by AI
Object* gAiIcon = nullptr;

Music gBackgroundMusic;

uint32_t gRedScore = 0;
uint32_t gBlueScore = 0;

std::random_device randomDevice;
std::mt19937 generator(randomDevice());
std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

uint32_t gPuckCount = 0;

void spawnPuck(void) {
    auto puck = std::unique_ptr<Puck>(new Puck("assets/textures/puck.png"));
    puck->size = {50.0f, 50.0f};

    float speed = (450.0f * distribution(generator)) + 450.0f;

    float positionY =
        Constants::MAP_RECT.y + (Constants::MAP_RECT.height / 2.0f * distribution(generator));

    float angle = (120.0f * distribution(generator) - 60.0f) * PI / 180.0f;
    if (distribution(generator) > 0.5f) {
        angle += PI;
    }

    puck->position.y = positionY;
    puck->velocity.x = speed * std::cos(angle);
    puck->velocity.y = speed * std::sin(angle);

    gObjects.emplace(puck->id, std::move(puck));
    ++gPuckCount;
}

void runAI(PlayerCharacter& character) {
    float nowSeconds = static_cast<float>(GetTime());

    if (character.moveDirection.y == 0.0f) {
        character.moveDirection.y = 1.0f;
    }

    if (character.position.y >= 750.0f) {
        character.moveDirection.y = -1.0f;
    } else if (character.position.y <= 200.0f) {
        character.moveDirection.y = 1.0f;
    }
}

void initialize(void) {
    InitWindow(Constants::SCREEN_WIDTH, Constants::SCREEN_HEIGHT, "Pong");
    InitAudioDevice();

    SetTargetFPS(Constants::TARGET_FPS);

    auto rinkBg = std::unique_ptr<Object>(new Object("assets/textures/rink.png"));
    auto aiIcon = std::unique_ptr<Object>(new Object("assets/textures/ai.png"));
    auto red =
        std::unique_ptr<PlayerCharacter>(new PlayerCharacter("assets/textures/red-player.png"));
    auto blue =
        std::unique_ptr<PlayerCharacter>(new PlayerCharacter("assets/textures/blue-player.png"));

    gRinkBg = rinkBg.get();
    gAiIcon = aiIcon.get();
    gRedPlayer = red.get();
    gBluePlayer = blue.get();

    gObjects.emplace(rinkBg->id, std::move(rinkBg));
    gObjects.emplace(aiIcon->id, std::move(aiIcon));
    gObjects.emplace(red->id, std::move(red));
    gObjects.emplace(blue->id, std::move(blue));

    gRinkBg->position.y = Constants::SCREEN_CENTER.y + Constants::MAP_RECT.y / 2.0f;
    gRinkBg->size = {2.5f * gRinkBg->size.x, Constants::MAP_RECT.height};

    gAiIcon->size = {50.0f, 50.0f};
    gAiIcon->position = {1500.0f, 50.0f};
    gAiIcon->shouldRender = false;

    gRedPlayer->position.x = Constants::RED_GOAL_X;
    gBluePlayer->position.x = Constants::BLUE_GOAL_X;

    spawnPuck();

    // Background music
    gBackgroundMusic = LoadMusicStream("assets/audio/i-hear-you-calling.mp3");
    SetMusicVolume(gBackgroundMusic, 0.33f);
    PlayMusicStream(gBackgroundMusic);

    gGameStartTime = static_cast<float>(GetTime());
}

void processInput(void) {
    gRedPlayer->moveDirection = {0.0f, 0.0f};

    if (IsKeyDown(KEY_W)) {
        gRedPlayer->moveDirection.y = -1.0f;
    } else if (IsKeyDown(KEY_S)) {
        gRedPlayer->moveDirection.y = 1.0f;
    }

    // Switch modes
    if (IsKeyPressed(KEY_T)) {
        if (gGameConfig == GameConfig::MULTIPLAYER) {
            gGameConfig = GameConfig::SINGLEPLAYER;
        } else {
            gGameConfig = GameConfig::MULTIPLAYER;
        }
    }

    if (gGameConfig == GameConfig::MULTIPLAYER) {
        gBluePlayer->moveDirection = {0.0f, 0.0f};

        if (IsKeyDown(KEY_UP)) {
            gBluePlayer->moveDirection.y = -1.0f;
        } else if (IsKeyDown(KEY_DOWN)) {
            gBluePlayer->moveDirection.y = 1.0f;
        }
    } else {
        runAI(*gBluePlayer);
    }

    if (IsKeyDown(KEY_Q) || WindowShouldClose()) {
        gAppStatus = AppStatus::TERMINATED;
    }
}

void update(void) {
    // Compute delta time
    float nowSec = static_cast<float>(GetTime());
    float deltaTime = nowSec - gPreviousTimestampSec;
    gPreviousTimestampSec = nowSec;

    gGameElapsedTime = nowSec - gGameStartTime;

    // Delete objects marked for deletion
    // Ugly C++11 iterate-and-erase stuff
    for (auto it = gObjects.begin(); it != gObjects.end();) {
        if (it->second->shouldDelete) {
            it = gObjects.erase(it);
        } else {
            ++it;
        }
    }

    gAiIcon->shouldRender = gGameConfig == GameConfig::SINGLEPLAYER;

    // Movement
    for (auto& pair : gObjects) {
        pair.second->update(deltaTime);
    }

    // Collisions
    for (auto& pair : gObjects) {
        Puck* puck = dynamic_cast<Puck*>(pair.second.get());
        if (!puck) {
            continue;
        }

        // Score!
        if (puck->position.x <= Constants::RED_GOAL_X) {
            ++gBlueScore;
            puck->markForDeletion();
            --gPuckCount;
            continue;
        } else if (puck->position.x >= Constants::BLUE_GOAL_X) {
            ++gRedScore;
            puck->markForDeletion();
            --gPuckCount;
            continue;
        }

        if (puck->isCollidingWithMapBoundsY()) {
            puck->onCollideWithMapBounds();
        } else {
            auto redResult = PhysicsObject::checkCollision(*puck, *gRedPlayer);
            auto blueResult = PhysicsObject::checkCollision(*puck, *gBluePlayer);

            if (redResult.collided) {
                puck->onCollideWithPlayer(*gRedPlayer, redResult);
            } else if (blueResult.collided) {
                puck->onCollideWithPlayer(*gBluePlayer, blueResult);
            }
        }
    }

    if (gPuckCount == 0) {
        spawnPuck();
    }

    UpdateMusicStream(gBackgroundMusic);
}

void render(void) {
    BeginDrawing();

    ClearBackground(gBackgroundColor);

    for (const auto& pair : gObjects) {
        if (!pair.second->shouldRender) {
            continue;
        }

        pair.second->render();
    }

    int32_t fontSize = 32;

    DrawText(TextFormat("%03i", gRedScore), Constants::UI_SCORE_X_OFFSET, 40, fontSize, RED);

    int32_t textWidth = MeasureText(TextFormat("%03i", gBlueScore), fontSize);
    DrawText(
        TextFormat("%03i", gBlueScore),
        Constants::SCREEN_WIDTH - Constants::UI_SCORE_X_OFFSET - textWidth,
        40,
        fontSize,
        BLUE
    );

    EndDrawing();
}

void shutdown(void) {
    gObjects.clear();

    CloseWindow();
}

int main(void) {
    initialize();

    while (gAppStatus == AppStatus::RUNNING) {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}