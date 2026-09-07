#pragma once

#include <string>

#include <Colors.hpp>

namespace HbTK {

struct FrameStyle
{
    Colors color;
    Colors borderColors;

    int borderWidth = 0;
    int radius = 0;
    int alpha = 255;
};

struct TitleStyle
{
    Colors color;
    Colors textColors;

    int height = 24;
    int alpha = 255;

    std::string layout = "menu:min:title:max:close";
};

struct ButtonStyle
{
    Colors color;
    Colors hoverColors;
    Colors pressedColors;
    Colors borderColors;
    Colors textColors;

    int width = 24;
    int height = 24;
    int borderWidth = 0;
    int radius = 0;
    int alpha = 255;
};

struct RootStyle
{
    std::string background;
    std::string backgroundFolder;
    std::string backgroundMode = "fill";

    int backgroundTimer = 0;
};

struct MenuStyle
{
    Colors color;
    Colors textColors;
    Colors hoverColors;
    Colors selectedColors;
    Colors borderColors;

    int borderWidth = 1;
    int radius = 0;
    int itemHeight = 24;
    int itemPadding = 8;
    int alpha = 255;
};

struct Style
{
    RootStyle root;
    
    FrameStyle frame;
    TitleStyle title;
    ButtonStyle button;
    MenuStyle menu;

    Colors background;
    Colors foreground;

    int alpha = 255;

    const Colors& windowBackground() const;
    const Colors& windowTitleBackground() const;
    const Colors& windowTitleText() const;
    const Colors& windowBorder() const;

    int titleHeight() const;

    int frameBorderWidth() const;
    int frameRadius() const;
    int frameAlpha() const;

    int titleAlpha() const;
    const std::string& titleLayout() const;

    int buttonWidth() const;
    int buttonHeight() const;
    int buttonBorderWidth() const;
    int buttonRadius() const;
    int buttonAlpha() const;

    const Colors& buttonColors() const;
    const Colors& buttonHoverColors() const;
    const Colors& buttonPressedColors() const;
    const Colors& buttonBorderColors() const;
    const Colors& buttonTextColors() const;

    const Colors& menuColors() const;
    const Colors& menuTextColors() const;
    const Colors& menuHoverColors() const;
    const Colors& menuSelectedColors() const;
    const Colors& menuBorderColors() const;

    int menuBorderWidth() const;
    int menuRadius() const;
    int menuItemHeight() const;
    int menuItemPadding() const;
    int menuAlpha() const;

    int globalAlpha() const;
};

} // namespace HbTK
