// DockButton.hpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#ifndef __DockButton_hpp
#define __DockButton_hpp

#include "DockObject.hpp"

class DockButton : public DockObject {
public:
    DockButton(const char *label);
    ~DockButton(void);

    void draw(Display *display, Window window);

private:
    char *label;
};

#endif // __DockButton_hpp