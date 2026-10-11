// Dock.hpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#ifndef __Dock_hpp
#define __Dock_hpp

#include <X11/Xlib.h>
#include <vector>

class HbScreen;
class DockObject;

class Dock {
public:
    enum Position {
        Top,
        Right,
        Bottom,
        Left
    };

    enum Edge {
        EdgeLeft,
        EdgeCenter,
        EdgeRight
    };

    Dock(HbScreen *screen);
    ~Dock(void);

    void show(void);
    void hide(void);
    void toggle(void);

    bool isVisible(void) const;
    bool handles(Window window) const;

    void configure(Position position, Edge edge, unsigned int size);
    void useStyleFile(bool use);

    void draw(void);
    void handleEvent(XEvent *event);

private:
    Dock(const Dock&);
    Dock& operator=(const Dock&);

    void createDock(void);
    void destroyDock(void);
    void updateGeometry(void);
    void layoutObjects(void);
    void addObject(DockObject *object);
    void clearObjects(void);

    HbScreen *screen;
    Window window;

    Position position;
    Edge edge;

    unsigned int size;

    bool visible;
    bool styleFile;

    std::vector<DockObject *> objects;
};

#endif // __Dock_hpp