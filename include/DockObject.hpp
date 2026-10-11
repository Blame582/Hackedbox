// DockObject.hpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#ifndef __DockObject_hpp
#define __DockObject_hpp

#include <X11/Xlib.h>

class DockObject {
public:
    DockObject(void);
    virtual ~DockObject(void);

    virtual void draw(Display *display, Window window) = 0;
    virtual bool buttonPress(Display *display, Window window,
                             unsigned int button, int x, int y);

    void setGeometry(int x, int y, unsigned int width, unsigned int height);

    bool contains(int x, int y) const;

    int getX(void) const;
    int getY(void) const;
    unsigned int getWidth(void) const;
    unsigned int getHeight(void) const;

protected:
    int x;
    int y;
    unsigned int width;
    unsigned int height;
};

#endif // __DockObject_hpp