// hackedbox.hpp for Hackedbox - an XLibre Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef HACKEDBOX_HPP
#define HACKEDBOX_HPP

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <sys/time.h>

#include <list>
#include <map>
#include <string>
#include <filesystem>

#include "BaseDisplay.hpp"
#include "Timer.hpp"
#include "Clock.hpp"

#define AttribShaded      (1UL << 0)
#define AttribMaxHoriz    (1UL << 1)
#define AttribMaxVert     (1UL << 2)
#define AttribOmnipresent (1UL << 3)
#define AttribWorkspace   (1UL << 4)
#define AttribStack       (1UL << 5)
#define AttribDecoration  (1UL << 6)

#define StackTop          0
#define StackNormal       1
#define StackBottom       2

#define DecorNone         0
#define DecorNormal       1
#define DecorTiny         2
#define DecorTool         3

namespace fs = std::filesystem;

struct HackedboxHints {
  unsigned long flags;
  unsigned long attrib;
  unsigned long workspace;
  unsigned long stack;
  unsigned long decoration;
};

struct HackedboxAttributes {
  unsigned long flags;
  unsigned long attrib;
  unsigned long workspace;
  unsigned long stack;
  unsigned long decoration;

  int premax_x;
  int premax_y;

  unsigned int premax_w;
  unsigned int premax_h;
};

#define PropHackedboxHintsElements      5
#define PropHackedboxAttributesElements 9

class Hackedbox;
class HbScreen;
class HackedboxWindow;
class HbWindowGroup;
class HbBasemenu;

class Hackedbox : public BaseDisplay, public TimeoutHandler {
private:
  struct HbCursor {
    Cursor session;
    Cursor move;
    Cursor ll_angle;
    Cursor lr_angle;
  };

  struct MenuTimestamp {
    std::string filename;
    fs::file_time_type timestamp;
  };

  struct HbResource {
    Time double_click_interval;

    std::string menu_file;
    std::string style_file;

    int colors_per_channel;

    timeval auto_raise_delay;

    unsigned long cache_life;
    unsigned long cache_max;

    bool clock_enabled;
    std::string clock_target;
    std::string clock_time_format;
    std::string clock_date_format;
    std::string clock_color;

    int clock_x;
    int clock_y;
  };

  HbCursor cursor;
  HbResource resource;

  using WindowLookup = std::map<Window, HackedboxWindow *>;
  using GroupLookup = std::map<Window, HbWindowGroup *>;
  using MenuLookup = std::map<Window, HbBasemenu *>;
  using MenuTimestampList = std::list<MenuTimestamp *>;
  using ScreenList = std::list<HbScreen *>;

  WindowLookup windowSearchList;
  GroupLookup groupSearchList;
  MenuLookup menuSearchList;

  MenuTimestampList menuTimestamps;
  ScreenList screenList;

  HbScreen *active_screen;
  HackedboxWindow *focused_window;
  HbTimer *timer;
  Clock *clock;
  HbTimer *clock_timer;

  bool no_focus;
  bool reconfigure_wait;
  bool reread_menu_wait;

  Time last_time;

  char **argv;

  std::filesystem::path rc_file;

  Atom xa_wm_colormap_windows;
  Atom xa_wm_protocols;
  Atom xa_wm_state;
  Atom xa_wm_delete_window;
  Atom xa_wm_take_focus;
  Atom xa_wm_change_state;
  Atom motif_wm_hints;

  Atom hackedbox_attributes;
  Atom hackedbox_change_attributes;
  Atom hackedbox_hints;

#ifdef HAVE_GETPID
  Atom hackedbox_pid;
#endif

  Atom hackedbox_structure_messages;
  Atom hackedbox_notify_startup;
  Atom hackedbox_notify_window_add;
  Atom hackedbox_notify_window_del;
  Atom hackedbox_notify_window_focus;
  Atom hackedbox_notify_current_workspace;
  Atom hackedbox_notify_workspace_count;
  Atom hackedbox_notify_window_raise;
  Atom hackedbox_notify_window_lower;

  Atom hackedbox_change_workspace;
  Atom hackedbox_change_window_focus;
  Atom hackedbox_cycle_window_focus;

#ifdef NEWWMSPEC
  Atom net_supported;
  Atom net_client_list;
  Atom net_client_list_stacking;
  Atom net_number_of_desktops;
  Atom net_desktop_geometry;
  Atom net_desktop_viewport;
  Atom net_current_desktop;
  Atom net_desktop_names;
  Atom net_active_window;
  Atom net_workarea;
  Atom net_supporting_wm_check;
  Atom net_virtual_roots;

  Atom net_close_window;
  Atom net_wm_moveresize;

  Atom net_properties;
  Atom net_wm_name;
  Atom net_wm_desktop;
  Atom net_wm_window_type;
  Atom net_wm_state;
  Atom net_wm_strut;
  Atom net_wm_icon_geometry;
  Atom net_wm_icon;
  Atom net_wm_pid;
  Atom net_wm_handled_icons;

  Atom net_wm_ping;
#endif

  Hackedbox(const Hackedbox &) = delete;
  Hackedbox &operator=(const Hackedbox &) = delete;

  void load_rc();
  void save_rc();
  void reload_rc();

  void real_rereadMenu();
  void real_reconfigure();

  void init_icccm();

  void process_event(XEvent *event) override;

public:
  Hackedbox(
    char **m_argv,
    const char *display_name = nullptr,
    const char *rc = nullptr
  );

  ~Hackedbox() override;

  HbBasemenu *searchMenu(Window window);
  HbWindowGroup *searchGroup(Window window);
  HackedboxWindow *searchWindow(Window window);
  HbScreen *searchScreen(Window window);

  void saveMenuSearch(Window window, HbBasemenu *data);
  void saveWindowSearch(Window window, HackedboxWindow *data);
  void saveGroupSearch(Window window, HbWindowGroup *data);

  void removeMenuSearch(Window window);
  void removeWindowSearch(Window window);
  void removeGroupSearch(Window window);

  HackedboxWindow *getFocusedWindow() const {
    return focused_window;
  }

  const Time &getDoubleClickInterval() const {
    return resource.double_click_interval;
  }

  const Time &getLastTime() const {
    return last_time;
  }

  const char *getStyleFilename() const {
    return resource.style_file.c_str();
  }

  const char *getMenuFilename() const {
    return resource.menu_file.c_str();
  }

  int getColorsPerChannel() const {
    return resource.colors_per_channel;
  }

  const timeval &getAutoRaiseDelay() const {
    return resource.auto_raise_delay;
  }

  unsigned long getCacheLife() const {
    return resource.cache_life;
  }

  unsigned long getCacheMax() const {
    return resource.cache_max;
  }

  bool getClockEnabled() const {
    return resource.clock_enabled;
  }

  const std::string &getClockTarget() const {
    return resource.clock_target;
  }

  const std::string &getClockTimeFormat() const {
    return resource.clock_time_format;
  }

  const std::string &getClockDateFormat() const {
    return resource.clock_date_format;
  }

  const std::string &getClockColor() const {
    return resource.clock_color;
  }

  int getClockX() const {
    return resource.clock_x;
  }

  int getClockY() const {
    return resource.clock_y;
  }

  Clock *getClock() const {
    return clock;
  }

  void setNoFocus(bool focus) {
    no_focus = focus;
  }

  Cursor getSessionCursor() const {
    return cursor.session;
  }

  Cursor getMoveCursor() const {
    return cursor.move;
  }

  Cursor getLowerLeftAngleCursor() const {
    return cursor.ll_angle;
  }

  Cursor getLowerRightAngleCursor() const {
    return cursor.lr_angle;
  }

  void setFocusedWindow(HackedboxWindow *window);

  void shutdown();

  void load_rc(HbScreen *screen);

  void saveStyleFilename(const std::string &filename);
  void saveMenuFilename(const std::string &filename);

  void restart(const char *program = nullptr);

  void reconfigure();
  void rereadMenu();
  void checkMenu();

  bool validateWindow(Window window);

  bool handleSignal(int signal) override;
  void timeout() override;

#ifdef HAVE_GETPID
  Atom getHackedboxPidAtom() const {
    return hackedbox_pid;
  }
#endif

  Atom getWMChangeStateAtom() const {
    return xa_wm_change_state;
  }

  Atom getWMStateAtom() const {
    return xa_wm_state;
  }

  Atom getWMDeleteAtom() const {
    return xa_wm_delete_window;
  }

  Atom getWMProtocolsAtom() const {
    return xa_wm_protocols;
  }

  Atom getWMTakeFocusAtom() const {
    return xa_wm_take_focus;
  }

  Atom getWMColormapAtom() const {
    return xa_wm_colormap_windows;
  }

  Atom getMotifWMHintsAtom() const {
    return motif_wm_hints;
  }

  Atom getHackedboxHintsAtom() const {
    return hackedbox_hints;
  }

  Atom getHackedboxAttributesAtom() const {
    return hackedbox_attributes;
  }

  Atom getHackedboxChangeAttributesAtom() const {
    return hackedbox_change_attributes;
  }

  Atom getHackedboxStructureMessagesAtom() const {
    return hackedbox_structure_messages;
  }

  Atom getHackedboxNotifyStartupAtom() const {
    return hackedbox_notify_startup;
  }

  Atom getHackedboxNotifyWindowAddAtom() const {
    return hackedbox_notify_window_add;
  }

  Atom getHackedboxNotifyWindowDelAtom() const {
    return hackedbox_notify_window_del;
  }

  Atom getHackedboxNotifyWindowFocusAtom() const {
    return hackedbox_notify_window_focus;
  }

  Atom getHackedboxNotifyCurrentWorkspaceAtom() const {
    return hackedbox_notify_current_workspace;
  }

  Atom getHackedboxNotifyWorkspaceCountAtom() const {
    return hackedbox_notify_workspace_count;
  }

  Atom getHackedboxNotifyWindowRaiseAtom() const {
    return hackedbox_notify_window_raise;
  }

  Atom getHackedboxNotifyWindowLowerAtom() const {
    return hackedbox_notify_window_lower;
  }

  Atom getHackedboxChangeWorkspaceAtom() const {
    return hackedbox_change_workspace;
  }

  Atom getHackedboxChangeWindowFocusAtom() const {
    return hackedbox_change_window_focus;
  }

  Atom getHackedboxCycleWindowFocusAtom() const {
    return hackedbox_cycle_window_focus;
  }

#ifdef NEWWMSPEC
  Atom getNETSupportedAtom() const {
    return net_supported;
  }

  Atom getNETClientListAtom() const {
    return net_client_list;
  }

  Atom getNETNumberOfDesktopsAtom() const {
    return net_number_of_desktops;
  }

  Atom getNETDesktopGeometryAtom() const {
    return net_desktop_geometry;
  }

  Atom getNETDesktopViewportAtom() const {
    return net_desktop_viewport;
  }

  Atom getNETCurrentDesktopAtom() const {
    return net_current_desktop;
  }

  Atom getNETDesktopNamesAtom() const {
    return net_desktop_names;
  }

  Atom getNETActiveWindowAtom() const {
    return net_active_window;
  }

  Atom getNETWorkareaAtom() const {
    return net_workarea;
  }

  Atom getNETSupportingWMCheckAtom() const {
    return net_supporting_wm_check;
  }

  Atom getNETVirtualRootsAtom() const {
    return net_virtual_roots;
  }

  Atom getNETCloseWindowAtom() const {
    return net_close_window;
  }

  Atom getNETWMMoveResizeAtom() const {
    return net_wm_moveresize;
  }

  Atom getNETPropertiesAtom() const {
    return net_properties;
  }

  Atom getNETWMNameAtom() const {
    return net_wm_name;
  }

  Atom getNETWMDesktopAtom() const {
    return net_wm_desktop;
  }

  Atom getNETWMWindowTypeAtom() const {
    return net_wm_window_type;
  }

  Atom getNETWMStateAtom() const {
    return net_wm_state;
  }

  Atom getNETWMStrutAtom() const {
    return net_wm_strut;
  }

  Atom getNETWMIconGeometryAtom() const {
    return net_wm_icon_geometry;
  }

  Atom getNETWMIconAtom() const {
    return net_wm_icon;
  }

  Atom getNETWMPidAtom() const {
    return net_wm_pid;
  }

  Atom getNETWMHandledIconsAtom() const {
    return net_wm_handled_icons;
  }

  Atom getNETWMPingAtom() const {
    return net_wm_ping;
  }
#endif
};

#endif // HACKEDBOX_HPP
