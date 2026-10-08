#ifndef UI_H
#define UI_H

#include <cstdint>
#include <string>

#include "raylib.h"

void drawRectangle(Vector2 center, Vector2 size, Color color);

void drawText(const char* text, Vector2 center, int32_t fontSize, Color textColor);

class TextButton {
public:
    Vector2 position = {0};
    Vector2 size = {0};
    Color backgroundColor = WHITE;
    Color hoverColor = GRAY;
    std::string text;
    Color textColor = BLACK;
    int32_t fontSize = 24;

public:
    TextButton() = default;

    bool leftClicked(void) const;
    bool isMouseInBounds(void) const;
    void render(void) const;
};

#endif