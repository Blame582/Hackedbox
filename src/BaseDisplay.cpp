// BaseDisplay.cpp for Hackedbox - an XLibre Window Manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H


#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "BaseDisplay.hpp"
#include "GCCache.hpp"
#include "Timer.hpp"
#include "Util.hpp"

// X error handler to handle any X errors while the application is running.
static bool internal_error = false;

BaseDisplay *base_display = nullptr;


static int handleXErrors(Display *display, XErrorEvent *event) {
#ifdef DEBUG
  char error_text[128];

  XGetErrorText(
    display,
    event->error_code,
    error_text,
    sizeof(error_text)
  );

  std::fprintf(
    stderr,
    "%s: X error: %s (%d), opcodes %d/%d, resource 0x%lx\n",
    base_display
      ? base_display->getApplicationName()
      : "Hackedbox",
    error_text,
    event->error_code,
    event->request_code,
    event->minor_code,
    event->resourceid
  );
#else
  (void)display;
  (void)event;
#endif // DEBUG

  if (internal_error)
    std::abort();

  return False;
}


// Signal handler to allow for proper and gentle shutdown.
static void signalHandler(int signal) {
  static int re_enter = 0;

  switch (signal) {

  case SIGCHLD: {
    int status = 0;

    waitpid(
      -1,
      &status,
      WNOHANG | WUNTRACED
    );

    break;
  }

default:
  if (base_display &&
      base_display->handleSignal(signal)) {

    return;
  }

    if (base_display) {
      std::fprintf(
        stderr,
        "%s: signal %d caught\n",
        base_display->getApplicationName(),
        signal
      );

      if (!base_display->isStartup() && !re_enter) {
        internal_error = true;
        re_enter = 1;

        std::fprintf(
          stderr,
          "%s",
          "shutting down\n"
        );

        base_display->shutdown();
      }
    }

    if (signal != SIGTERM &&
        signal != SIGINT) {

      std::fprintf(
        stderr,
        "%s",
        "aborting... dumping core\n"
      );

      std::abort();
    }

    std::exit(0);
  }
}


BaseDisplay::BaseDisplay(
  const char *applicationName,
  const char *displayName) {

  application_name = applicationName;
  run_state = STARTUP;

  ::base_display = this;

#ifdef HAVE_SIGACTION
  struct sigaction action {};

  action.sa_handler = signalHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_NOCLDSTOP | SA_NODEFER;

  sigaction(SIGPIPE, &action, nullptr);
  sigaction(SIGSEGV, &action, nullptr);
  sigaction(SIGFPE, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGCHLD, &action, nullptr);
  sigaction(SIGHUP, &action, nullptr);
  sigaction(SIGUSR1, &action, nullptr);
  sigaction(SIGUSR2, &action, nullptr);

#else // !HAVE_SIGACTION

  signal(SIGPIPE, signalHandler);
  signal(SIGSEGV, signalHandler);
  signal(SIGFPE, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGUSR1, signalHandler);
  signal(SIGUSR2, signalHandler);
  signal(SIGHUP, signalHandler);
  signal(SIGCHLD, signalHandler);

#endif // HAVE_SIGACTION


  display = XOpenDisplay(displayName);

  if (!display) {
    std::fprintf(
      stderr,
      "%s: connection to X server failed.\n",
      "BaseDisplay::BaseDisplay"
    );

    std::exit(2);
  }


  if (fcntl(
        ConnectionNumber(display),
        F_SETFD,
        FD_CLOEXEC
      ) == -1) {

    std::fprintf(
      stderr,
      "%s: couldn't mark display connection as close-on-exec\n",
      "BaseDisplay::BaseDisplay"
    );

    std::exit(2);
  }


  display_name = XDisplayName(displayName);


#ifdef SHAPE

  shape.extensions =
    XShapeQueryExtension(
      display,
      &shape.event_basep,
      &shape.error_basep
    );

#else

  shape.extensions = False;

#endif // SHAPE


  XSetErrorHandler(
    reinterpret_cast<XErrorHandler>(handleXErrors)
  );


  screenInfoList.reserve(
    ScreenCount(display)
  );

  for (int screen = 0;
       screen < ScreenCount(display);
       ++screen) {

    screenInfoList.emplace_back(
      this,
      static_cast<unsigned int>(screen)
    );
  }


  NumLockMask = 0;
  ScrollLockMask = 0;


  const XModifierKeymap *modifierMap =
    XGetModifierMapping(display);

  if (modifierMap &&
      modifierMap->max_keypermod > 0) {

    const int maskTable[] = {
      ShiftMask,
      LockMask,
      ControlMask,
      Mod1Mask,
      Mod2Mask,
      Mod3Mask,
      Mod4Mask,
      Mod5Mask
    };

    const std::size_t size =
      (sizeof(maskTable) / sizeof(maskTable[0])) *
      static_cast<std::size_t>(
        modifierMap->max_keypermod
      );


    const KeyCode numLock =
      XKeysymToKeycode(
        display,
        XK_Num_Lock
      );

    const KeyCode scrollLock =
      XKeysymToKeycode(
        display,
        XK_Scroll_Lock
      );


    for (std::size_t count = 0;
         count < size;
         ++count) {

      if (!modifierMap->modifiermap[count])
        continue;


      if (numLock ==
          modifierMap->modifiermap[count]) {

        NumLockMask =
          maskTable[
            count /
            static_cast<std::size_t>(
              modifierMap->max_keypermod
            )
          ];
      }


      if (scrollLock ==
          modifierMap->modifiermap[count]) {

        ScrollLockMask =
          maskTable[
            count /
            static_cast<std::size_t>(
              modifierMap->max_keypermod
            )
          ];
      }
    }
  }


  MaskList[0] = 0;
  MaskList[1] = LockMask;
  MaskList[2] = NumLockMask;
  MaskList[3] = LockMask | NumLockMask;
  MaskList[4] = ScrollLockMask;
  MaskList[5] = ScrollLockMask | LockMask;
  MaskList[6] = ScrollLockMask | NumLockMask;
  MaskList[7] =
    ScrollLockMask |
    LockMask |
    NumLockMask;

  MaskListLength =
    sizeof(MaskList) /
    sizeof(MaskList[0]);


  if (modifierMap)
    XFreeModifiermap(
      const_cast<XModifierKeymap *>(modifierMap)
    );


  gccache = nullptr;
}


BaseDisplay::~BaseDisplay() {
  delete gccache;
  gccache = nullptr;

  if (display)
    XCloseDisplay(display);

  display = nullptr;
}


void BaseDisplay::eventLoop() {
  run();

  const int xfd =
    ConnectionNumber(display);

  while (run_state == RUNNING &&
         !internal_error) {

    if (XPending(display)) {

      XEvent event {};

      XNextEvent(
        display,
        &event
      );

      process_event(&event);

    } else {

      fd_set read_fds;
      timeval now {};
      timeval timeout_value {};
      timeval *timeout = nullptr;


      FD_ZERO(&read_fds);
      FD_SET(xfd, &read_fds);


      if (!timerList.empty()) {

        const HbTimer *timer =
          timerList.top();

        gettimeofday(
          &now,
          nullptr
        );

        timeout_value =
          timer->timeRemaining(now);

        timeout = &timeout_value;
      }


      select(
        xfd + 1,
        &read_fds,
        nullptr,
        nullptr,
        timeout
      );


      gettimeofday(
        &now,
        nullptr
      );


      while (!timerList.empty()) {

        HbTimer *timer =
          timerList.top();

        if (!timer->shouldFire(now))
          break;


        timerList.pop();

        timer->fireTimeout();
        timer->halt();

        if (timer->isRecurring())
          timer->start();
      }
    }
  }
}


void BaseDisplay::addTimer(HbTimer *timer) {
  if (!timer)
    return;

  timerList.push(timer);
}


void BaseDisplay::removeTimer(HbTimer *timer) {
  if (!timer)
    return;

  timerList.release(timer);
}


/*
 * Grabs a button, including every possible combination with the keyboard
 * lock modifiers so that they do not cancel the event.
 *
 * If allow_scroll_lock is true, only the first half of MaskList is used and
 * Scroll Lock is ignored.
 */
void BaseDisplay::grabButton(
  unsigned int button,
  unsigned int modifiers,
  Window grab_window,
  bool owner_events,
  unsigned int event_mask,
  int pointer_mode,
  int keyboard_mode,
  Window confine_to,
  Cursor cursor,
  bool allow_scroll_lock) const {

  const std::size_t length =
    allow_scroll_lock
      ? MaskListLength / 2
      : MaskListLength;


  for (std::size_t count = 0;
       count < length;
       ++count) {

    XGrabButton(
      display,
      button,
      modifiers | MaskList[count],
      grab_window,
      owner_events,
      event_mask,
      pointer_mode,
      keyboard_mode,
      confine_to,
      cursor
    );
  }
}


/*
 * Releases a button grab and all possible combinations of the keyboard
 * lock modifiers.
 */
void BaseDisplay::ungrabButton(
  unsigned int button,
  unsigned int modifiers,
  Window grab_window) const {

  for (std::size_t count = 0;
       count < MaskListLength;
       ++count) {

    XUngrabButton(
      display,
      button,
      modifiers | MaskList[count],
      grab_window
    );
  }
}


const ScreenInfo *BaseDisplay::getScreenInfo(
  unsigned int screen) const {

  if (screen < screenInfoList.size())
    return &screenInfoList[screen];

  return nullptr;
}


GCCache *BaseDisplay::gcCache() const {
  if (!gccache) {

    gccache =
      new GCCache(
        this,
        screenInfoList.size()
      );
  }

  return gccache;
}


ScreenInfo::ScreenInfo(
  BaseDisplay *display,
  unsigned int number) {

  basedisplay = display;
  screen_number = number;


  root_window =
    RootWindow(
      basedisplay->getXDisplay(),
      screen_number
    );


  Screen *screen =
    ScreenOfDisplay(
      basedisplay->getXDisplay(),
      screen_number
    );


  rect.setSize(
    WidthOfScreen(screen),
    HeightOfScreen(screen)
  );


  /*
   * If the default depth is at least 8, use it.
   * Otherwise search for the largest TrueColor visual.
   * 24-bit depth is preferred over depths greater than 24.
   */

  depth =
    DefaultDepth(
      basedisplay->getXDisplay(),
      screen_number
    );

  visual =
    DefaultVisual(
      basedisplay->getXDisplay(),
      screen_number
    );

  colormap =
    DefaultColormap(
      basedisplay->getXDisplay(),
      screen_number
    );


  if (depth < 8) {

    XVisualInfo visualTemplate {};
    XVisualInfo *visualInfo = nullptr;
    int visualCount = 0;
    int best = -1;


    visualTemplate.screen =
      static_cast<int>(screen_number);

    visualTemplate.c_class =
      TrueColor;


    visualInfo =
      XGetVisualInfo(
        basedisplay->getXDisplay(),
        VisualScreenMask |
        VisualClassMask,
        &visualTemplate,
        &visualCount
      );


    if (visualInfo) {

      int maxDepth = 1;


      for (int i = 0;
           i < visualCount;
           ++i) {

        if (visualInfo[i].depth > maxDepth) {

          if (maxDepth == 24 &&
              visualInfo[i].depth > 24) {
            break;
          }

          maxDepth =
            visualInfo[i].depth;

          best = i;
        }
      }


      if (maxDepth < depth)
        best = -1;
    }


    if (best != -1) {

      depth =
        visualInfo[best].depth;

      visual =
        visualInfo[best].visual;

      colormap =
        XCreateColormap(
          basedisplay->getXDisplay(),
          root_window,
          visual,
          AllocNone
        );
    }


    if (visualInfo)
      XFree(visualInfo);
  }


  /*
   * Get the default display string and strip the screen number.
   */
  std::string defaultString =
    DisplayString(
      basedisplay->getXDisplay()
    );


  const std::string::size_type position =
    defaultString.rfind('.');


  if (position != std::string::npos)
    defaultString.resize(position);


  display_string =
    "DISPLAY=" +
    defaultString +
    '.' +
    itostring(
      static_cast<unsigned long>(
        screen_number
      )
    );
}
