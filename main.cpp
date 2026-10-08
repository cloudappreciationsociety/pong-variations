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
#include "ui.h"

enum class GameConfig { SINGLEPLAYER, MULTIPLAYER };
enum class GameStatus { IN_MENU, STARTING, IN_PROGRESS, ENDED };

// =============================================================================
// App-level globals
// =============================================================================
AppStatus gAppStatus = AppStatus::RUNNING;
GameStatus gGameStatus = GameStatus::IN_MENU;
GameConfig gGameConfig = GameConfig::MULTIPLAYER;

float gPreviousTimestampSec = 0.0f;
float gRoundStartTime = 0.0f;
float gRoundElapsedTime = 0.0f;

Color gBackgroundColor = BLACK;

// std::map over std::unordered_map to guarantee rendering order so BG renders
// behind everything else
std::map<uint32_t, std::unique_ptr<Object>> gObjects;

PlayerCharacter* gRedPlayer = nullptr;
PlayerCharacter* gBluePlayer = nullptr;
Object* gRinkBg = nullptr;

// Displayed next to blue's score when blue is controlled by AI
Object* gAiIcon = nullptr;

Music gMenuMusic;
Music gRoundMusic;

float gRoundCountdown = constants::ROUND_COUNTDOWN_SEC;
uint32_t gRedScore = 0;
uint32_t gBlueScore = 0;

std::random_device randomDevice;
std::mt19937 generator(randomDevice());
std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

uint32_t gPuckCount = 0;

TextButton gBackToMenuButton;
TextButton gPlayButton;

void spawnPuck(void) {
    auto puck = std::unique_ptr<Puck>(new Puck("assets/textures/puck.png"));
    puck->size = {50.0f, 50.0f};

    float speed = (450.0f * distribution(generator)) + 450.0f;

    float positionY =
        constants::MAP_RECT.y + (constants::MAP_RECT.height / 2.0f * distribution(generator));

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
    // Always keep moving
    if (character.moveDirection.y == 0.0f) {
        character.moveDirection.y = 1.0f;
    }

    // Up-and-down motion
    if (character.position.y >= 750.0f) {
        character.moveDirection.y = -1.0f;
    } else if (character.position.y <= 200.0f) {
        character.moveDirection.y = 1.0f;
    }

    float loseRatio = gRedScore / std::max(static_cast<float>(gBlueScore), 1.0f);

    if (loseRatio >= 2.0f) {
        character.moveSpeed = PlayerCharacter::BASE_MOVE_SPEED * loseRatio;
    } else {
        character.moveSpeed = PlayerCharacter::BASE_MOVE_SPEED;
    }
}

void initialize(void) {
    InitWindow(constants::SCREEN_WIDTH, constants::SCREEN_HEIGHT, "Pong");
    InitAudioDevice();

    SetTargetFPS(constants::TARGET_FPS);

    // Game objects and textured backgrounds
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

    gRinkBg->position.y = constants::SCREEN_CENTER.y + constants::MAP_RECT.y / 2.0f;
    gRinkBg->size = {2.5f * gRinkBg->size.x, constants::MAP_RECT.height};

    gAiIcon->size = {50.0f, 50.0f};
    gAiIcon->position = {1500.0f, 50.0f};
    gAiIcon->shouldRender = false;

    gRedPlayer->position.x = constants::RED_GOAL_X;
    gBluePlayer->position.x = constants::BLUE_GOAL_X;

    // Background music
    gRoundMusic = LoadMusicStream("assets/audio/i-hear-you-calling.mp3");
    SetMusicVolume(gRoundMusic, 0.33f);

    // UI elements
    gBackToMenuButton.position = {constants::SCREEN_CENTER.x, 1.35f * constants::SCREEN_CENTER.y};
    gBackToMenuButton.size = {200.0f, 100.0f};
    gBackToMenuButton.backgroundColor = DARKGREEN;
    gBackToMenuButton.hoverColor = GREEN;
    gBackToMenuButton.text = "To Menu";

    gPlayButton.position = {constants::SCREEN_CENTER.x, 1.35f * constants::SCREEN_CENTER.y};
    gPlayButton.size = {200.0f, 100.0f};
    gPlayButton.backgroundColor = DARKGREEN;
    gPlayButton.hoverColor = GREEN;
    gPlayButton.text = "PLAY";
}

void processInput(void) {
    Vector2 mousePosition = GetMousePosition();

    switch (gGameStatus) {
        case (GameStatus::IN_MENU): {
            if (gPlayButton.leftClicked()) {
                gGameStatus = GameStatus::STARTING;
                gRoundCountdown = constants::ROUND_COUNTDOWN_SEC;
            }

            break;
        }
        case (GameStatus::STARTING): {
            // No inputs
            break;
        }
        case (GameStatus::IN_PROGRESS): {
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
                gBluePlayer->moveSpeed = PlayerCharacter::BASE_MOVE_SPEED;

                if (IsKeyDown(KEY_UP)) {
                    gBluePlayer->moveDirection.y = -1.0f;
                } else if (IsKeyDown(KEY_DOWN)) {
                    gBluePlayer->moveDirection.y = 1.0f;
                }
            } else {
                runAI(*gBluePlayer);
            }

            break;
        }
        case (GameStatus::ENDED): {
            if (gBackToMenuButton.leftClicked()) {
                gGameStatus = GameStatus::IN_MENU;
            }

            break;
        }
        default:
            break;
    }

    if (IsKeyDown(KEY_Q) || WindowShouldClose()) {
        gAppStatus = AppStatus::TERMINATED;
    }
}

void updateGameStarting(float deltaTime) {
    gRedScore = 0;
    gBlueScore = 0;

    gRoundCountdown = std::max(0.0f, gRoundCountdown - deltaTime);

    if (gRoundCountdown == 0.0f) {
        gGameStatus = GameStatus::IN_PROGRESS;
        gRoundStartTime = static_cast<float>(GetTime());

        PlayMusicStream(gRoundMusic);
    }
}

void updateGameInProgress(float deltaTime) {
    gRoundElapsedTime = static_cast<float>(GetTime()) - gRoundStartTime;

    gAiIcon->shouldRender = gGameConfig == GameConfig::SINGLEPLAYER;

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
        if (puck->position.x <= constants::RED_GOAL_X) {
            ++gBlueScore;
            puck->markForDeletion();
            --gPuckCount;
            continue;
        } else if (puck->position.x >= constants::BLUE_GOAL_X) {
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

    UpdateMusicStream(gRoundMusic);

    if (gRoundElapsedTime >= constants::ROUND_MAX_TIME_SEC) {
        StopMusicStream(gRoundMusic);
        gGameStatus = GameStatus::ENDED;
    }
}

void update(void) {
    // Compute delta time
    float nowSec = static_cast<float>(GetTime());
    float deltaTime = nowSec - gPreviousTimestampSec;
    gPreviousTimestampSec = nowSec;

    // Delete objects marked for deletion
    // Ugly C++11 iterate-and-erase stuff
    for (auto it = gObjects.begin(); it != gObjects.end();) {
        if (it->second->shouldDelete) {
            it = gObjects.erase(it);
        } else {
            ++it;
        }
    }

    switch (gGameStatus) {
        case (GameStatus::IN_MENU): {
            break;
        }
        case (GameStatus::STARTING): {
            updateGameStarting(deltaTime);
            break;
        }
        case (GameStatus::IN_PROGRESS): {
            updateGameInProgress(deltaTime);
            break;
        }
        case (GameStatus::ENDED): {
            break;
        }
        default:
            break;
    }
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

    switch (gGameStatus) {
        case (GameStatus::IN_MENU): {
            drawRectangle(constants::SCREEN_CENTER, constants::SCREEN_SIZE, WHITE);
            gPlayButton.render();

            break;
        }
        case (GameStatus::STARTING): {
            drawRectangle(constants::SCREEN_CENTER, constants::SCREEN_SIZE, constants::BLACK_TINT);
            drawText(
                TextFormat(
                    "Match begins in %i seconds...",
                    static_cast<int32_t>(std::ceil(gRoundCountdown))
                ),
                constants::SCREEN_CENTER,
                constants::ui::HEADER_FONT_SIZE,
                WHITE
            );

            break;
        }
        case (GameStatus::IN_PROGRESS): {
            drawText(
                TextFormat("%03i", gRedScore),
                {constants::ui::SCORE_X_OFFSET, constants::ui::SCORE_Y_POS},
                constants::ui::FONT_SIZE,
                RED
            );

            drawText(
                TextFormat("%03i", gBlueScore),
                {constants::SCREEN_WIDTH - constants::ui::SCORE_X_OFFSET,
                 constants::ui::SCORE_Y_POS},
                constants::ui::FONT_SIZE,
                BLUE
            );

            drawText(
                TextFormat(
                    "%03i",
                    static_cast<int>(std::ceil(constants::ROUND_MAX_TIME_SEC - gRoundElapsedTime))
                ),
                {constants::SCREEN_CENTER.x, constants::ui::SCORE_Y_POS},
                constants::ui::HEADER_FONT_SIZE,
                WHITE
            );

            break;
        }
        case (GameStatus::ENDED): {
            drawRectangle(constants::SCREEN_CENTER, constants::SCREEN_SIZE, constants::BLACK_TINT);

            const char* roundOutcomeText;
            Color textColor = WHITE;
            if (gRedScore > gBlueScore) {
                roundOutcomeText = "Red wins!";
                textColor = RED;
            } else if (gBlueScore > gRedScore) {
                roundOutcomeText = "Blue wins!";
                textColor = BLUE;
            } else {
                roundOutcomeText = "Tie! Everyone's a loser!";
            }

            drawText(
                roundOutcomeText,
                constants::SCREEN_CENTER,
                constants::ui::HEADER_FONT_SIZE,
                textColor
            );

            gBackToMenuButton.render();

            break;
        }
        default:
            break;
    }

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