#pragma once

namespace HbTK {
class Window;
class Splitter;
class Widget;
}

class Hackedbox;

class HbSettingsWindow
{
public:
    explicit HbSettingsWindow(Hackedbox* hackedbox);
    ~HbSettingsWindow();

    bool show();
    void hide();

private:
    Hackedbox* m_hackedbox;

    HbTK::Window* m_window;
    HbTK::Splitter* m_splitter;
    HbTK::Widget* m_navigation;
    HbTK::Widget* m_display;

    bool m_visible;
};
