// hackedbox.cpp for Hackedbox - an XLibre Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <X11/extensions/shape.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>

#include "Hackedbox.hpp"
#include "BaseMenu.hpp"
#include "ClientMenu.hpp"
#include "ImageControl.hpp"
#include "GCCache.hpp"
#include "RootMenu.hpp"
#include "Screen.hpp"
#include "Util.hpp"
#include "Window.hpp"
#include "Workspace.hpp"

namespace fs = std::filesystem;

Hackedbox *hackedbox = nullptr;

namespace {

volatile std::sig_atomic_t signal_pending = 0;

void signalHandler(int signal) noexcept {
  signal_pending = signal;
}

std::optional<fs::path> environmentPath(const char *name) {
  if (const char *value = std::getenv(name); value && *value)
    return fs::path(value);

  return std::nullopt;
}

fs::path configurationDirectory() {
  if (const auto xdg = environmentPath("XDG_CONFIG_HOME"))
    return *xdg / "hackedbox";

  if (const auto home = environmentPath("HOME"))
    return *home / ".hackedbox";

  return {};
}

fs::path configurationFile(std::string_view filename) {
  const fs::path directory = configurationDirectory();

  if (directory.empty())
    return {};

  return directory / filename;
}

bool ensureConfigurationDirectory(const fs::path& directory) {
  if (directory.empty())
    return false;

  std::error_code error;

  if (fs::exists(directory, error))
    return fs::is_directory(directory, error);

  return fs::create_directories(directory, error);
}

std::string makeResourceName(std::string_view prefix,
                             int screenNumber,
                             std::string_view resource) {
  std::string result;
  result.reserve(prefix.size() + 32 + resource.size());

  result += prefix;
  result += std::to_string(screenNumber);
  result += '.';
  result += resource;

  return result;
}

std::string makeResourceClass(std::string_view prefix,
                              int screenNumber,
                              std::string_view resource) {
  std::string result;
  result.reserve(prefix.size() + 32 + resource.size());

  result += prefix;
  result += std::to_string(screenNumber);
  result += '.';
  result += resource;

  return result;
}

bool resourceIsTrue(const XrmValue& value) {
  if (!value.addr)
    return false;

  const std::string_view text(value.addr, value.size);

  return text == "true" ||
         text == "True" ||
         text == "TRUE";
}

bool resourceIsFalse(const XrmValue& value) {
  if (!value.addr)
    return false;

  const std::string_view text(value.addr, value.size);

  return text == "false" ||
         text == "False" ||
         text == "FALSE";
}

void putResource(XrmDatabase *database,
                 std::string_view name,
                 std::string_view value) {
  std::string resource;
  resource.reserve(name.size() + value.size() + 2);

  resource += name;
  resource += ": ";
  resource += value;

  XrmPutLineResource(database, resource.c_str());
}

} // namespace


Hackedbox::Hackedbox(char **m_argv,
                     const char *displayName,
                     const char *configurationFileName)
  : BaseDisplay(m_argv[0], displayName),
    argv(m_argv) {

  if (!XSupportsLocale())
    std::cerr << "Hackedbox: X server does not support locale\n";

  if (XSetLocaleModifiers("") == nullptr)
    std::cerr << "Hackedbox: cannot set locale modifiers\n";

  ::hackedbox = this;

  if (configurationFileName) {
    rc_file = expandTilde(configurationFileName);
  } else {
    rc_file = configurationFile("hackedbox.rc");

    if (rc_file.empty()) {
      std::cerr
        << "Hackedbox: unable to determine configuration directory\n";
      std::exit(EXIT_FAILURE);
    }

    if (!ensureConfigurationDirectory(rc_file.parent_path())) {
      std::error_code error;

      if (!fs::exists(rc_file.parent_path(), error) ||
          !fs::is_directory(rc_file.parent_path(), error)) {
        std::cerr
          << "Hackedbox: unable to create configuration directory '"
          << rc_file.parent_path()
          << "': "
          << error.message()
          << '\n';

        std::exit(EXIT_FAILURE);
      }
    }
  }

  no_focus = false;

  resource.auto_raise_delay.tv_sec = 0;
  resource.auto_raise_delay.tv_usec = 0;

  active_screen = nullptr;
  focused_window = nullptr;

  XrmInitialize();

  load_rc();
  init_icccm();

  Display *display = getXDisplay();

  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);
  cursor.ll_angle = XCreateFontCursor(display, XC_ll_angle);
  cursor.lr_angle = XCreateFontCursor(display, XC_lr_angle);

  /*
   * Create the clock before creating the screens.
   *
   * Menus are constructed by HbScreen, so the clock must already
   * exist when the menus perform their initial update.
   */
  clock = new Clock(this);

  clock->setTimeFormat(resource.clock_time_format);
  clock->setDateFormat(resource.clock_date_format);
  clock->setColor(resource.clock_color);
  clock->setPosition(resource.clock_x, resource.clock_y);

  clock->setRoot(
    resource.clock_target == "root" ||
    resource.clock_target == "both"
  );

  clock->update();

  for (unsigned int i = 0; i < getNumberOfScreens(); ++i) {
    auto *screen = new HbScreen(this, i);

    if (!screen->isScreenManaged()) {
      delete screen;
      continue;
    }

    screenList.push_back(screen);
  }

  if (screenList.empty()) {
    std::cerr
      << "Hackedbox::Hackedbox: no manageable screens found, aborting.\n";

    std::exit(EXIT_FAILURE);
  }

  active_screen = screenList.front();
  setFocusedWindow(nullptr);

  XSynchronize(display, False);
  XSync(display, False);

  reconfigure_wait = false;
  reread_menu_wait = false;

  /*
   * Existing timer is used for deferred reconfigure/menu reread.
   * Keep it separate from the recurring clock timer.
   */
  timer = new HbTimer(this, this);
  timer->setTimeout(0L);

  /*
   * Clock timer fires once per second.
   */
  clock_timer = new HbTimer(this, this);
  clock_timer->setTimeout(1000L);
  clock_timer->recurring(true);

  if (resource.clock_enabled) {
    if (resource.clock_target == "root" ||
        resource.clock_target == "both") {
      clock->drawRoot();
    }

    clock_timer->start();
  }
}


Hackedbox::~Hackedbox() {
  if (clock_timer)
    clock_timer->stop();

  delete clock_timer;
  delete clock;

  for (auto *screen : screenList)
    delete screen;

  for (auto *timestamp : menuTimestamps)
    delete timestamp;

  delete timer;
}


void Hackedbox::process_event(XEvent *event) {
  switch (event->type) {

  case ButtonPress: {
    event->xbutton.state &= ~(NumLockMask | ScrollLockMask | LockMask);

    last_time = event->xbutton.time;

    HackedboxWindow *window = searchWindow(event->xbutton.window);
    HbBasemenu *menu = nullptr;
    HbScreen *screen = nullptr;

    if (window) {
      window->buttonPressEvent(&event->xbutton);

      if (event->xbutton.button == 1)
        window->installColormap(True);

    } else if ((menu = searchMenu(event->xbutton.window))) {
      menu->buttonPressEvent(&event->xbutton);

    } else if ((screen = searchScreen(event->xbutton.window))) {
      screen->buttonPressEvent(&event->xbutton);

      if (active_screen != screen) {
        active_screen = screen;
        setFocusedWindow(nullptr);
      }
    }

    break;
  }


  case ButtonRelease: {
    event->xbutton.state &= ~(NumLockMask | ScrollLockMask | LockMask);

    last_time = event->xbutton.time;

    HackedboxWindow *window = searchWindow(event->xbutton.window);
    HbBasemenu *menu = nullptr;

    if (window) {
      window->buttonReleaseEvent(&event->xbutton);
    } else if ((menu = searchMenu(event->xbutton.window))) {
      menu->buttonReleaseEvent(&event->xbutton);
    }

    break;
  }


  case ConfigureRequest: {
    HackedboxWindow *window =
      searchWindow(event->xconfigurerequest.window);

    if (window) {
      window->configureRequestEvent(&event->xconfigurerequest);
    } else if (validateWindow(event->xconfigurerequest.window)) {
      XWindowChanges changes{};

      changes.x = event->xconfigurerequest.x;
      changes.y = event->xconfigurerequest.y;
      changes.width = event->xconfigurerequest.width;
      changes.height = event->xconfigurerequest.height;
      changes.border_width = event->xconfigurerequest.border_width;
      changes.sibling = event->xconfigurerequest.above;
      changes.stack_mode = event->xconfigurerequest.detail;

      XConfigureWindow(
        getXDisplay(),
        event->xconfigurerequest.window,
        event->xconfigurerequest.value_mask,
        &changes
      );
    }

    break;
  }


  case MapRequest: {
#ifdef DEBUG
    std::cerr
      << "Hackedbox::process_event(): MapRequest for 0x"
      << std::hex
      << event->xmaprequest.window
      << std::dec
      << '\n';
#endif

    HackedboxWindow *window = searchWindow(event->xmaprequest.window);

    if (window) {
      bool focus = false;

      if (window->isIconic()) {
        window->deiconify();
        focus = true;
      }

      if (window->isShaded()) {
        window->shade();
        focus = true;
      }

      if (focus &&
          (window->isTransient() ||
           window->getScreen()->doFocusNew())) {
        window->setInputFocus();
      }

    } else {
      HbScreen *screen =
        searchScreen(event->xmaprequest.parent);

      if (!screen) {
        XWindowAttributes attributes{};

        if (!XGetWindowAttributes(
              getXDisplay(),
              event->xmaprequest.window,
              &attributes)) {
          break;
        }

        screen = searchScreen(attributes.root);

        assert(screen != nullptr);
      }

      screen->manageWindow(event->xmaprequest.window);
    }

    break;
  }


  case UnmapNotify: {
    if (auto *window = searchWindow(event->xunmap.window))
      window->unmapNotifyEvent(&event->xunmap);

    break;
  }


  case DestroyNotify: {
    if (auto *window = searchWindow(event->xdestroywindow.window)) {
      window->destroyNotifyEvent(&event->xdestroywindow);

    } else if (auto *group =
                 searchGroup(event->xdestroywindow.window)) {
      delete group;
    }

    break;
  }


  case ReparentNotify: {
    if (auto *window = searchWindow(event->xreparent.window))
      window->reparentNotifyEvent(&event->xreparent);

    break;
  }


  case MotionNotify: {
    XEvent realEvent{};
    unsigned int compressed = 0;

    while (XCheckTypedWindowEvent(
             getXDisplay(),
             event->xmotion.window,
             MotionNotify,
             &realEvent)) {
      ++compressed;
    }

    if (compressed > 0)
      event = &realEvent;

    event->xmotion.state &= ~(NumLockMask |
                              ScrollLockMask |
                              LockMask);

    last_time = event->xmotion.time;

    if (auto *window = searchWindow(event->xmotion.window)) {
      window->motionNotifyEvent(&event->xmotion);

    } else if (auto *menu = searchMenu(event->xmotion.window)) {
      menu->motionNotifyEvent(&event->xmotion);
    }

    break;
  }


  case PropertyNotify: {
    last_time = event->xproperty.time;

    if (auto *window = searchWindow(event->xproperty.window))
      window->propertyNotifyEvent(&event->xproperty);

    break;
  }


  case EnterNotify: {
    last_time = event->xcrossing.time;

    if (event->xcrossing.mode == NotifyGrab)
      break;

    if (event->xcrossing.window == event->xcrossing.root) {
      if (auto *screen =
            searchScreen(event->xcrossing.window)) {
        screen->getImageControl()->installRootColormap();
      }

    } else if (auto *window =
                 searchWindow(event->xcrossing.window)) {
      if (!no_focus)
        window->enterNotifyEvent(&event->xcrossing);

    } else if (auto *menu =
                 searchMenu(event->xcrossing.window)) {
      menu->enterNotifyEvent(&event->xcrossing);
    }

    break;
  }


  case LeaveNotify: {
    last_time = event->xcrossing.time;

    if (auto *menu = searchMenu(event->xcrossing.window)) {
      menu->leaveNotifyEvent(&event->xcrossing);

    } else if (auto *window =
                 searchWindow(event->xcrossing.window)) {
      window->leaveNotifyEvent(&event->xcrossing);
    }

    break;
  }


  case Expose: {
    XEvent realEvent{};

    unsigned int compressed = 0;

    int x1 = event->xexpose.x;
    int y1 = event->xexpose.y;

    int x2 =
      x1 + event->xexpose.width - 1;

    int y2 =
      y1 + event->xexpose.height - 1;

    while (XCheckTypedWindowEvent(
             getXDisplay(),
             event->xexpose.window,
             Expose,
             &realEvent)) {

      ++compressed;

      x1 = std::min(realEvent.xexpose.x, x1);
      y1 = std::min(realEvent.xexpose.y, y1);

      x2 = std::max(
        realEvent.xexpose.x +
        realEvent.xexpose.width - 1,
        x2
      );

      y2 = std::max(
        realEvent.xexpose.y +
        realEvent.xexpose.height - 1,
        y2
      );
    }

    if (compressed > 0)
      event = &realEvent;

    event->xexpose.x = x1;
    event->xexpose.y = y1;
    event->xexpose.width = x2 - x1 + 1;
    event->xexpose.height = y2 - y1 + 1;

    if (auto *window = searchWindow(event->xexpose.window)) {
      window->exposeEvent(&event->xexpose);

    } else if (auto *menu = searchMenu(event->xexpose.window)) {
      menu->exposeEvent(&event->xexpose);
    }

    break;
  }


  case KeyPress:
    // Keybindings are handled directly by Hackedbox.
    break;


  case ColormapNotify: {
    if (auto *screen =
          searchScreen(event->xcolormap.window)) {
      screen->setRootColormapInstalled(
        event->xcolormap.state == ColormapInstalled
      );
    }

    break;
  }


  case FocusIn: {
    if (event->xfocus.detail != NotifyNonlinear)
      break;

    if (auto *window = searchWindow(event->xfocus.window)) {
      if (!window->isFocused())
        window->setFocusFlag(True);

      event->xfocus.window = None;
    }

    break;
  }


  case FocusOut: {
    if (event->xfocus.detail != NotifyNonlinear)
      break;

    HackedboxWindow *window = searchWindow(event->xfocus.window);

    if (window && window->isFocused()) {
      XEvent focusEvent{};

      bool checkFocus =
        event->xfocus.mode == NotifyNormal;

      if (XCheckTypedEvent(
            getXDisplay(),
            FocusIn,
            &focusEvent)) {

        process_event(&focusEvent);

        if (focusEvent.xfocus.window == None)
          checkFocus = false;
      }

      if (checkFocus) {
        Window focus = None;
        int revert = RevertToNone;

        XGetInputFocus(
          getXDisplay(),
          &focus,
          &revert
        );

        if (auto *focusWindow = searchWindow(focus))
          setFocusedWindow(focusWindow);
        else
          setFocusedWindow(nullptr);
      }
    }

    break;
  }


  case ClientMessage: {
    if (event->xclient.format != 32)
      break;

    if (event->xclient.message_type ==
        getWMChangeStateAtom()) {

      HackedboxWindow *window =
        searchWindow(event->xclient.window);

      if (!window || !window->validateClient())
        break;

      if (event->xclient.data.l[0] == IconicState)
        window->iconify();

      else if (event->xclient.data.l[0] == NormalState)
        window->deiconify();

    } else if (
      event->xclient.message_type ==
      getHackedboxChangeWorkspaceAtom()) {

      HbScreen *screen =
        searchScreen(event->xclient.window);

      const unsigned int workspace =
        static_cast<unsigned int>(
          event->xclient.data.l[0]
        );

      if (screen &&
          workspace < screen->getWorkspaceCount()) {
        screen->changeWorkspaceID(workspace);
      }

    } else if (
      event->xclient.message_type ==
      getHackedboxChangeWindowFocusAtom()) {

      HackedboxWindow *window =
        searchWindow(event->xclient.window);

      if (window &&
          window->isVisible() &&
          window->setInputFocus()) {
        window->installColormap(True);
      }

    } else if (
      event->xclient.message_type ==
      getHackedboxCycleWindowFocusAtom()) {

      HbScreen *screen =
        searchScreen(event->xclient.window);

      if (screen) {
        if (event->xclient.data.l[0] == 0)
          screen->prevFocus();
        else
          screen->nextFocus();
      }

    } else if (
      event->xclient.message_type ==
      getHackedboxChangeAttributesAtom()) {

      HackedboxWindow *window =
        searchWindow(event->xclient.window);

      if (window && window->validateClient()) {
        HackedboxHints hints{};

        hints.flags = event->xclient.data.l[0];
        hints.attrib = event->xclient.data.l[1];
        hints.workspace = event->xclient.data.l[2];
        hints.stack = event->xclient.data.l[3];
        hints.decoration = event->xclient.data.l[4];

        window->changeHackedboxHints(&hints);
      }
    }

    break;
  }


  case NoExpose:
  case ConfigureNotify:
  case MapNotify:
    break;


  default: {
#ifdef SHAPE
    if (event->type == getShapeEventBase()) {
      auto *shapeEvent =
        reinterpret_cast<XShapeEvent *>(event);

      if (auto *window =
            searchWindow(event->xany.window)) {
        window->shapeEvent(shapeEvent);
      }
    }
#endif
    break;
  }

  }
}


bool Hackedbox::handleSignal(int signal) {
  switch (signal) {
  case SIGHUP:
  case SIGUSR1:
  case SIGUSR2:
  case SIGPIPE:
  case SIGINT:
  case SIGTERM:
    signal_pending = signal;
    return true;

  default:
    return false;
  }
}


void Hackedbox::init_icccm() {
  Display *display = getXDisplay();

  xa_wm_colormap_windows =
    XInternAtom(display, "WM_COLORMAP_WINDOWS", False);

  xa_wm_protocols =
    XInternAtom(display, "WM_PROTOCOLS", False);

  xa_wm_state =
    XInternAtom(display, "WM_STATE", False);

  xa_wm_change_state =
    XInternAtom(display, "WM_CHANGE_STATE", False);

  xa_wm_delete_window =
    XInternAtom(display, "WM_DELETE_WINDOW", False);

  xa_wm_take_focus =
    XInternAtom(display, "WM_TAKE_FOCUS", False);

  motif_wm_hints =
    XInternAtom(display, "_MOTIF_WM_HINTS", False);

  hackedbox_hints =
    XInternAtom(display, "_HACKEDBOX_HINTS", False);

  hackedbox_attributes =
    XInternAtom(display, "_HACKEDBOX_ATTRIBUTES", False);

  hackedbox_change_attributes =
    XInternAtom(display, "_HACKEDBOX_CHANGE_ATTRIBUTES", False);

  hackedbox_structure_messages =
    XInternAtom(display, "_HACKEDBOX_STRUCTURE_MESSAGES", False);

  hackedbox_notify_startup =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_STARTUP", False);

  hackedbox_notify_window_add =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_WINDOW_ADD", False);

  hackedbox_notify_window_del =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_WINDOW_DEL", False);

  hackedbox_notify_current_workspace =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_CURRENT_WORKSPACE", False);

  hackedbox_notify_workspace_count =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_WORKSPACE_COUNT", False);

  hackedbox_notify_window_focus =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_WINDOW_FOCUS", False);

  hackedbox_notify_window_raise =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_WINDOW_RAISE", False);

  hackedbox_notify_window_lower =
    XInternAtom(display, "_HACKEDBOX_NOTIFY_WINDOW_LOWER", False);

  hackedbox_change_workspace =
    XInternAtom(display, "_HACKEDBOX_CHANGE_WORKSPACE", False);

  hackedbox_change_window_focus =
    XInternAtom(display, "_HACKEDBOX_CHANGE_WINDOW_FOCUS", False);

  hackedbox_cycle_window_focus =
    XInternAtom(display, "_HACKEDBOX_CYCLE_WINDOW_FOCUS", False);

#ifdef NEWWMSPEC
  net_supported =
    XInternAtom(display, "_NET_SUPPORTED", False);

  net_client_list =
    XInternAtom(display, "_NET_CLIENT_LIST", False);

  net_client_list_stacking =
    XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);

  net_number_of_desktops =
    XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);

  net_desktop_geometry =
    XInternAtom(display, "_NET_DESKTOP_GEOMETRY", False);

  net_desktop_viewport =
    XInternAtom(display, "_NET_DESKTOP_VIEWPORT", False);

  net_current_desktop =
    XInternAtom(display, "_NET_CURRENT_DESKTOP", False);

  net_desktop_names =
    XInternAtom(display, "_NET_DESKTOP_NAMES", False);

  net_active_window =
    XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

  net_workarea =
    XInternAtom(display, "_NET_WORKAREA", False);

  net_supporting_wm_check =
    XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);

  net_virtual_roots =
    XInternAtom(display, "_NET_VIRTUAL_ROOTS", False);

  net_close_window =
    XInternAtom(display, "_NET_CLOSE_WINDOW", False);

  net_wm_moveresize =
    XInternAtom(display, "_NET_WM_MOVERESIZE", False);

  net_properties =
    XInternAtom(display, "_NET_PROPERTIES", False);

  net_wm_name =
    XInternAtom(display, "_NET_WM_NAME", False);

  net_wm_desktop =
    XInternAtom(display, "_NET_WM_DESKTOP", False);

  net_wm_window_type =
    XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

  net_wm_state =
    XInternAtom(display, "_NET_WM_STATE", False);

  net_wm_strut =
    XInternAtom(display, "_NET_WM_STRUT", False);

  net_wm_icon_geometry =
    XInternAtom(display, "_NET_WM_ICON_GEOMETRY", False);

  net_wm_icon =
    XInternAtom(display, "_NET_WM_ICON", False);

  net_wm_pid =
    XInternAtom(display, "_NET_WM_PID", False);

  net_wm_handled_icons =
    XInternAtom(display, "_NET_WM_HANDLED_ICONS", False);

  net_wm_ping =
    XInternAtom(display, "_NET_WM_PING", False);
#endif

#ifdef HAVE_GETPID
  hackedbox_pid =
    XInternAtom(display, "_HACKEDBOX_PID", False);
#endif
}


bool Hackedbox::validateWindow(Window window) {
  XEvent event{};

  if (XCheckTypedWindowEvent(
        getXDisplay(),
        window,
        DestroyNotify,
        &event)) {

    XPutBackEvent(getXDisplay(), &event);
    return false;
  }

  return true;
}


HbScreen *Hackedbox::searchScreen(Window window) {
  for (auto *screen : screenList) {
    if (screen->getRootWindow() == window)
      return screen;
  }

  return nullptr;
}


HackedboxWindow *Hackedbox::searchWindow(Window window) {
  const auto it = windowSearchList.find(window);

  if (it != windowSearchList.end())
    return it->second;

  return nullptr;
}


HbWindowGroup *Hackedbox::searchGroup(Window window) {
  const auto it = groupSearchList.find(window);

  if (it != groupSearchList.end())
    return it->second;

  return nullptr;
}


HbBasemenu *Hackedbox::searchMenu(Window window) {
  const auto it = menuSearchList.find(window);

  if (it != menuSearchList.end())
    return it->second;

  return nullptr;
}


void Hackedbox::saveWindowSearch(Window window,
                                  HackedboxWindow *data) {
  windowSearchList.emplace(window, data);
}


void Hackedbox::saveGroupSearch(Window window,
                                HbWindowGroup *data) {
  groupSearchList.emplace(window, data);
}


void Hackedbox::saveMenuSearch(Window window,
                               HbBasemenu *data) {
  menuSearchList.emplace(window, data);
}


void Hackedbox::removeWindowSearch(Window window) {
  windowSearchList.erase(window);
}


void Hackedbox::removeGroupSearch(Window window) {
  groupSearchList.erase(window);
}


void Hackedbox::removeMenuSearch(Window window) {
  menuSearchList.erase(window);
}


void Hackedbox::restart(const char *program) {
  shutdown();

  if (program && *program) {
    const std::string display =
      screenList.empty()
        ? std::string{}
        : screenList.front()->displayString();

    if (!display.empty())
      setenv("DISPLAY", display.c_str(), 1);

    execlp(program, program, static_cast<char *>(nullptr));

    std::cerr
      << "Hackedbox: unable to restart '"
      << program
      << "': "
      << std::strerror(errno)
      << '\n';
  }

  if (argv && argv[0])
    execvp(argv[0], argv);

  std::cerr
    << "Hackedbox: unable to restart process\n";
}


void Hackedbox::shutdown() {
  BaseDisplay::shutdown();

  XSetInputFocus(
    getXDisplay(),
    PointerRoot,
    None,
    CurrentTime
  );

  for (auto *screen : screenList)
    screen->shutdown();

  XSync(getXDisplay(), False);

  save_rc();
}


void Hackedbox::save_rc() {
  XrmDatabase newDatabase = nullptr;

  load_rc();

  putResource(
    &newDatabase,
    "session.menuFile",
    getMenuFilename()
  );

  putResource(
    &newDatabase,
    "session.colorsPerChannel",
    std::to_string(resource.colors_per_channel)
  );

  putResource(
    &newDatabase,
    "session.doubleClickInterval",
    std::to_string(resource.double_click_interval)
  );

  const unsigned long autoRaiseDelay =
    (resource.auto_raise_delay.tv_sec * 1000UL) +
    (resource.auto_raise_delay.tv_usec / 1000UL);

  putResource(
    &newDatabase,
    "session.autoRaiseDelay",
    std::to_string(autoRaiseDelay)
  );

  putResource(
    &newDatabase,
    "session.cacheLife",
    std::to_string(resource.cache_life / 60000UL)
  );

  putResource(
    &newDatabase,
    "session.cacheMax",
    std::to_string(resource.cache_max)
  );

  for (auto *screen : screenList) {
    const int screenNumber =
      screen->getScreenNumber();

    putResource(
      &newDatabase,
      "session.opaqueMove",
      screen->doOpaqueMove() ? "True" : "False"
    );

    putResource(
      &newDatabase,
      "session.imageDither",
      screen->getImageControl()->doDither()
        ? "True"
        : "False"
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "fullMaximization"
      ),
      screen->doFullMax() ? "True" : "False"
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "focusNewWindows"
      ),
      screen->doFocusNew() ? "True" : "False"
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "focusLastWindow"
      ),
      screen->doFocusLast() ? "True" : "False"
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "disableBindingsWithScrollLock"
      ),
      screen->allowScrollLock() ? "True" : "False"
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "rowPlacementDirection"
      ),
      screen->getRowPlacementDirection() ==
        HbScreen::LeftRight
        ? "LeftToRight"
        : "RightToLeft"
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "colPlacementDirection"
      ),
      screen->getColPlacementDirection() ==
        HbScreen::TopBottom
        ? "TopToBottom"
        : "BottomToTop"
    );

    std::string placement;

    switch (screen->getPlacementPolicy()) {
    case HbScreen::CascadePlacement:
      placement = "CascadePlacement";
      break;

    case HbScreen::ColSmartPlacement:
      placement = "ColSmartPlacement";
      break;

    case HbScreen::RowSmartPlacement:
    default:
      placement = "RowSmartPlacement";
      break;
    }

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "windowPlacement"
      ),
      placement
    );

    std::string focusModel;

    if (screen->isSloppyFocus()) {
      focusModel = "SloppyFocus";

      if (screen->doAutoRaise())
        focusModel += " AutoRaise";

      if (screen->doClickRaise())
        focusModel += " ClickRaise";

    } else {
      focusModel = "ClickToFocus";
    }

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "focusModel"
      ),
      focusModel
    );

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "workspaces"
      ),
      std::to_string(screen->getWorkspaceCount())
    );

    load_rc(screen);

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "edgeSnapThreshold"
      ),
      std::to_string(screen->getEdgeSnapThreshold())
    );

    std::string workspaceNames;

    for (unsigned int i = 0;
         i < screen->getWorkspaceCount();
         ++i) {

      if (i != 0)
        workspaceNames += ',';

      workspaceNames +=
        screen->getWorkspace(i)->getName();
    }

    putResource(
      &newDatabase,
      makeResourceName(
        "session.screen",
        screenNumber,
        "workspaceNames"
      ),
      workspaceNames
    );
  }

  if (!newDatabase)
    return;

  XrmDatabase oldDatabase =
    XrmGetFileDatabase(rc_file.c_str());

  XrmMergeDatabases(
    newDatabase,
    &oldDatabase
  );

  XrmPutFileDatabase(
    oldDatabase,
    rc_file.c_str()
  );

  if (oldDatabase)
    XrmDestroyDatabase(oldDatabase);
}


void Hackedbox::load_rc() {
  XrmDatabase database =
    XrmGetFileDatabase(rc_file.c_str());

  if (!database)
    return;

  XrmValue value{};
  char *valueType = nullptr;

  if (XrmGetResource(
        database,
        "session.menuFile",
        "Session.MenuFile",
        &valueType,
        &value)) {

    resource.menu_file =
      expandTilde(value.addr);

  } else {
    resource.menu_file = DEFAULTMENU;
  }

  resource.colors_per_channel = 4;

  int integerValue = 0;

  if (XrmGetResource(
        database,
        "session.colorsPerChannel",
        "Session.ColorsPerChannel",
        &valueType,
        &value) &&
      std::sscanf(
        value.addr,
        "%d",
        &integerValue
      ) == 1) {

    resource.colors_per_channel = std::clamp(
      integerValue,
      2,
      6
    );
  }

  if (XrmGetResource(
        database,
        "session.styleFile",
        "Session.StyleFile",
        &valueType,
        &value)) {

    resource.style_file =
      expandTilde(value.addr);

  } else {
    resource.style_file = DEFAULTSTYLE;
  }

  resource.double_click_interval = 250;

  unsigned long unsignedValue = 0;

  if (XrmGetResource(
        database,
        "session.doubleClickInterval",
        "Session.DoubleClickInterval",
        &valueType,
        &value) &&
      std::sscanf(
        value.addr,
        "%lu",
        &unsignedValue
      ) == 1) {

    resource.double_click_interval =
      unsignedValue;
  }

  resource.auto_raise_delay.tv_usec = 400;

  if (XrmGetResource(
        database,
        "session.autoRaiseDelay",
        "Session.AutoRaiseDelay",
        &valueType,
        &value) &&
      std::sscanf(
        value.addr,
        "%lu",
        &unsignedValue
      ) == 1) {

    resource.auto_raise_delay.tv_usec =
      static_cast<suseconds_t>(unsignedValue);
  }

  resource.auto_raise_delay.tv_sec =
    resource.auto_raise_delay.tv_usec / 1000;

  resource.auto_raise_delay.tv_usec -=
    resource.auto_raise_delay.tv_sec * 1000;

  resource.auto_raise_delay.tv_usec *= 1000;

  resource.cache_life = 5UL;

  if (XrmGetResource(
        database,
        "session.cacheLife",
        "Session.CacheLife",
        &valueType,
        &value) &&
      std::sscanf(
        value.addr,
        "%lu",
        &unsignedValue
      ) == 1) {

    resource.cache_life = unsignedValue;
  }

  resource.cache_life *= 60000UL;

  resource.cache_max = 200UL;

  if (XrmGetResource(
        database,
        "session.cacheMax",
        "Session.CacheMax",
        &valueType,
        &value) &&
      std::sscanf(
        value.addr,
        "%lu",
        &unsignedValue
      ) == 1) {

    resource.cache_max = unsignedValue;
  }

  // Clock
  resource.clock_enabled = false;

  if (XrmGetResource(
        database,
        "session.clock.enabled",
        "Session.Clock.Enabled",
        &valueType,
        &value)) {

    resource.clock_enabled = resourceIsTrue(value);
  }

  resource.clock_target = "root";

  if (XrmGetResource(
        database,
        "session.clock.target",
        "Session.Clock.Target",
        &valueType,
        &value)) {

    resource.clock_target = value.addr;
  }

  resource.clock_time_format = "%I:%M:%S %p";

  if (XrmGetResource(
        database,
        "session.clock.timeFormat",
        "Session.Clock.TimeFormat",
        &valueType,
        &value)) {

    resource.clock_time_format = value.addr;
  }

  resource.clock_date_format = "%m/%d/%Y";

  if (XrmGetResource(
        database,
        "session.clock.dateFormat",
        "Session.Clock.DateFormat",
        &valueType,
        &value)) {

    resource.clock_date_format = value.addr;
  }

  resource.clock_color = "#00FF00";

  if (XrmGetResource(
        database,
        "session.clock.color",
        "Session.Clock.Color",
        &valueType,
        &value)) {

    resource.clock_color = value.addr;
  }

  resource.clock_x = 20;

  if (XrmGetResource(
        database,
        "session.clock.x",
        "Session.Clock.X",
        &valueType,
        &value) &&
      std::sscanf(value.addr, "%d", &integerValue) == 1) {

    resource.clock_x = integerValue;
  }

  resource.clock_y = 20;

  if (XrmGetResource(
        database,
        "session.clock.y",
        "Session.Clock.Y",
        &valueType,
        &value) &&
      std::sscanf(value.addr, "%d", &integerValue) == 1) {

    resource.clock_y = integerValue;
  }

  XrmDestroyDatabase(database);
}


void Hackedbox::load_rc(HbScreen *screen) {
  XrmDatabase database =
    XrmGetFileDatabase(rc_file.c_str());

  if (!database)
    return;

  XrmValue value{};
  char *valueType = nullptr;

  const int screenNumber =
    screen->getScreenNumber();

  auto resourceLookup =
    [&](std::string_view name,
        std::string_view resourceClass) -> bool {

    const std::string resourceName(name);
    const std::string resourceClassName(resourceClass);

    return XrmGetResource(
      database,
      resourceName.c_str(),
      resourceClassName.c_str(),
      &valueType,
      &value
    );
  };

  screen->saveFullMax(false);

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "fullMaximization"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "FullMaximization"
        )) &&
      resourceIsTrue(value)) {

    screen->saveFullMax(true);
  }

  screen->saveFocusNew(false);

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "focusNewWindows"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "FocusNewWindows"
        )) &&
      resourceIsTrue(value)) {

    screen->saveFocusNew(true);
  }

  screen->saveFocusLast(false);

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "focusLastWindow"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "FocusLastWindow"
        )) &&
      resourceIsTrue(value)) {

    screen->saveFocusLast(true);
  }

  screen->saveAllowScrollLock(false);

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "disableBindingsWithScrollLock"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "DisableBindingsWithScrollLock"
        )) &&
      resourceIsTrue(value)) {

    screen->saveAllowScrollLock(true);
  }

  screen->saveRowPlacementDirection(
    HbScreen::LeftRight
  );

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "rowPlacementDirection"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "RowPlacementDirection"
        ))) {

    const std::string_view direction(
      value.addr,
      value.size
    );

    if (direction == "righttoleft" ||
        direction == "RightToLeft" ||
        direction == "RIGHTTOLEFT") {

      screen->saveRowPlacementDirection(
        HbScreen::RightLeft
      );
    }
  }

  screen->saveColPlacementDirection(
    HbScreen::TopBottom
  );

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "colPlacementDirection"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "ColPlacementDirection"
        ))) {

    const std::string_view direction(
      value.addr,
      value.size
    );

    if (direction == "bottomtotop" ||
        direction == "BottomToTop" ||
        direction == "BOTTOMTOTOP") {

      screen->saveColPlacementDirection(
        HbScreen::BottomTop
      );
    }
  }

  screen->saveWorkspaces(1);

  int integerValue = 0;

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "workspaces"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "Workspaces"
        )) &&
      std::sscanf(
        value.addr,
        "%d",
        &integerValue
      ) == 1 &&
      integerValue > 0 &&
      integerValue < 128) {

    screen->saveWorkspaces(integerValue);
  }

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "workspaceNames"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "WorkspaceNames"
        ))) {

    const std::string names(
      value.addr,
      value.size
    );

    std::size_t start = 0;

    while (start <= names.size()) {
      const std::size_t separator =
        names.find(',', start);

      const std::size_t end =
        separator == std::string::npos
          ? names.size()
          : separator;

      screen->addWorkspaceName(
        names.substr(start, end - start)
      );

      if (separator == std::string::npos)
        break;

      start = separator + 1;
    }
  }

  screen->saveSloppyFocus(true);
  screen->saveAutoRaise(false);
  screen->saveClickRaise(false);

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "focusModel"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "FocusModel"
        ))) {

    const std::string focusModel(
      value.addr,
      value.size
    );

    if (focusModel.find("ClickToFocus") !=
        std::string::npos) {

      screen->saveSloppyFocus(false);

    } else {

      if (focusModel.find("AutoRaise") !=
          std::string::npos) {
        screen->saveAutoRaise(true);
      }

      if (focusModel.find("ClickRaise") !=
          std::string::npos) {
        screen->saveClickRaise(true);
      }
    }
  }

  screen->savePlacementPolicy(
    HbScreen::RowSmartPlacement
  );

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "windowPlacement"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "WindowPlacement"
        ))) {

    const std::string_view placement(
      value.addr,
      value.size
    );

    if (placement == "ColSmartPlacement") {
      screen->savePlacementPolicy(
        HbScreen::ColSmartPlacement
      );

    } else if (placement == "CascadePlacement") {
      screen->savePlacementPolicy(
        HbScreen::CascadePlacement
      );
    }
  }

  screen->saveEdgeSnapThreshold(0);

  if (resourceLookup(
        makeResourceName(
          "session.screen",
          screenNumber,
          "edgeSnapThreshold"
        ),
        makeResourceClass(
          "Session.Screen",
          screenNumber,
          "EdgeSnapThreshold"
        )) &&
      std::sscanf(
        value.addr,
        "%d",
        &integerValue
      ) == 1) {

    screen->saveEdgeSnapThreshold(integerValue);
  }

  screen->saveImageDither(true);

  if (XrmGetResource(
        database,
        "session.imageDither",
        "Session.ImageDither",
        &valueType,
        &value) &&
      resourceIsFalse(value)) {

    screen->saveImageDither(false);
  }

  screen->saveOpaqueMove(false);

  if (XrmGetResource(
        database,
        "session.opaqueMove",
        "Session.OpaqueMove",
        &valueType,
        &value) &&
      resourceIsTrue(value)) {

    screen->saveOpaqueMove(true);
  }

  XrmDestroyDatabase(database);
}


void Hackedbox::reload_rc() {
  load_rc();

  if (clock) {
    clock->setTimeFormat(resource.clock_time_format);
    clock->setDateFormat(resource.clock_date_format);
    clock->setColor(resource.clock_color);
    clock->setPosition(resource.clock_x, resource.clock_y);

    const bool root =
      resource.clock_target == "root" ||
      resource.clock_target == "both";

    clock->setRoot(root);

    if (resource.clock_enabled) {
      clock->update();

      if (root)
        clock->drawRoot();

      if (clock_timer &&
          !clock_timer->isTiming()) {
        clock_timer->start();
      }

    } else if (clock_timer) {
      clock_timer->stop();
    }
  }

  reconfigure();
}


void Hackedbox::reconfigure() {
  reconfigure_wait = true;

  if (!timer->isTiming())
    timer->start();
}


void Hackedbox::real_reconfigure() {
  XrmDatabase newDatabase = nullptr;

  fprintf(stderr,
          "Hackedbox: reconfigure style = %s\n",
          resource.style_file.c_str());

  const std::string styleResource =
    "session.styleFile: " + resource.style_file;

  XrmPutLineResource(
    &newDatabase,
    styleResource.c_str()
  );

  XrmDatabase oldDatabase =
    XrmGetFileDatabase(rc_file.c_str());

  XrmMergeDatabases(
    newDatabase,
    &oldDatabase
  );

  XrmPutFileDatabase(
    oldDatabase,
    rc_file.c_str()
  );

  if (oldDatabase)
    XrmDestroyDatabase(oldDatabase);

  for (auto *timestamp : menuTimestamps)
    delete timestamp;

  menuTimestamps.clear();

  gcCache()->purge();

  for (auto *screen : screenList)
    screen->reconfigure();
}


void Hackedbox::checkMenu() {
  bool reread = false;

  for (auto *timestamp : menuTimestamps) {
    std::error_code error;

    const auto currentTime =
      fs::last_write_time(
        timestamp->filename,
        error
      );

    if (error || currentTime != timestamp->timestamp) {
      reread = true;
      break;
    }
  }

  if (reread)
    rereadMenu();
}


void Hackedbox::rereadMenu() {
  reread_menu_wait = true;

  if (!timer->isTiming())
    timer->start();
}


void Hackedbox::real_rereadMenu() {
  for (auto *timestamp : menuTimestamps)
    delete timestamp;

  menuTimestamps.clear();

  for (auto *screen : screenList)
    screen->rereadMenu();
}


void Hackedbox::saveStyleFilename(
  const std::string& filename) {

  if (filename.empty())
    return;

  resource.style_file = filename;
}


void Hackedbox::saveMenuFilename(
  const std::string& filename) {

  if (filename.empty())
    return;

  const auto found =
    std::find_if(
      menuTimestamps.begin(),
      menuTimestamps.end(),
      [&](const auto *timestamp) {
        return timestamp->filename == filename;
      }
    );

  if (found != menuTimestamps.end())
    return;

  std::error_code error;

  const auto timestamp =
    fs::last_write_time(
      filename,
      error
    );

  if (error)
    return;

  auto *entry = new MenuTimestamp;

  entry->filename = filename;
  entry->timestamp = timestamp;

  menuTimestamps.push_back(entry);
}


void Hackedbox::timeout() {
  if (reconfigure_wait)
    real_reconfigure();

  if (reread_menu_wait)
    real_rereadMenu();

  reconfigure_wait = false;
  reread_menu_wait = false;

  if (!clock ||
      !resource.clock_enabled)
    return;

  clock->update();

  if (resource.clock_target == "root" ||
      resource.clock_target == "both") {
    clock->drawRoot();
  }

  if (resource.clock_target == "menu" ||
      resource.clock_target == "both") {

    std::set<HbBasemenu *> menus;

    for (const auto &entry : menuSearchList) {
      if (entry.second)
        menus.insert(entry.second);
    }

    for (auto *menu : menus)
      menu->redrawClock();
  }
}


void Hackedbox::setFocusedWindow(HackedboxWindow *window) {
  if (focused_window &&
      focused_window == window) {
    return;
  }

  if (focused_window)
    focused_window->setFocusFlag(False);

  if (window && !window->isIconic()) {
    active_screen = window->getScreen();
    focused_window = window;
  } else {
    focused_window = nullptr;
  }
}
