// src/Widget.cpp

#include <Widget.hpp>

namespace HbTK {

Widget::Widget()
    : m_parent(nullptr),
      m_x(0),
      m_y(0),
      m_width(0),
      m_height(0),
      m_visible(true)
{
}

Widget::~Widget() = default;

void Widget::paint(Painter&)
{
}

void Widget::resize(int width, int height)
{
    m_width = width;
    m_height = height;
}

void Widget::mousePress(int,
                        int,
                        unsigned int)
{
}

void Widget::mouseRelease(int,
                          int,
                          unsigned int)
{
}

void Widget::mouseMove(int,
                       int)
{
}

void Widget::keyPress(unsigned int)
{
}

void Widget::keyRelease(unsigned int)
{
}

void Widget::setParent(Widget* parent)
{
    m_parent = parent;
}

Widget* Widget::parent() const
{
    return m_parent;
}

void Widget::setGeometry(int x,
                         int y,
                         int width,
                         int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void Widget::setPosition(int x, int y)
{
    m_x = x;
    m_y = y;
}

void Widget::setSize(int width, int height)
{
    m_width = width;
    m_height = height;
}

int Widget::x() const
{
    return m_x;
}

int Widget::y() const
{
    return m_y;
}

int Widget::width() const
{
    return m_width;
}

int Widget::height() const
{
    return m_height;
}

bool Widget::visible() const
{
    return m_visible;
}

void Widget::show()
{
    m_visible = true;
}

void Widget::hide()
{
    m_visible = false;
}

} // namespace HbTK
