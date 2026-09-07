#include <MainWindow.hpp>

#include <Painter.hpp>
#include <Style.hpp>

#include <algorithm>

namespace HbTK {

Window::Window(int x,
               int y,
               int width,
               int height)
    : m_display(nullptr),
      m_window(0),
      m_graphicsContext(0),
      m_title("HbTK"),
      m_style(nullptr),
      m_centralWidget(nullptr),
      m_created(false),
      m_wmDeleteAtom(0)
{
    setGeometry(x, y, width, height);
}

Window::~Window()
{
    if (m_graphicsContext && m_display)
        XFreeGC(m_display, m_graphicsContext);

    if (m_window && m_display)
        XDestroyWindow(m_display, m_window);

    if (m_display)
        XCloseDisplay(m_display);
}

bool Window::create()
{
    if (m_created)
        return true;

    m_display = XOpenDisplay(nullptr);

    if (!m_display)
        return false;

    const int screen = DefaultScreen(m_display);

    m_window =
        XCreateSimpleWindow(
            m_display,
            RootWindow(m_display, screen),
            x(),
            y(),
            static_cast<unsigned int>(width()),
            static_cast<unsigned int>(height()),
            0,
            BlackPixel(m_display, screen),
            WhitePixel(m_display, screen)
        );

    if (!m_window)
    {
        XCloseDisplay(m_display);
        m_display = nullptr;
        return false;
    }

    const long eventMask =
        ExposureMask |
        StructureNotifyMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask |
        KeyPressMask |
        KeyReleaseMask;

    XSelectInput(
        m_display,
        m_window,
        eventMask
    );

    /*
     * Tell the window manager that this window supports
     * the standard close request.
     */
    m_wmDeleteAtom =
        XInternAtom(
            m_display,
            "WM_DELETE_WINDOW",
            False
        );

    XSetWMProtocols(
        m_display,
        m_window,
        &m_wmDeleteAtom,
        1
    );

    m_graphicsContext =
        XCreateGC(
            m_display,
            m_window,
            0,
            nullptr
        );

    if (!m_graphicsContext)
    {
        XDestroyWindow(m_display, m_window);
        XCloseDisplay(m_display);

        m_window = 0;
        m_display = nullptr;

        return false;
    }

    XStoreName(
        m_display,
        m_window,
        m_title.c_str()
    );

    m_created = true;

    return true;
}

void Window::show()
{
    if (!create())
        return;

    XMapWindow(
        m_display,
        m_window
    );

    XFlush(m_display);

    paint();
}

void Window::hide()
{
    if (!m_created)
        return;

    XUnmapWindow(
        m_display,
        m_window
    );

    XFlush(m_display);
}

void Window::setTitle(const std::string& title)
{
    m_title = title;

    if (!m_created)
        return;

    XStoreName(
        m_display,
        m_window,
        m_title.c_str()
    );

    XFlush(m_display);
}

void Window::setStyle(const Style& style)
{
    m_style = &style;

    if (m_created)
        paint();
}

void Window::setCentralWidget(Widget* widget)
{
    if (m_centralWidget == widget)
        return;

    if (m_centralWidget)
        m_centralWidget->setParent(nullptr);

    m_centralWidget = widget;

    if (!m_centralWidget)
    {
        if (m_created)
            paint();

        return;
    }

    m_centralWidget->setParent(this);

    int top = 0;

    if (m_style)
        top = m_style->titleHeight();

    m_centralWidget->setGeometry(
        0,
        top,
        width(),
        std::max(0, height() - top)
    );

    if (m_created)
        paint();
}

Widget* Window::centralWidget() const
{
    return m_centralWidget;
}

Display* Window::display() const
{
    return m_display;
}

::Window Window::window() const
{
    return m_window;
}

Atom Window::wmDeleteAtom() const
{
    return m_wmDeleteAtom;
}

void Window::resize(int width, int height)
{
    Widget::resize(width, height);

    if (!m_centralWidget)
        return;

    int top = 0;

    if (m_style)
        top = m_style->titleHeight();

    m_centralWidget->setGeometry(
        0,
        top,
        width,
        std::max(0, height - top)
    );
}

void Window::paint()
{
    if (!m_created)
        return;

    Painter painter(
        m_display,
        m_window,
        m_graphicsContext
    );

    /*
     * Give Window a usable default appearance even when
     * no Style has been assigned.
     */
    if (m_style)
    {
        painter.fillRect(
            0,
            0,
            static_cast<unsigned int>(width()),
            static_cast<unsigned int>(height()),
            m_style->windowBackground()
        );

        const int titleHeight =
            m_style->titleHeight();

        if (titleHeight > 0)
        {
            painter.fillRect(
                0,
                0,
                static_cast<unsigned int>(width()),
                static_cast<unsigned int>(titleHeight),
                m_style->windowTitleBackground()
            );

            painter.drawText(
                10,
                titleHeight - 6,
                m_title.c_str(),
                m_style->windowTitleText()
            );
        }
    }
    else
    {
        Colors background(235, 235, 235);
        Colors titleBackground(55, 55, 55);
        Colors titleText(255, 255, 255);

        painter.fillRect(
            0,
            0,
            static_cast<unsigned int>(width()),
            static_cast<unsigned int>(height()),
            background
        );

        painter.fillRect(
            0,
            0,
            static_cast<unsigned int>(width()),
            28,
            titleBackground
        );

        painter.drawText(
            10,
            20,
            m_title.c_str(),
            titleText
        );
    }

    if (m_centralWidget &&
        m_centralWidget->visible())
    {
        m_centralWidget->paint(painter);
    }

    XFlush(m_display);
}

} // namespace HbTK
