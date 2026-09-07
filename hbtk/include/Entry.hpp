#pragma once

#include <cstddef>
#include <string>

#include <Widget.hpp>

namespace HbTK {

class Entry : public Widget
{
public:
    Entry(int x,
          int y,
          int width,
          int height);

    ~Entry() override;

    void setText(const std::string& text);
    const std::string& text() const;

    void clear();

    void setPlaceholder(const std::string& text);
    const std::string& placeholder() const;

    void setReadOnly(bool readOnly);
    bool readOnly() const;

    void setCursorPosition(std::size_t position);
    std::size_t cursorPosition() const;

    void setFocus(bool focus);
    bool hasFocus() const;

    void paint(Painter& painter) override;

    void mousePress(int x,
                    int y,
                    unsigned int button) override;

    void keyPress(unsigned int keycode) override;

private:
    std::string m_text;
    std::string m_placeholder;

    std::size_t m_cursorPosition;

    bool m_readOnly;
    bool m_focus;
};

} // namespace HbTK
