// Compositor.hpp for Hackedbox - an X Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// Look in the Authors file for credits and copyrights.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef HACKEDBOX_COMPOSITOR_HPP
#define HACKEDBOX_COMPOSITOR_HPP

#include <X11/Xlib.h>

class HbCompositor {
private:
    Display *display;
    Window root;
    int screen;

    bool available;
    bool initialized;

    int composite_event_base;
    int composite_error_base;
    int damage_event_base;
    int damage_error_base;
    int fixes_event_base;
    int fixes_error_base;
    int render_event_base;
    int render_error_base;

    int composite_major;
    int composite_minor;
    int damage_major;
    int damage_minor;
    int fixes_major;
    int fixes_minor;

public:
    HbCompositor(Display *display, int screen);
    ~HbCompositor();

    HbCompositor(const HbCompositor &) = delete;
    HbCompositor &operator=(const HbCompositor &) = delete;

    bool initialize();
    void shutdown();

    bool isAvailable() const;
    bool isInitialized() const;

    Window rootWindow() const;

    int compositeEventBase() const;
    int damageEventBase() const;
    int fixesEventBase() const;
    int renderEventBase() const;
};

#endif // HACKEDBOX_COMPOSITOR_HPP
