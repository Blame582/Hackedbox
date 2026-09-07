// src/Colors.cpp

#include <Colors.hpp>

namespace HbTK {

Colors::Colors()
    : m_red(0),
      m_green(0),
      m_blue(0)
{
}

Colors::Colors(unsigned char red,
             unsigned char green,
             unsigned char blue)
    : m_red(red),
      m_green(green),
      m_blue(blue)
{
}

unsigned char Colors::red() const
{
    return m_red;
}

unsigned char Colors::green() const
{
    return m_green;
}

unsigned char Colors::blue() const
{
    return m_blue;
}

} // namespace HbTK
