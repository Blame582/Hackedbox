#pragma once

#include <string>

#include <Widget.hpp>

namespace HbTK {

class Painter;
struct Style;

class Frame : public Widget
{
public:
    Frame();
    ~Frame() override;

    void setStyle(const Style& style);
    const Style* style() const;

    void setTitle(const std::string& title);
    const std::string& title() const;

    void setCentralWidget(Widget* widget);
    Widget* centralWidget() const;

    void resize(int width, int height) override;

    void paint(Painter& painter) override;

private:
    void updateCentralWidgetGeometry();

    const Style* m_style;
    std::string m_title;
    Widget* m_centralWidget;
};

} // namespace HbTK
