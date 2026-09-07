#include <cstring>

#include <Painter.hpp>
#include <Colors.hpp>

namespace HbTK {

Painter::Painter(Display* display,
                 ::Window window,
                 GC graphicsContext)
    : m_display(display),
      m_window(window),
      m_graphicsContext(graphicsContext)
{
}

unsigned long Painter::pixel(const Colors& color)
{
    XColor xcolor;

    xcolor.red =
        static_cast<unsigned short>(color.red()) * 257;

    xcolor.green =
        static_cast<unsigned short>(color.green()) * 257;

    xcolor.blue =
        static_cast<unsigned short>(color.blue()) * 257;

    xcolor.flags = DoRed | DoGreen | DoBlue;

    Colormap colormap =
        DefaultColormap(m_display,
                        DefaultScreen(m_display));

    if (!XAllocColor(m_display, colormap, &xcolor))
        return BlackPixel(m_display, DefaultScreen(m_display));

    return xcolor.pixel;
}

void Painter::fillRect(int x,
                       int y,
                       unsigned int width,
                       unsigned int height,
                       const Colors& color)
{
    XSetForeground(m_display,
                   m_graphicsContext,
                   pixel(color));

    XFillRectangle(m_display,
                   m_window,
                   m_graphicsContext,
                   x,
                   y,
                   width,
                   height);
}

void Painter::drawRect(int x,
                       int y,
                       unsigned int width,
                       unsigned int height,
                       const Colors& color)
{
    XSetForeground(m_display,
                   m_graphicsContext,
                   pixel(color));

    XDrawRectangle(m_display,
                   m_window,
                   m_graphicsContext,
                   x,
                   y,
                   width - 1,
                   height - 1);
}

void Painter::drawText(int x,
                       int y,
                       const char* text,
                       const Colors& color)
{
    XSetForeground(m_display,
                   m_graphicsContext,
                   pixel(color));

    XDrawString(m_display,
                m_window,
                m_graphicsContext,
                x,
                y,
                text,
                static_cast<int>(strlen(text)));
}

} // namespace HbTK
