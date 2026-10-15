// Compositor.cpp for Hackedbox - an X Window manager
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

#include "Compositor.hpp"

#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>

#include <algorithm>
#include <iostream>
#include <string>

HbCompositor::HbCompositor(Display *display, int screen)
    : display(display),
      root(display ? RootWindow(display, screen) : None),
      overlay(None),
      selection_owner(None),
      screen(screen),
      damage_event_base(0),
      damage_error_base(0),
      available(false),
      initialized(false),
      owns_selection(false),
      root_picture(None),
      overlay_picture(None),
      selection_atom(None) {
}

HbCompositor::~HbCompositor() {
    shutdown();
}

bool HbCompositor::initialize() {
    if (initialized)
        return available;

    if (!display || root == None)
        return false;

    int event_base = 0;
    int error_base = 0;

    if (!XRenderQueryExtension(display, &event_base, &error_base) ||
        !XDamageQueryExtension(display, &damage_event_base,
                               &damage_error_base)) {
        std::cerr << "HbCompositor: XRender or XDamage unavailable\n";
        return false;
    }

    int major = 0;
    int minor = 0;

    if (!XCompositeQueryVersion(display, &major, &minor)) {
        std::cerr << "HbCompositor: XComposite unavailable\n";
        return false;
    }

    selection_atom = XInternAtom(
        display, ("_NET_WM_CM_S" + std::to_string(screen)).c_str(), False);

    if (XGetSelectionOwner(display, selection_atom) != None) {
        std::cerr << "Hackedbox: another compositor owns screen "
                  << screen << '\n';
        return false;
    }

    XSetWindowAttributes owner_attributes{};
    owner_attributes.override_redirect = True;
    owner_attributes.event_mask = StructureNotifyMask | PropertyChangeMask;

    selection_owner = XCreateWindow(
        display, root, -1, -1, 1, 1, 0,
        CopyFromParent, InputOutput, CopyFromParent,
        CWOverrideRedirect | CWEventMask, &owner_attributes);

    if (selection_owner == None)
        return false;

    XSetSelectionOwner(
        display, selection_atom, selection_owner, CurrentTime);
    XSync(display, False);

    if (XGetSelectionOwner(display, selection_atom) != selection_owner) {
        XDestroyWindow(display, selection_owner);
        selection_owner = None;
        return false;
    }

    owns_selection = true;

    /*
     * Redirect root subwindows, never the root window itself.
     * The root background remains owned by the normal X11 background
     * renderer. The overlay is only a presentation surface.
     */
    XCompositeRedirectSubwindows(
        display, root, CompositeRedirectManual);
    XSync(display, False);

    overlay = XCompositeGetOverlayWindow(display, root);

    if (overlay == None) {
        shutdown();
        return false;
    }

    XserverRegion empty_region =
        XFixesCreateRegion(display, nullptr, 0);

    XFixesSetWindowShapeRegion(
        display, overlay, ShapeInput, 0, 0, empty_region);
    XFixesDestroyRegion(display, empty_region);

    XWindowAttributes root_attributes{};
    XWindowAttributes overlay_attributes{};

    if (!XGetWindowAttributes(display, root, &root_attributes) ||
        !XGetWindowAttributes(display, overlay, &overlay_attributes)) {
        shutdown();
        return false;
    }

    XRenderPictFormat *root_format =
        XRenderFindVisualFormat(display, root_attributes.visual);
    XRenderPictFormat *overlay_format =
        XRenderFindVisualFormat(display, overlay_attributes.visual);

    if (!root_format || !overlay_format) {
        shutdown();
        return false;
    }

    root_picture = XRenderCreatePicture(
        display, root, root_format, 0, nullptr);

    overlay_picture = XRenderCreatePicture(
        display, overlay, overlay_format, 0, nullptr);

    if (root_picture == None || overlay_picture == None) {
        shutdown();
        return false;
    }

    available = true;
    initialized = true;

    Window returned_root = None;
    Window parent = None;
    Window *children = nullptr;
    unsigned int count = 0;

    if (XQueryTree(display, root, &returned_root, &parent,
                   &children, &count)) {
        for (unsigned int i = 0; i < count; ++i)
            trackWindow(children[i]);

        if (children)
            XFree(children);
    }

    /*
     * Initial setup is the only full-screen composition. Subsequent
     * damage events repaint only the affected area.
     */
    repaintRegion(0, 0,
                  root_attributes.width, root_attributes.height);

    XFlush(display);
    return true;
}

void HbCompositor::trackWindow(Window window) {
    if (!display || window == None ||
        window == root || window == overlay)
        return;

    if (damages.find(window) != damages.end())
        return;

    XWindowAttributes attributes{};

    if (!XGetWindowAttributes(display, window, &attributes) ||
        attributes.map_state != IsViewable ||
        attributes.c_class == InputOnly)
        return;

    Damage damage = XDamageCreate(
        display, window, XDamageReportNonEmpty);

    if (damage != None)
        damages.emplace(window, damage);
}

void HbCompositor::paintWindow(Window window, int x, int y) {
    if (!display || overlay_picture == None ||
        window == None || window == root || window == overlay)
        return;

    XWindowAttributes attributes{};

    if (!XGetWindowAttributes(display, window, &attributes) ||
        attributes.map_state != IsViewable ||
        attributes.c_class == InputOnly ||
        attributes.width <= 0 || attributes.height <= 0)
        return;

    trackWindow(window);

    Pixmap pixmap = XCompositeNameWindowPixmap(display, window);

    if (pixmap != None) {
        XRenderPictFormat *format =
            XRenderFindVisualFormat(display, attributes.visual);

        if (format) {
            Picture source = XRenderCreatePicture(
                display, pixmap, format, 0, nullptr);

            if (source != None) {
                XRenderComposite(
                    display, PictOpOver,
                    source, None, overlay_picture,
                    0, 0, 0, 0,
                    x, y,
                    static_cast<unsigned int>(attributes.width),
                    static_cast<unsigned int>(attributes.height));

                XRenderFreePicture(display, source);
            }
        }

        XFreePixmap(display, pixmap);
    }

    Window returned_root = None;
    Window parent = None;
    Window *children = nullptr;
    unsigned int count = 0;

    if (!XQueryTree(display, window, &returned_root, &parent,
                    &children, &count))
        return;

    for (unsigned int i = 0; i < count; ++i) {
        Window child = children[i];

        if (child == overlay || child == root)
            continue;

        XWindowAttributes child_attributes{};

        if (!XGetWindowAttributes(
                display, child, &child_attributes) ||
            child_attributes.map_state != IsViewable ||
            child_attributes.c_class == InputOnly)
            continue;

        int child_x = 0;
        int child_y = 0;
        Window child_return = None;

        if (XTranslateCoordinates(
                display, child, root, 0, 0,
                &child_x, &child_y, &child_return))
            paintWindow(child, child_x, child_y);
    }

    if (children)
        XFree(children);
}

void HbCompositor::repaintRegion(
    int x, int y, unsigned int width, unsigned int height) {
    if (!display || !initialized || !available ||
        overlay_picture == None || !width || !height)
        return;

    XWindowAttributes root_attributes{};

    if (!XGetWindowAttributes(display, root, &root_attributes))
        return;

    int left = std::max(0, x);
    int top = std::max(0, y);
    int right = std::min(
        root_attributes.width, x + static_cast<int>(width));
    int bottom = std::min(
        root_attributes.height, y + static_cast<int>(height));

    if (right <= left || bottom <= top)
        return;

    XRectangle rectangle{
        static_cast<short>(left),
        static_cast<short>(top),
        static_cast<unsigned short>(right - left),
        static_cast<unsigned short>(bottom - top)
    };

    XserverRegion region =
        XFixesCreateRegion(display, &rectangle, 1);

    XFixesSetPictureClipRegion(
        display, overlay_picture, 0, 0, region);

    /*
     * Clear the damaged overlay region. The root background is not
     * modified. The overlay is kept transparent where no window is
     * drawn, allowing the root background to show through.
     */
    XRenderColor transparent{};
    XRenderFillRectangle(
        display, PictOpSrc, overlay_picture, &transparent,
        left, top,
        static_cast<unsigned int>(right - left),
        static_cast<unsigned int>(bottom - top));

    Window returned_root = None;
    Window parent = None;
    Window *children = nullptr;
    unsigned int count = 0;

    if (XQueryTree(display, root, &returned_root, &parent,
                   &children, &count)) {
        for (unsigned int i = 0; i < count; ++i) {
            Window child = children[i];

            if (child == overlay || child == root)
                continue;

            XWindowAttributes attributes{};

            if (!XGetWindowAttributes(display, child, &attributes) ||
                attributes.map_state != IsViewable ||
                attributes.c_class == InputOnly)
                continue;

            int child_x = 0;
            int child_y = 0;
            Window child_return = None;

            if (XTranslateCoordinates(
                    display, child, root, 0, 0,
                    &child_x, &child_y, &child_return))
                paintWindow(child, child_x, child_y);
        }

        if (children)
            XFree(children);
    }

    XFixesSetPictureClipRegion(
        display, overlay_picture, 0, 0, None);
    XFixesDestroyRegion(display, region);

    XFlush(display);
}

void HbCompositor::repaint() {
    if (!display || !initialized || !available)
        return;

    XWindowAttributes attributes{};

    if (XGetWindowAttributes(display, root, &attributes))
        repaintRegion(0, 0, attributes.width, attributes.height);
}

bool HbCompositor::handleEvent(XEvent *event) {
    if (!initialized || !available || !event)
        return false;

    if (event->type == damage_event_base + XDamageNotify) {
        auto *damage_event =
            reinterpret_cast<XDamageNotifyEvent *>(event);

        auto it = damages.find(damage_event->drawable);

        if (it == damages.end())
            return false;

        XDamageSubtract(display, it->second, None, None);

        int wx = 0;
        int wy = 0;
        Window child_return = None;

        if (XTranslateCoordinates(
                display, damage_event->drawable, root,
                0, 0, &wx, &wy, &child_return)) {
            repaintRegion(
                wx + damage_event->area.x,
                wy + damage_event->area.y,
                damage_event->area.width,
                damage_event->area.height);
        }

        return true;
    }

    switch (event->type) {
    case MapNotify:
        trackWindow(event->xmap.window);
        repaint();
        break;

    case UnmapNotify:
        repaint();
        break;

    case ConfigureNotify:
    case CirculateNotify:
    case ReparentNotify:
        repaint();
        break;

    case DestroyNotify: {
        Window window = event->xdestroywindow.window;
        auto it = damages.find(window);

        if (it != damages.end()) {
            XDamageDestroy(display, it->second);
            damages.erase(it);
        }

        repaint();
        break;
    }

    default:
        break;
    }

    return false;
}

void HbCompositor::clearTracking() {
    if (!display)
        return;

    for (const auto &[window, damage] : damages) {
        (void)window;
        XDamageDestroy(display, damage);
    }

    damages.clear();
    redirected.clear();
}

void HbCompositor::shutdown() {
    if (!display)
        return;

    clearTracking();

    if (overlay_picture != None) {
        XRenderFreePicture(display, overlay_picture);
        overlay_picture = None;
    }

    if (root_picture != None) {
        XRenderFreePicture(display, root_picture);
        root_picture = None;
    }

    if (overlay != None) {
        XCompositeReleaseOverlayWindow(display, root);
        overlay = None;
    }

    if (root != None)
        XCompositeUnredirectSubwindows(
            display, root, CompositeRedirectManual);

    if (owns_selection && selection_atom != None &&
        selection_owner != None &&
        XGetSelectionOwner(display, selection_atom) == selection_owner) {
        XSetSelectionOwner(
            display, selection_atom, None, CurrentTime);
    }

    if (selection_owner != None) {
        XDestroyWindow(display, selection_owner);
        selection_owner = None;
    }

    XSync(display, False);

    owns_selection = false;
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
