#include <EventDispatcher.hpp>

#include <Event.hpp>
#include <Widget.hpp>

namespace HbTK {

EventDispatcher::EventDispatcher()
{
}

EventDispatcher::~EventDispatcher()
{
}

void EventDispatcher::dispatch(Widget* widget,
                               const Event& event)
{
    if (!widget)
        return;

    switch (event.type())
    {
        case EventType::MousePress:
            widget->mousePress(
                event.x(),
                event.y(),
                event.button()
            );
            break;

        case EventType::MouseRelease:
            widget->mouseRelease(
                event.x(),
                event.y(),
                event.button()
            );
            break;

        case EventType::MouseMove:
            widget->mouseMove(
                event.x(),
                event.y()
            );
            break;

        case EventType::KeyPress:
            widget->keyPress(
                event.keycode()
            );
            break;

        case EventType::KeyRelease:
            widget->keyRelease(
                event.keycode()
            );
            break;

        case EventType::Resize:
            widget->resize(
                event.width(),
                event.height()
            );
            break;

        case EventType::None:
        case EventType::FocusIn:
        case EventType::FocusOut:
        case EventType::Enter:
        case EventType::Leave:
        case EventType::Expose:
            break;
    }
}

} // namespace HbTK
