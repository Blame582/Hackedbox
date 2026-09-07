// Application.cpp for HbTK
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#include "Application.hpp"

#include <MainWindow.hpp>

namespace HbTK {

Application::Application(int& argc,
                         char** argv)
    : m_display(nullptr),
      m_running(false)
{
    (void)argc;
    (void)argv;

    m_display = XOpenDisplay(nullptr);
}

Application::~Application()
{
    if (m_display)
        XCloseDisplay(m_display);
}

int Application::exec()
{
    if (!m_display)
        return 1;

    m_running = true;

    while (m_running)
    {
        XEvent event;

        XNextEvent(m_display, &event);

        if (event.type == ClientMessage ||
            event.type == DestroyNotify)
        {
            continue;
        }
    }

    return 0;
}

void Application::quit()
{
    m_running = false;
}

void Application::addWindow(Window* window)
{
    if (!window)
        return;

    if (!window->create())
        return;

    window->show();
}

} // namespace HbTK
