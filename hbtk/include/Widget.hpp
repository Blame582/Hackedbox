// include/Widget.hpp

#pragma once

namespace HbTK {

class Painter;

class Widget
{
public:
    Widget();
    virtual ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    virtual void paint(Painter& painter);

    virtual void resize(int width, int height);

    virtual void mousePress(int x,
                            int y,
                            unsigned int button);

    virtual void mouseRelease(int x,
                              int y,
                              unsigned int button);

    virtual void mouseMove(int x,
                           int y);

    virtual void keyPress(unsigned int keycode);

    virtual void keyRelease(unsigned int keycode);

    void setParent(Widget* parent);
    Widget* parent() const;

    void setGeometry(int x,
                     int y,
                     int width,
                     int height);

    void setPosition(int x, int y);
    void setSize(int width, int height);

    int x() const;
    int y() const;

    int width() const;
    int height() const;

    bool visible() const;

    void show();
    void hide();

protected:
    Widget* m_parent;

    int m_x;
    int m_y;
    int m_width;
    int m_height;

    bool m_visible;
};

} // namespace HbTK
