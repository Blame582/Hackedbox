#include <Entry.hpp>

#include <Colors.hpp>
#include <Painter.hpp>

namespace HbTK {

Entry::Entry(int x,
             int y,
             int width,
             int height)
    : m_text(),
      m_placeholder(),
      m_cursorPosition(0),
      m_readOnly(false),
      m_focus(false)
{
    setGeometry(x, y, width, height);
}

Entry::~Entry()
{
}

void Entry::setText(const std::string& text)
{
    m_text = text;

    if (m_cursorPosition > m_text.length())
        m_cursorPosition = m_text.length();
}

const std::string& Entry::text() const
{
    return m_text;
}

void Entry::clear()
{
    m_text.clear();
    m_cursorPosition = 0;
}

void Entry::setPlaceholder(const std::string& text)
{
    m_placeholder = text;
}

const std::string& Entry::placeholder() const
{
    return m_placeholder;
}

void Entry::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool Entry::readOnly() const
{
    return m_readOnly;
}

void Entry::setCursorPosition(std::size_t position)
{
    if (position > m_text.length())
        position = m_text.length();

    m_cursorPosition = position;
}

std::size_t Entry::cursorPosition() const
{
    return m_cursorPosition;
}

void Entry::setFocus(bool focus)
{
    m_focus = focus;
}

bool Entry::hasFocus() const
{
    return m_focus;
}

void Entry::paint(Painter& painter)
{
    if (!visible())
        return;

    painter.fillRect(
        0,
        0,
        static_cast<unsigned int>(width()),
        static_cast<unsigned int>(height()),
        Colors(30, 30, 30)
    );

    painter.drawRect(
        0,
        0,
        static_cast<unsigned int>(width()),
        static_cast<unsigned int>(height()),
        m_focus
            ? Colors(180, 180, 180)
            : Colors(80, 80, 80)
    );

    if (!m_text.empty())
    {
        painter.drawText(
            8,
            (height() / 2) + 5,
            m_text.c_str(),
            Colors(235, 235, 235)
        );
    }
    else if (!m_placeholder.empty())
    {
        painter.drawText(
            8,
            (height() / 2) + 5,
            m_placeholder.c_str(),
            Colors(120, 120, 120)
        );
    }

    /*
     * Cursor rendering will be added with the text/font
     * measurement support.
     */
}

void Entry::mousePress(int x,
                       int y,
                       unsigned int button)
{
    if (button != 1)
        return;

    if (x < 0 ||
        y < 0 ||
        x >= width() ||
        y >= height())
        return;

    m_focus = true;
}

void Entry::keyPress(unsigned int keycode)
{
    if (m_readOnly)
        return;

    /*
     * Text input and cursor editing will be implemented
     * once HbTK's keyboard event handling is connected.
     */

    (void)keycode;
}

} // namespace HbTK
