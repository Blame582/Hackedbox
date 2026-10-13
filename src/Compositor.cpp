// Compositor.cpp for Hackedbox - an X Window Manager
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

#include "Compositor.hpp"

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>

HbCompositor::HbCompositor(Display *display, int screen)
    : display(display),
      root(display ? RootWindow(display, screen) : None),
      screen(screen),
      available(false),
      initialized(false),
      composite_event_base(0),
      composite_error_base(0),
      damage_event_base(0),
      damage_error_base(0),
      fixes_event_base(0),
      fixes_error_base(0),
      render_event_base(0),
      render_error_base(0),
      composite_major(0),
      composite_minor(0),
      damage_major(0),
      damage_minor(0),
      fixes_major(0),
      fixes_minor(0) {
}

HbCompositor::~HbCompositor() {
    shutdown();
}

bool HbCompositor::initialize() {
    if (initialized)
        return available;

    initialized = true;

    if (!display || root == None)
        return false;

    if (!XCompositeQueryExtension(
            display,
            &composite_event_base,
            &composite_error_base))
        return false;

    if (!XDamageQueryExtension(
            display,
            &damage_event_base,
            &damage_error_base))
        return false;

    if (!XFixesQueryExtension(
            display,
            &fixes_event_base,
            &fixes_error_base))
        return false;

    if (!XRenderQueryExtension(
            display,
            &render_event_base,
            &render_error_base))
        return false;

    if (!XCompositeQueryVersion(
            display,
            &composite_major,
            &composite_minor))
        return false;

    if (!XDamageQueryVersion(
            display,
            &damage_major,
            &damage_minor))
        return false;

    if (!XFixesQueryVersion(
            display,
            &fixes_major,
            &fixes_minor))
        return false;

    available = true;
    return true;
}

void HbCompositor::shutdown() {
    available = false;
    initialized = false;
}

bool HbCompositor::isAvailable() const {
    return available;
}

bool HbCompositor::isInitialized() const {
    return initialized;
}

Window HbCompositor::rootWindow() const {
    return root;
}

int HbCompositor::compositeEventBase() const {
    return composite_event_base;
}

int HbCompositor::damageEventBase() const {
    return damage_event_base;
}

int HbCompositor::fixesEventBase() const {
    return fixes_event_base;
}

int HbCompositor::renderEventBase() const {
    return render_event_base;
}
