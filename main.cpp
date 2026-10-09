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
#include <vector>

#include "constants.h"
#include "lib.h"
#include "object.h"
#include "ui.h"

enum class GameConfig { SINGLEPLAYER, MULTIPLAYER };
enum class GameStatus { IN_MENU, STARTING, IN_PROGRESS, ENDED };

AppStatus gAppStatus = AppStatus::RUNNING;
GameStatus gGameStatus = GameStatus::IN_MENU;
GameConfig gGameConfig = GameConfig::MULTIPLAYER;

float gPreviousTimestampSec = 0.0f;
float gRoundStartTime = 0.0f;
float gRoundElapsedTime = 0.0f;

Color gBackgroundColor = BLACK;

PlayerCharacter* gRedPlayer = nullptr;
PlayerCharacter* gBluePlayer = nullptr;

Object* gRinkBg = nullptr;
Object* gMenuBg = nullptr;
Object* gLogo = nullptr;

// Displayed next to blue's score when blue is controlled by AI
Object* gAiIcon = nullptr;
// Displayed next to round timer
Object* gPuckCountBgIcon = nullptr;

Music gMenuMusic;
Music gRoundMusic;
Sound gHitSound;
Sound gExplosionSound;

float gRoundCountdown = constants::ROUND_COUNTDOWN_SEC;
int32_t gRedScore = 0;
int32_t gBlueScore = 0;

std::random_device randomDevice;
std::mt19937 generator(randomDevice());
std::uniform_real_distribution<float> puck_rng(0.0f, 1.0f);

uint32_t gCurrentPuckCount = 0;
uint32_t gTotalPuckCount = 1;

TextButton gBackToMenuButton;
TextButton gPlayButton;
TextButton gQuitButton;

void spawnPuck(void) {
    std::unique_ptr<Puck> puck;
    // Only spawn explosive pucks when there's more than one puck in play
    if (gCurrentPuckCount > 1 && puck_rng(generator) < 0.25f) {
        puck =
            std::unique_ptr<ExplosivePuck>(new ExplosivePuck("assets/textures/explosive-puck.png"));
        puck->size = {75.0f, 75.0f};
    } else {
        puck = std::unique_ptr<Puck>(new Puck("assets/textures/puck.png"));
        puck->size = {50.0f, 50.0f};
    }

    float speed = (450.0f * puck_rng(generator)) + 450.0f;

    float positionY =
        constants::MAP_RECT.y + (constants::MAP_RECT.height / 2.0f * puck_rng(generator));

    float angle = (120.0f * puck_rng(generator) - 60.0f) * PI / 180.0f;
    if (puck_rng(generator) < 0.5f) {
        angle += PI;
    }

    puck->position.y = positionY;
    puck->velocity.x = speed * std::cos(angle);
    puck->velocity.y = speed * std::sin(angle);

    Object::registry().emplace(puck->id, std::move(puck));
    ++gCurrentPuckCount;
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
        character.moveSpeed = PlayerCharacter::BASE_MOVE_SPEED * std::min(loseRatio, 5.0f);
    } else {
        character.moveSpeed = PlayerCharacter::BASE_MOVE_SPEED;
    }
}

void resetGame(void) {
    gRedScore = 0;
    gBlueScore = 0;
    gTotalPuckCount = 1;
    gGameConfig = GameConfig::MULTIPLAYER;
    gRoundCountdown = constants::ROUND_COUNTDOWN_SEC;

    gRedPlayer->position = {constants::RED_GOAL_X, constants::SCREEN_CENTER.y};
    gBluePlayer->position = {constants::BLUE_GOAL_X, constants::SCREEN_CENTER.y};

    // Delete all pucks
    if (gCurrentPuckCount > 0) {
        for (auto& pair : Object::registry()) {
            auto object = pair.second.get();
            if (Puck* p = dynamic_cast<Puck*>(object)) {
                p->markForDeletion();
            } else if (
                ExplosivePuck::Explosion* e = dynamic_cast<ExplosivePuck::Explosion*>(object)
            ) {
                e->markForDeletion();
            }
        }

        gCurrentPuckCount = 0;
    }
}

void initialize(void) {
    InitWindow(constants::SCREEN_WIDTH, constants::SCREEN_HEIGHT, "Pong");
    InitAudioDevice();

    SetTargetFPS(constants::TARGET_FPS);

// Handy macro to initialize the unique pointer, link a global variable to it,
// and add the pointer to the global object list
#define INIT_OBJECT(className, gVar, texturePath) \
    do { \
        auto ptr = std::unique_ptr<className>(new (className)((texturePath))); \
        (gVar) = ptr.get(); \
        Object::registry().emplace((gVar)->id, std::move(ptr)); \
    } while (0)

    gRinkBg = Object::create(new Object("assets/textures/rink.png"));
    gAiIcon = Object::create(new Object("assets/textures/ai.png"));
    gPuckCountBgIcon = Object::create(new Object("assets/textures/puck.png"));
    gRedPlayer = Object::create(new PlayerCharacter("assets/textures/red-player.png"));
    gBluePlayer = Object::create(new PlayerCharacter("assets/textures/blue-player.png"));
    gMenuBg = Object::create(new Object("assets/textures/menu-bg.jpg"));
    gLogo = Object::create(new Object("assets/textures/logo.png"));

#undef INIT_OBJECT

    // Backgrounds
    gRinkBg->position.y = constants::SCREEN_CENTER.y + constants::MAP_RECT.y / 2.0f;
    gRinkBg->size = {2.5f * gRinkBg->size.x, constants::MAP_RECT.height};

    gMenuBg->position = constants::SCREEN_CENTER;
    gMenuBg->size = constants::SCREEN_SIZE;

    gLogo->position = {constants::SCREEN_CENTER.x, constants::SCREEN_CENTER.y - 100.0f};

    // Icons
    gAiIcon->size = {50.0f, 50.0f};
    gAiIcon->position = {1500.0f, 50.0f};
    gAiIcon->shouldRender = false;

    gPuckCountBgIcon->size = {50.0f, 50.0f};
    gPuckCountBgIcon->position = {constants::SCREEN_CENTER.x + 150.0f, 50.0f};
    gPuckCountBgIcon->shouldRender = false;

    // Sets player positions, zeroes out scores, etc.
    resetGame();

    // Audio
    gMenuMusic = LoadMusicStream("assets/audio/fat-lip.mp3");
    SetMusicVolume(gMenuMusic, 0.2f);

    gRoundMusic = LoadMusicStream("assets/audio/i-hear-you-calling.mp3");
    SetMusicVolume(gRoundMusic, 0.25f);

    gHitSound = LoadSound("assets/audio/hit1.wav");
    gExplosionSound = LoadSound("assets/audio/explosion.mp3");

    // UI elements
    gBackToMenuButton.position = {constants::SCREEN_CENTER.x, 1.35f * constants::SCREEN_CENTER.y};
    gBackToMenuButton.size = {200.0f, 75.0f};
    gBackToMenuButton.backgroundColor = DARKGREEN;
    gBackToMenuButton.hoverColor = GREEN;
    gBackToMenuButton.text = "To Menu";

    gPlayButton.position = {constants::SCREEN_CENTER.x, 1.35f * constants::SCREEN_CENTER.y};
    gPlayButton.size = {200.0f, 75.0f};
    gPlayButton.backgroundColor = DARKGREEN;
    gPlayButton.hoverColor = GREEN;
    gPlayButton.text = "PLAY";

    gQuitButton.position = {constants::SCREEN_CENTER.x, 1.6f * constants::SCREEN_CENTER.y};
    gQuitButton.size = {200.0f, 75.0f};
    gQuitButton.backgroundColor = RED;
    gQuitButton.hoverColor = PINK;
    gQuitButton.text = "QUIT";
}

void processInput(void) {
    Vector2 mousePosition = GetMousePosition();

    switch (gGameStatus) {
        case (GameStatus::IN_MENU): {
            if (gPlayButton.leftClicked()) {
                resetGame();
                gGameStatus = GameStatus::STARTING;
            }

            if (gQuitButton.leftClicked()) {
                gAppStatus = AppStatus::TERMINATED;
            }

            break;
        }
        case (GameStatus::STARTING): {
            // No inputs possible
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

            // Keys nine to one to spawn nine to one pucks
            for (int32_t i = KEY_NINE; i >= KEY_ONE; --i) {
                if (IsKeyDown(i)) {
                    gTotalPuckCount = i + 1 - KEY_ONE;
                    break;
                }
            }

            break;
        }
        case (GameStatus::ENDED): {
            if (gBackToMenuButton.leftClicked()) {
                resetGame();
                gGameStatus = GameStatus::IN_MENU;
            }

            if (gQuitButton.leftClicked()) {
                gAppStatus = AppStatus::TERMINATED;
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

void updateGameMenu(void) {
    if (!IsMusicStreamPlaying(gMenuMusic)) {
        PlayMusicStream(gMenuMusic);
    }

    UpdateMusicStream(gMenuMusic);
}

void updateGameStarting(float deltaTime) {
    StopMusicStream(gMenuMusic);

    gRoundCountdown = std::max(0.0f, gRoundCountdown - deltaTime);

    if (!IsMusicStreamPlaying(gRoundMusic)) {
        PlayMusicStream(gRoundMusic);
    }

    if (gRoundCountdown == 0.0f) {
        gGameStatus = GameStatus::IN_PROGRESS;
        gRoundStartTime = static_cast<float>(GetTime());
    }

    UpdateMusicStream(gRoundMusic);
}

void updateGameInProgress(float deltaTime) {
    gRoundElapsedTime = static_cast<float>(GetTime()) - gRoundStartTime;

    // Delete pucks over our limit

    // Spawn pucks until we reach the limit
    while (gCurrentPuckCount < gTotalPuckCount) {
        spawnPuck();
    }

    for (auto& pair : Object::registry()) {
        pair.second->update(deltaTime);
    }

    // Collisions
    for (auto& pair : Object::registry()) {
        Puck* puck = dynamic_cast<Puck*>(pair.second.get());
        if (!puck) {
            continue;
        }

        // Score!
        if (puck->position.x <= constants::RED_GOAL_X) {
            gBlueScore += puck->points;
            puck->onScore();
            continue;
        } else if (puck->position.x >= constants::BLUE_GOAL_X) {
            gRedScore += puck->points;
            puck->onScore();
            continue;
        }

        if (puck->isCollidingWithMapBoundsY()) {
            puck->onCollideWithMapBounds();
        } else {
            auto redResult = PhysicsObject::checkCollision(*puck, *gRedPlayer);
            auto blueResult = PhysicsObject::checkCollision(*puck, *gBluePlayer);

            if (redResult.collided) {
                puck->onCollideWithPlayer(*gRedPlayer, redResult);
                PlaySound(gHitSound);
            } else if (blueResult.collided) {
                puck->onCollideWithPlayer(*gBluePlayer, blueResult);
                PlaySound(gHitSound);
            }
        }
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

    gAiIcon->shouldRender =
        (gGameStatus == GameStatus::IN_PROGRESS) && (gGameConfig == GameConfig::SINGLEPLAYER);
    gPuckCountBgIcon->shouldRender = gGameStatus == GameStatus::IN_PROGRESS;
    gMenuBg->shouldRender = (gGameStatus == GameStatus::IN_MENU);
    gLogo->shouldRender = gMenuBg->shouldRender;

    // Delete objects marked for deletion
    // Ugly C++11 iterate-and-erase stuff
    for (auto it = Object::registry().begin(); it != Object::registry().end();) {
        if (it->second->shouldDelete) {
            // Deleting a puck
            if (Puck* puck = dynamic_cast<Puck*>(it->second.get())) {
                --gCurrentPuckCount;
            }

            it = Object::registry().erase(it);
        } else {
            ++it;
        }
    }

    switch (gGameStatus) {
        case (GameStatus::IN_MENU): {
            updateGameMenu();
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

void renderTintedOverlay(void) {
    drawRectangle(constants::SCREEN_CENTER, constants::SCREEN_SIZE, constants::BLACK_TINT);
}

void renderScores(void) {
    // Red score
    drawText(
        TextFormat("%03i", gRedScore),
        {constants::ui::SCORE_X_OFFSET, constants::ui::SCORE_Y_POS},
        constants::ui::FONT_SIZE,
        RED
    );

    // Blue score
    drawText(
        TextFormat("%03i", gBlueScore),
        {constants::SCREEN_WIDTH - constants::ui::SCORE_X_OFFSET, constants::ui::SCORE_Y_POS},
        constants::ui::FONT_SIZE,
        BLUE
    );
}

void render(void) {
    BeginDrawing();

    ClearBackground(gBackgroundColor);

    for (const auto& pair : Object::registry()) {
        if (!pair.second->shouldRender) {
            continue;
        }

        pair.second->render();
    }

    switch (gGameStatus) {
        case (GameStatus::IN_MENU): {
            gPlayButton.render();
            gQuitButton.render();

            break;
        }
        case (GameStatus::STARTING): {
            renderTintedOverlay();
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
            renderScores();

            // Round timer
            drawText(
                TextFormat(
                    "%03i",
                    static_cast<int>(std::ceil(constants::ROUND_MAX_TIME_SEC - gRoundElapsedTime))
                ),
                {constants::SCREEN_CENTER.x, constants::ui::SCORE_Y_POS},
                constants::ui::HEADER_FONT_SIZE,
                WHITE
            );

            // Puck count
            drawText(
                TextFormat("%01i", gTotalPuckCount),
                gPuckCountBgIcon->position,
                constants::ui::FONT_SIZE,
                WHITE
            );

            break;
        }
        case (GameStatus::ENDED): {
            renderTintedOverlay();

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

            renderScores();

            gBackToMenuButton.render();
            gQuitButton.render();

            break;
        }
        default:
            break;
    }

    EndDrawing();
}

void shutdown(void) {
    Object::registry().clear();

    UnloadSound(gHitSound);
    UnloadMusicStream(gRoundMusic);
    UnloadMusicStream(gMenuMusic);

    CloseAudioDevice();
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