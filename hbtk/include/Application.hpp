// include/HbTK/Application.hpp

#pragma once

#include <X11/Xlib.h>

namespace HbTK {

class Window;

class Application {
public:
    Application(int& argc,
                char** argv);

    ~Application();

    int exec();

    void quit();

    void addWindow(Window* window);

private:
    Display* m_display;
    bool m_running;
};

} // namespace HbTK
