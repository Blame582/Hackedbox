// Dock.cpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xatom.h>

#include "Dock.hpp"
#include "DockObject.hpp"
#include "DockButton.hpp"
#include "Screen.hpp"

Dock::Dock(HbScreen *screen)
  : screen(screen), window(None), position(Bottom), edge(EdgeCenter),
    size(600), visible(false), styleFile(true) {
    createDock();

    addObject(new DockButton("Hackedbox"));
}

Dock::~Dock(void) {
    clearObjects();
    destroyDock();
}

void Dock::createDock(void) {
    if (!screen || window != None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();
    XSetWindowAttributes attributes;

    attributes.background_pixel = 0x202020;
    attributes.border_pixel = 0;
    attributes.override_redirect = True;

    window = XCreateWindow(display, screen->getRootWindow(), 0, 0, 1, 1, 0,
                           CopyFromParent, InputOutput, CopyFromParent,
                           CWBackPixel | CWBorderPixel | CWOverrideRedirect,
                           &attributes);

    if (window == None)
        return;

	XSelectInput(display, window, ExposureMask | ButtonPressMask | StructureNotifyMask);

    Atom windowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom dockType = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    XChangeProperty(display, window, windowType, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(&dockType), 1);

    Atom desktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
    unsigned long allDesktops = 0xFFFFFFFFUL;

    XChangeProperty(display, window, desktop, XA_CARDINAL, 32, PropModeReplace,
                    reinterpret_cast<unsigned char *>(&allDesktops), 1);

    updateGeometry();
}

void Dock::destroyDock(void) {
    if (!screen || window == None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();

    if (visible)
        XUnmapWindow(display, window);

    XDestroyWindow(display, window);
    window = None;
}

void Dock::configure(Position newPosition, Edge newEdge, unsigned int newSize) {
    position = newPosition;
    edge = newEdge;
    size = newSize ? newSize : 1;

    updateGeometry();
    layoutObjects();

    if (visible) {
        draw();
        XRaiseWindow(screen->getBaseDisplay()->getXDisplay(), window);
    }
}

void Dock::useStyleFile(bool use) {
    styleFile = use;

    if (visible)
        draw();
}

void Dock::updateGeometry(void) {
    if (!screen || window == None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();
    const unsigned int screenWidth = screen->getWidth();
    const unsigned int screenHeight = screen->getHeight();

    unsigned int width = 1;
    unsigned int height = 1;
    int x = 0;
    int y = 0;

    if (position == Top || position == Bottom) {
        width = size;
        height = 64;

        if (width > screenWidth)
            width = screenWidth;

        switch (edge) {
        case EdgeLeft:
            x = 0;
            break;
        case EdgeCenter:
            x = (screenWidth - width) / 2;
            break;
        case EdgeRight:
            x = screenWidth - width;
            break;
        }

        y = position == Top ? 0 : screenHeight - height;
    } else {
        width = 64;
        height = size;

        if (height > screenHeight)
            height = screenHeight;

        switch (edge) {
        case EdgeLeft:
            y = 0;
            break;
        case EdgeCenter:
            y = (screenHeight - height) / 2;
            break;
        case EdgeRight:
            y = screenHeight - height;
            break;
        }

        x = position == Left ? 0 : screenWidth - width;
    }

    XMoveResizeWindow(display, window, x, y, width, height);
}

void Dock::layoutObjects(void) {
    if (!screen || window == None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();
    XWindowAttributes attributes;

    if (!XGetWindowAttributes(display, window, &attributes))
        return;

    const unsigned int objectSize = 48;
    const unsigned int spacing = 8;
    const unsigned int margin = 8;

    int objectX = margin;
    int objectY = margin;

    for (DockObject *object : objects) {
        if (position == Top || position == Bottom) {
            if (objectX + objectSize > attributes.width - margin)
                break;

            object->setGeometry(objectX, objectY, objectSize, objectSize);
            objectX += objectSize + spacing;
        } else {
            if (objectY + objectSize > attributes.height - margin)
                break;

            object->setGeometry(objectX, objectY, objectSize, objectSize);
            objectY += objectSize + spacing;
        }
    }
}

void Dock::addObject(DockObject *object) {
    if (!object)
        return;

    objects.push_back(object);
}

void Dock::clearObjects(void) {
    for (DockObject *object : objects)
        delete object;

    objects.clear();
}

void Dock::draw(void) {
    if (!screen || window == None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();
    XWindowAttributes attributes;

    if (!XGetWindowAttributes(display, window, &attributes))
        return;

    GC gc = XCreateGC(display, window, 0, 0);

    if (!gc)
        return;

    XSetForeground(display, gc, styleFile ? 0x202020 : 0x303030);
    XFillRectangle(display, window, gc, 0, 0,
                   attributes.width, attributes.height);

    XFreeGC(display, gc);

    layoutObjects();

    for (DockObject *object : objects)
        object->draw(display, window);
}

void Dock::handleEvent(XEvent *event) {
    if (!event || !screen || window == None)
        return;

    if (event->xany.window != window)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();

    switch (event->type) {
    case Expose:
        if (event->xexpose.count == 0)
            draw();
        break;

    case ButtonPress:
        for (DockObject *object : objects) {
            if (!object->contains(event->xbutton.x, event->xbutton.y))
                continue;

            if (object->buttonPress(display, window,
                                     event->xbutton.button,
                                     event->xbutton.x,
                                     event->xbutton.y))
                break;
        }
        break;
    }
}

void Dock::show(void) {
    if (!screen || window == None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();

    visible = true;

    updateGeometry();
    layoutObjects();

    XMapRaised(display, window);
    XClearWindow(display, window);
    draw();
    XRaiseWindow(display, window);
    XFlush(display);
}

void Dock::hide(void) {
    if (!screen || window == None)
        return;

    Display *display = screen->getBaseDisplay()->getXDisplay();

    XUnmapWindow(display, window);
    visible = false;

    XFlush(display);
}

void Dock::toggle(void) {
    if (visible)
        hide();
    else
        show();
}

bool Dock::isVisible(void) const {
    return visible;
}

bool Dock::handles(Window window) const {
    return this->window == window;
}