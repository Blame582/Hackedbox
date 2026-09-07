// include/Module.hpp

#pragma once

#include <string>

#include <Widget.hpp>

namespace HbTK {

class Module
{
public:
    explicit Module(const std::string& title);
    virtual ~Module();

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    const std::string& title() const;

    void setWidget(Widget* widget);
    Widget* widget() const;

    void setVisible(bool visible);
    bool visible() const;

private:
    std::string m_title;
    Widget* m_widget;
    bool m_visible;
};

} // namespace HbTK
