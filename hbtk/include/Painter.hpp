// include/HbTK/Painter.hpp

#pragma once

#include <X11/Xlib.h>

#include <Colors.hpp>

namespace HbTK {

class Painter {
public:
    Painter(Display* display,
            ::Window window,
            GC graphicsContext);

    void fillRect(int x,
                  int y,
                  unsigned int width,
                  unsigned int height,
                  const Colors& color);

    void drawRect(int x,
                  int y,
                  unsigned int width,
                  unsigned int height,
                  const Colors& color);

    void drawText(int x,
                  int y,
                  const char* text,
                  const Colors& color);

private:
    unsigned long pixel(const Colors& color);

    Display* m_display;
    ::Window m_window;
    GC m_graphicsContext;
};

} // namespace HbTK
