//Settings.cpp
#include <Settings.hpp>

#include <SWindow.hpp>

HbSettings::HbSettings(Hackedbox* hackedbox)
    : m_hackedbox(hackedbox),
      m_window(nullptr)
{
    m_window =
        new HbSettingsWindow(m_hackedbox);
}

HbSettings::~HbSettings()
{
    delete m_window;
}

bool HbSettings::show()
{
    if (!m_window)
        return false;

    return m_window->show();
}

void HbSettings::hide()
{
    if (!m_window)
        return;

    m_window->hide();
}
