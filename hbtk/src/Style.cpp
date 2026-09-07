#include <Style.hpp>

namespace HbTK {

const Colors& Style::windowBackground() const
{
    return background;
}

const Colors& Style::windowTitleBackground() const
{
    return title.color;
}

const Colors& Style::windowTitleText() const
{
    return title.textColors;
}

const Colors& Style::windowBorder() const
{
    return frame.borderColors;
}

int Style::titleHeight() const
{
    return title.height;
}

int Style::frameBorderWidth() const
{
    return frame.borderWidth;
}

int Style::frameRadius() const
{
    return frame.radius;
}

int Style::frameAlpha() const
{
    return frame.alpha;
}

int Style::titleAlpha() const
{
    return title.alpha;
}

const std::string& Style::titleLayout() const
{
    return title.layout;
}

int Style::buttonWidth() const
{
    return button.width;
}

int Style::buttonHeight() const
{
    return button.height;
}

int Style::buttonBorderWidth() const
{
    return button.borderWidth;
}

int Style::buttonRadius() const
{
    return button.radius;
}

int Style::buttonAlpha() const
{
    return button.alpha;
}

const Colors& Style::buttonColors() const
{
    return button.color;
}

const Colors& Style::buttonHoverColors() const
{
    return button.hoverColors;
}

const Colors& Style::buttonPressedColors() const
{
    return button.pressedColors;
}

const Colors& Style::buttonBorderColors() const
{
    return button.borderColors;
}

const Colors& Style::buttonTextColors() const
{
    return button.textColors;
}

int Style::globalAlpha() const
{
    return alpha;
}

const Colors& Style::menuColors() const
{
    return menu.color;
}

const Colors& Style::menuTextColors() const
{
    return menu.textColors;
}

const Colors& Style::menuHoverColors() const
{
    return menu.hoverColors;
}

const Colors& Style::menuSelectedColors() const
{
    return menu.selectedColors;
}

const Colors& Style::menuBorderColors() const
{
    return menu.borderColors;
}

int Style::menuBorderWidth() const
{
    return menu.borderWidth;
}

int Style::menuRadius() const
{
    return menu.radius;
}

int Style::menuItemHeight() const
{
    return menu.itemHeight;
}

int Style::menuItemPadding() const
{
    return menu.itemPadding;
}

int Style::menuAlpha() const
{
    return menu.alpha;
}

} // namespace HbTK
