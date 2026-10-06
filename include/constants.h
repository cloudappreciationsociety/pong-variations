#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

#include "raylib.h"

namespace Constants {

constexpr int32_t SCREEN_WIDTH = 1600;
constexpr int32_t SCREEN_HEIGHT = 900;
constexpr int32_t TARGET_FPS = 60;
constexpr Vector2 SCREEN_CENTER = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
constexpr Vector2 SCREEN_SIZE = {
    static_cast<float>(SCREEN_WIDTH),
    static_cast<float>(SCREEN_HEIGHT)
};

constexpr static float MAP_X_LEFT_PADDING = 30.0f;
constexpr static float MAP_X_RIGHT_PADDING = 30.0f;

constexpr static float MAP_Y_TOP_PADDING = 70.0f;
constexpr static float MAP_Y_BOTTOM_PADDING = 20.0f;

constexpr Rectangle MAP_RECT = {
    MAP_X_LEFT_PADDING,
    MAP_Y_TOP_PADDING,
    SCREEN_WIDTH - MAP_X_LEFT_PADDING - MAP_X_RIGHT_PADDING,
    SCREEN_HEIGHT - MAP_Y_TOP_PADDING - MAP_Y_BOTTOM_PADDING
};

constexpr static float GOAL_X_OFFSET = 100.0f;
constexpr float RED_GOAL_X = GOAL_X_OFFSET;
constexpr float BLUE_GOAL_X = SCREEN_WIDTH - GOAL_X_OFFSET;

constexpr uint32_t UI_SCORE_X_OFFSET = 250.0f;

} // namespace Constants

#endif