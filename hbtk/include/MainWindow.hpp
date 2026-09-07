// hbtk/include/MainWindow.hpp

#pragma once

#include <string>

#include <X11/Xlib.h>

#include <Widget.hpp>

namespace HbTK {

class Style;

class Window : public Widget
{
public:
    Window(int x, int y, int width, int height);

    ~Window() override;

    bool create();

    void show();
    void hide();

    void setTitle(const std::string& title);

    void setStyle(const Style& style);

    void setCentralWidget(Widget* widget);
    Widget* centralWidget() const;

    Display* display() const;
    ::Window window() const;

    using Widget::paint;
    void paint();

    void resize(int width, int height) override;

    Atom wmDeleteAtom() const;

private:
    Display* m_display;
    ::Window m_window;
    GC m_graphicsContext;

    std::string m_title;

    const Style* m_style;

    Widget* m_centralWidget;

    bool m_created;

    Atom m_wmDeleteAtom;
};

} // namespace HbTK
