//hbtk/include/EventDispatcher.hpp
#pragma once

namespace HbTK {

class Event;
class Widget;

class EventDispatcher
{
public:
    EventDispatcher();
    ~EventDispatcher();

    void dispatch(Widget* widget,
                  const Event& event);
};

} // namespace HbTK
