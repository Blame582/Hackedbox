#include <Event.hpp>

namespace HbTK {

Event::Event()
    : m_type(EventType::None),
      m_x(0),
      m_y(0),
      m_button(0),
      m_keycode(0),
      m_width(0),
      m_height(0)
{
}

Event::Event(EventType type)
    : m_type(type),
      m_x(0),
      m_y(0),
      m_button(0),
      m_keycode(0),
      m_width(0),
      m_height(0)
{
}

EventType Event::type() const
{
    return m_type;
}

void Event::setType(EventType type)
{
    m_type = type;
}

int Event::x() const
{
    return m_x;
}

int Event::y() const
{
    return m_y;
}

void Event::setPosition(int x, int y)
{
    m_x = x;
    m_y = y;
}

unsigned int Event::button() const
{
    return m_button;
}

void Event::setButton(unsigned int button)
{
    m_button = button;
}

unsigned int Event::keycode() const
{
    return m_keycode;
}

void Event::setKeycode(unsigned int keycode)
{
    m_keycode = keycode;
}

int Event::width() const
{
    return m_width;
}

int Event::height() const
{
    return m_height;
}

void Event::setSize(int width, int height)
{
    m_width = width;
    m_height = height;
}

} // namespace HbTK
