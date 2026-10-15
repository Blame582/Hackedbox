// Compositor.hpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
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
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>

#include <unordered_map>
#include <unordered_set>

class HbCompositor {
public:
    HbCompositor(Display *display, int screen);
    ~HbCompositor();

    HbCompositor(const HbCompositor &) = delete;
    HbCompositor &operator=(const HbCompositor &) = delete;

    bool initialize();
    void shutdown();
    bool handleEvent(XEvent *event);
    void repaint();

    bool isAvailable() const;
    bool isInitialized() const;
    Window rootWindow() const;

private:
    void trackWindow(Window window);
    void paintWindow(Window window, int x, int y);
    void repaintRegion(int x, int y,
                       unsigned int width, unsigned int height);
    void clearTracking();

    Display *display;
    Window root;
    Window overlay;
    Window selection_owner;
    int screen;
    int damage_event_base;
    int damage_error_base;
    bool available;
    bool initialized;
    bool owns_selection;

    Picture root_picture;
    Picture overlay_picture;
    Atom selection_atom;

    std::unordered_set<Window> redirected;
    std::unordered_map<Window, Damage> damages;
};

#endif
