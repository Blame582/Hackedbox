#pragma once

#include <vector>

#include <Widget.hpp>

namespace HbTK {

class WidgetContainer : public Widget
{
public:
    WidgetContainer();
    ~WidgetContainer() override;

    void addWidget(Widget* widget);

    void paint(Painter& painter) override;

    void mousePress(int x,
                    int y,
                    unsigned int button) override;

    void mouseRelease(int x,
                      int y,
                      unsigned int button) override;

    void mouseMove(int x,
                   int y) override;

    void keyPress(unsigned int keycode) override;
    void keyRelease(unsigned int keycode) override;

private:
    Widget* widgetAt(int x, int y) const;

    std::vector<Widget*> m_widgets;
    Widget* m_focusWidget;
};

} // namespace HbTK
