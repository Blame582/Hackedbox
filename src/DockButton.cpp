// DockButton.cpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#include <cstring>

#include "DockButton.hpp"

DockButton::DockButton(const char *newLabel)
  : DockObject(), label(0) {
    if (newLabel)
        label = strdup(newLabel);
}

DockButton::~DockButton(void) {
    delete[] label;
}

void DockButton::draw(Display *display, Window window) {
    GC gc = XCreateGC(display, window, 0, 0);

    if (!gc)
        return;

    XSetForeground(display, gc, 0x303030);
    XFillRectangle(display, window, gc, x, y, width, height);

    XSetForeground(display, gc, 0x909090);
    XDrawRectangle(display, window, gc, x, y, width - 1, height - 1);

    if (label) {
        XSetForeground(display, gc, 0xFFFFFF);
        XDrawString(display, window, gc, x + 8, y + 20, label,
                    static_cast<int>(strlen(label)));
    }

    XFreeGC(display, gc);
}