// src/Module.cpp

#include <Module.hpp>

namespace HbTK {

Module::Module(const std::string& title)
    : m_title(title),
      m_widget(nullptr),
      m_visible(true)
{
}

Module::~Module() = default;

const std::string& Module::title() const
{
    return m_title;
}

void Module::setWidget(Widget* widget)
{
    m_widget = widget;
}

Widget* Module::widget() const
{
    return m_widget;
}

void Module::setVisible(bool visible)
{
    m_visible = visible;

    if (!m_widget)
        return;

    if (visible)
        m_widget->show();
    else
        m_widget->hide();
}

bool Module::visible() const
{
    return m_visible;
}

} // namespace HbTK
