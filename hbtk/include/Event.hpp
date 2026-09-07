#pragma once

namespace HbTK {

enum class EventType {
    None,

    MousePress,
    MouseRelease,
    MouseMove,

    KeyPress,
    KeyRelease,

    FocusIn,
    FocusOut,

    Enter,
    Leave,

    Expose,
    Resize
};

class Event
{
public:
    Event();
    explicit Event(EventType type);

    EventType type() const;

    void setType(EventType type);

    int x() const;
    int y() const;

    void setPosition(int x, int y);

    unsigned int button() const;
    void setButton(unsigned int button);

    unsigned int keycode() const;
    void setKeycode(unsigned int keycode);

    int width() const;
    int height() const;

    void setSize(int width, int height);

private:
    EventType m_type;

    int m_x;
    int m_y;

    unsigned int m_button;
    unsigned int m_keycode;

    int m_width;
    int m_height;
};

} // namespace HbTK
