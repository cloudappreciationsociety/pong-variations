#include "ui.h"

#include <cmath>

void drawRectangle(Vector2 center, Vector2 size, Color color) {
    DrawRectangle(center.x - size.x / 2, center.y - size.y / 2, size.x, size.y, color);
}

void drawText(const char* text, Vector2 center, int32_t fontSize, Color textColor) {
    // The spacing used in DrawText is apparently fontSize / 10: https://github.com/raysan5/raylib/blob/master/src/rtext.c#L1211
    Vector2 textBounds =
        MeasureTextEx(GetFontDefault(), text, fontSize, static_cast<float>(fontSize / 10));

    DrawText(text, center.x - textBounds.x / 2, center.y - textBounds.y / 2, fontSize, textColor);
}

bool TextButton::leftClicked(void) const {
    return this->isMouseInBounds() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool TextButton::isMouseInBounds(void) const {
    Vector2 mousePosition = GetMousePosition();

    return (
        std::fabs(mousePosition.x - this->position.x) < this->size.x / 2
        && std::fabs(mousePosition.y - this->position.y) < this->size.y / 2
    );
}

void TextButton::render(void) const {
    drawRectangle(
        this->position,
        this->size,
        this->isMouseInBounds() ? this->hoverColor : this->backgroundColor
    );
    drawText(this->text.c_str(), this->position, this->fontSize, this->textColor);
}