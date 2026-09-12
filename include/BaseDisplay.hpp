// BaseDisplay.hpp for Hackedbox - an X Window Manager
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

#ifndef HACKEDBOX_BASE_DISPLAY_HPP
#define HACKEDBOX_BASE_DISPLAY_HPP

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <cstddef>
#include <string>
#include <vector>

#include "Timer.hpp"
#include "Util.hpp"

class BaseDisplay;
class GCCache;

class ScreenInfo {
private:
  BaseDisplay *basedisplay;

  Visual *visual;
  Window root_window;
  Colormap colormap;

  int depth;
  unsigned int screen_number;

  std::string display_string;
  Rect rect;

public:
  ScreenInfo(
    BaseDisplay *display,
    unsigned int number
  );

  BaseDisplay *getBaseDisplay() const {
    return basedisplay;
  }

  Visual *getVisual() const {
    return visual;
  }

  Window getRootWindow() const {
    return root_window;
  }

  Colormap getColormap() const {
    return colormap;
  }

  int getDepth() const {
    return depth;
  }

  unsigned int getScreenNumber() const {
    return screen_number;
  }

  const Rect &getRect() const {
    return rect;
  }

  unsigned int getWidth() const {
    return rect.width();
  }

  unsigned int getHeight() const {
    return rect.height();
  }

  const std::string &displayString() const {
    return display_string;
  }
};


class BaseDisplay : public TimerQueueManager {
private:
  struct ShapeInfo {
    bool extensions;
    int event_basep;
    int error_basep;
  };

  ShapeInfo shape;

  unsigned int MaskList[8];
  std::size_t MaskListLength;

  enum RunState {
    STARTUP,
    RUNNING,
    SHUTDOWN
  };

  RunState run_state;

  Display *display;
  mutable GCCache *gccache;

  using ScreenInfoList =
    std::vector<ScreenInfo>;

  ScreenInfoList screenInfoList;

  HbTimerQueue timerList;

  const char *display_name;
  const char *application_name;

  // No copying.
  BaseDisplay(const BaseDisplay &) = delete;
  BaseDisplay &operator=(const BaseDisplay &) = delete;

protected:
  // Pure virtual event handler implemented by Hackedbox.
  virtual void process_event(XEvent *event) = 0;

  // Modifier masks ignored for button events.
  int NumLockMask;
  int ScrollLockMask;

public:
  BaseDisplay(
    const char *applicationName,
    const char *displayName = nullptr
  );

  virtual ~BaseDisplay();

  const ScreenInfo *getScreenInfo(
    unsigned int screen
  ) const;

  GCCache *gcCache() const;

  bool hasShapeExtensions() const {
    return shape.extensions;
  }

  bool doShutdown() const {
    return run_state == SHUTDOWN;
  }

  bool isStartup() const {
    return run_state == STARTUP;
  }

  Display *getXDisplay() const {
    return display;
  }

  const char *getXDisplayName() const {
    return display_name;
  }

  const char *getApplicationName() const {
    return application_name;
  }

  unsigned int getNumberOfScreens() const {
    return static_cast<unsigned int>(
      screenInfoList.size()
    );
  }

  int getShapeEventBase() const {
    return shape.event_basep;
  }

  void shutdown() {
    run_state = SHUTDOWN;
  }

  void run() {
    run_state = RUNNING;
  }

  void grabButton(
    unsigned int button,
    unsigned int modifiers,
    Window grab_window,
    bool owner_events,
    unsigned int event_mask,
    int pointer_mode,
    int keyboard_mode,
    Window confine_to,
    Cursor cursor,
    bool allow_scroll_lock
  ) const;

  void ungrabButton(
    unsigned int button,
    unsigned int modifiers,
    Window grab_window
  ) const;

  void eventLoop();

  // TimerQueueManager interface.
  void addTimer(HbTimer *timer) override;
  void removeTimer(HbTimer *timer) override;

  // Handle signals not understood by BaseDisplay.
  virtual bool handleSignal(int signal) = 0;
};


#endif // HACKEDBOX_BASE_DISPLAY_HPP
