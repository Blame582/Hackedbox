#include <Button.hpp>

#include <Colors.hpp>
#include <Painter.hpp>

namespace HbTK {

Button::Button(int x,
               int y,
               int width,
               int height,
               const std::string& text)
    : m_text(text),
      m_pressed(false),
      m_enabled(true)
{
    setGeometry(x, y, width, height);
}

Button::~Button()
{
}

void Button::setText(const std::string& text)
{
    m_text = text;
}

const std::string& Button::text() const
{
    return m_text;
}

void Button::setPressed(bool pressed)
{
    m_pressed = pressed;
}

bool Button::pressed() const
{
    return m_pressed;
}

void Button::setEnabled(bool enabled)
{
    m_enabled = enabled;

    if (!m_enabled)
        m_pressed = false;
}

bool Button::enabled() const
{
    return m_enabled;
}

void Button::paint(Painter& painter)
{
    if (!visible())
        return;

    Colors background;

    if (!m_enabled)
        background = Colors(90, 90, 90);
    else if (m_pressed)
        background = Colors(110, 110, 110);
    else
        background = Colors(180, 180, 180);

    painter.fillRect(
        0,
        0,
        static_cast<unsigned int>(width()),
        static_cast<unsigned int>(height()),
        background
    );

    painter.drawRect(
        0,
        0,
        static_cast<unsigned int>(width()),
        static_cast<unsigned int>(height()),
        Colors(40, 40, 40)
    );

    painter.drawText(
        10,
        (height() / 2) + 5,
        m_text.c_str(),
        m_enabled
            ? Colors(20, 20, 20)
            : Colors(130, 130, 130)
    );
}

void Button::mousePress(int x,
                        int y,
                        unsigned int button)
{
    if (!m_enabled || button != 1)
        return;

    if (x < 0 ||
        y < 0 ||
        x >= width() ||
        y >= height())
        return;

    m_pressed = true;
}

void Button::mouseRelease(int x,
                          int y,
                          unsigned int button)
{
    if (!m_enabled || button != 1)
        return;

    const bool inside =
        x >= 0 &&
        y >= 0 &&
        x < width() &&
        y < height();

    m_pressed = false;

    /*
     * Button activation will be connected to the
     * HbTK event/callback system once that interface
     * is established.
     */

    (void)inside;
}

} // namespace HbTK
