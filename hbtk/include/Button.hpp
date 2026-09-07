#pragma once

#include <string>

#include <Widget.hpp>

namespace HbTK {

class Button : public Widget
{
public:
    Button(int x,
           int y,
           int width,
           int height,
           const std::string& text);

    ~Button() override;

    void setText(const std::string& text);
    const std::string& text() const;

    void setPressed(bool pressed);
    bool pressed() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void paint(Painter& painter) override;

    void mousePress(int x,
                    int y,
                    unsigned int button) override;

    void mouseRelease(int x,
                      int y,
                      unsigned int button) override;

private:
    std::string m_text;

    bool m_pressed;
    bool m_enabled;
};

} // namespace HbTK
