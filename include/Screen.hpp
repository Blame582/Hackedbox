// Screen.hpp for Hackedbox - an XLibre Window manager
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

#ifndef __Screen_hpp
#define __Screen_hpp

#include <X11/Xresource.h>

#include <sys/time.h>

#include <cstdio>
#include <list>
#include <string>
#include <vector>

#include "Color.hpp"
#include "ConfigMenu.hpp"
#include "Texture.hpp"
#include "IconMenu.hpp"
#include "Netizen.hpp"
#include "RootMenu.hpp"
#include "Workspace.hpp"
#include "WorkspaceMenu.hpp"
#include "Hackedbox.hpp"
#include "Font.hpp"

class Slit;
class MenuManager;

// Text alignment
enum TextJustify {
  LeftJustify = 1,
  RightJustify,
  CenterJustify
};


struct WindowStyle {
  HbColor l_text_focus;
  HbColor l_text_unfocus;
  HbColor b_pic_focus;
  HbColor b_pic_unfocus;

  HbTexture f_focus;
  HbTexture f_unfocus;
  HbTexture t_focus;
  HbTexture t_unfocus;
  HbTexture l_focus;
  HbTexture l_unfocus;
  HbTexture h_focus;
  HbTexture h_unfocus;
  HbTexture b_focus;
  HbTexture b_unfocus;
  HbTexture b_pressed;
  HbTexture g_focus;
  HbTexture g_unfocus;

  XFontSet fontset;
  XFontSetExtents *fontset_extents;
  HbFont *font;

  TextJustify justify;

  int doJustify(const char *text,
                int &start_pos,
                unsigned int max_length,
                unsigned int modifier,
                bool multibyte) const;
};


struct MenuStyle {
  HbColor t_text;
  HbColor f_text;
  HbColor h_text;
  HbColor d_text;

  HbTexture title;
  HbTexture frame;
  HbTexture hilite;
  HbTexture sel;

  XFontSet t_fontset;
  XFontSet f_fontset;

  XFontSetExtents *t_fontset_extents;
  XFontSetExtents *f_fontset_extents;

  HbFont *t_font;
  HbFont *f_font;

  TextJustify t_justify;
  TextJustify f_justify;

  int bullet;
  int bullet_pos;
};


struct Strut {
  unsigned int top;
  unsigned int bottom;
  unsigned int left;
  unsigned int right;

  Strut(void)
    : top(0),
      bottom(0),
      left(0),
      right(0) {}
};


class HbScreen : public ScreenInfo {
private:
  bool root_colormap_installed;
  bool managed;
  bool geom_visible;

  GC opGC;
  Pixmap geom_pixmap;
  Window geom_window;

  Hackedbox *hackedbox;
  HbImageControl *image_control;
  Configmenu *configmenu;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  typedef std::list<Rootmenu*> RootmenuList;
  RootmenuList rootmenuList;

  typedef std::list<Netizen*> NetizenList;
  NetizenList netizenList;

  HackedboxWindowList iconList;
  HackedboxWindowList windowList;

  Workspace *current_workspace;
  Workspacemenu *workspacemenu;

  unsigned int geom_w;
  unsigned int geom_h;
  unsigned long event_mask;

  Rect usableArea;

  typedef std::list<Strut*> StrutList;
  StrutList strutList;

  typedef std::vector<std::string> WorkspaceNamesList;
  WorkspaceNamesList workspaceNames;

  typedef std::vector<Workspace*> WorkspaceList;
  WorkspaceList workspacesList;

  struct screen_resource {
    WindowStyle wstyle;
    MenuStyle mstyle;

    bool sloppy_focus;
    bool auto_raise;
    bool auto_edge_balance;
    bool image_dither;
    bool ordered_dither;
    bool opaque_move;
    bool full_max;
    bool focus_new;
    bool focus_last;
    bool click_raise;
    bool allow_scroll_lock;

    HbColor border_color;
    XrmDatabase stylerc;

    unsigned int workspaces;

    int placement_policy;
    int edge_snap_threshold;
    int row_direction;
    int col_direction;

    unsigned int handle_width;
    unsigned int bevel_width;
    unsigned int frame_width;
    unsigned int border_width;

    std::string backgroundFolder;
    int backgroundTimer;

#ifdef HAVE_STRFTIME
    std::string strftime_format;
#else
    bool clock24hour;
    int date_format;
#endif

  } resource;

  HbScreen(const HbScreen&);
  HbScreen& operator=(const HbScreen&);

  HbTexture readDatabaseTexture(const std::string &rname,
                                const std::string &rclass,
                                const std::string &default_color);

  HbColor readDatabaseColor(const std::string &rname,
                            const std::string &rclass,
                            const std::string &default_color);

  XFontSet readDatabaseFontSet(const std::string &rname,
                               const std::string &rclass);

  HbFont *readDatabaseFont(const std::string &rname,
                           const std::string &rclass);

  XFontSet createFontSet(const std::string &fontname);

  void InitMenu(void);
  void LoadStyle(void);


public:
  enum {
    RowSmartPlacement = 1,
    ColSmartPlacement,
    CascadePlacement,
    LeftRight,
    RightLeft,
    TopBottom,
    BottomTop
  };

  enum {
    RoundBullet = 1,
    TriangleBullet,
    SquareBullet,
    NoBullet
  };

  enum {
    Restart = 1,
    RestartOther,
    Exit,
    Shutdown,
    Execute,
    Runbox,
    Reconfigure,
    WindowShade,
    WindowIconify,
    WindowMaximize,
    WindowClose,
    WindowRaise,
    WindowLower,
    WindowStick,
    WindowKill,
    SetStyle
  };

  enum FocusModel {
    SloppyFocus,
    ClickToFocus
  };

  HbScreen(Hackedbox *hb, unsigned int scrn);
  ~HbScreen(void);

  inline bool isSloppyFocus(void) const
  {
    return resource.sloppy_focus;
  }

  inline bool isRootColormapInstalled(void) const
  {
    return root_colormap_installed;
  }

  inline bool doAutoRaise(void) const
  {
    return resource.auto_raise;
  }

  inline bool doClickRaise(void) const
  {
    return resource.click_raise;
  }

  inline bool isScreenManaged(void) const
  {
    return managed;
  }

  inline bool doImageDither(void) const
  {
    return resource.image_dither;
  }

  inline bool doOrderedDither(void) const
  {
    return resource.ordered_dither;
  }

  inline bool doOpaqueMove(void) const
  {
    return resource.opaque_move;
  }

  inline bool doFullMax(void) const
  {
    return resource.full_max;
  }

  inline bool doFocusNew(void) const
  {
    return resource.focus_new;
  }

  inline bool doFocusLast(void) const
  {
    return resource.focus_last;
  }

  inline bool allowScrollLock(void) const
  {
    return resource.allow_scroll_lock;
  }

  inline const GC &getOpGC(void) const
  {
    return opGC;
  }

  inline Hackedbox *getHackedbox(void)
  {
    return hackedbox;
  }

  inline HbColor *getBorderColor(void)
  {
    return &resource.border_color;
  }

  inline HbImageControl *getImageControl(void)
  {
    return image_control;
  }

  inline Rootmenu *getRootmenu(void)
  {
    return rootmenu;
  }

  inline Configmenu *getConfigmenu(void)
  {
    return configmenu;
  }

  inline void addRootmenu(Rootmenu *menu)
  {
    if (menu)
      rootmenuList.push_back(menu);
  }

  Workspace *getWorkspace(unsigned int index);

  inline Workspace *getCurrentWorkspace(void)
  {
    return current_workspace;
  }

  inline Workspacemenu *getWorkspacemenu(void)
  {
    return workspacemenu;
  }

  inline unsigned int getHandleWidth(void) const
  {
    return resource.handle_width;
  }

  inline unsigned int getBevelWidth(void) const
  {
    return resource.bevel_width;
  }

  inline unsigned int getFrameWidth(void) const
  {
    return resource.frame_width;
  }

  inline unsigned int getBorderWidth(void) const
  {
    return resource.border_width;
  }

  inline unsigned int getCurrentWorkspaceID(void) const
  {
    return current_workspace->getID();
  }

  inline unsigned int getWorkspaceCount(void) const
  {
    return workspacesList.size();
  }

  inline unsigned int getIconCount(void) const
  {
    return iconList.size();
  }

  inline unsigned int getNumberOfWorkspaces(void) const
  {
    return resource.workspaces;
  }

  inline int getPlacementPolicy(void) const
  {
    return resource.placement_policy;
  }

  inline int getEdgeSnapThreshold(void) const
  {
    return resource.edge_snap_threshold;
  }

  inline int getRowPlacementDirection(void) const
  {
    return resource.row_direction;
  }

  inline int getColPlacementDirection(void) const
  {
    return resource.col_direction;
  }

  inline void setRootColormapInstalled(bool r)
  {
    root_colormap_installed = r;
  }

  inline void saveSloppyFocus(bool s)
  {
    resource.sloppy_focus = s;
  }

  inline void saveAutoRaise(bool a)
  {
    resource.auto_raise = a;
  }

  inline void saveClickRaise(bool c)
  {
    resource.click_raise = c;
  }

  inline void saveWorkspaces(unsigned int w)
  {
    resource.workspaces = w;
  }

  inline void savePlacementPolicy(int p)
  {
    resource.placement_policy = p;
  }

  inline void saveRowPlacementDirection(int d)
  {
    resource.row_direction = d;
  }

  inline void saveColPlacementDirection(int d)
  {
    resource.col_direction = d;
  }

  inline void saveEdgeSnapThreshold(int t)
  {
    resource.edge_snap_threshold = t;
  }

  inline void saveImageDither(bool d)
  {
    resource.image_dither = d;
  }

  inline void saveOpaqueMove(bool o)
  {
    resource.opaque_move = o;
  }

  inline void saveFullMax(bool f)
  {
    resource.full_max = f;
  }

  inline void saveFocusNew(bool f)
  {
    resource.focus_new = f;
  }

  inline void saveFocusLast(bool f)
  {
    resource.focus_last = f;
  }

  inline void saveAllowScrollLock(bool a)
  {
    resource.allow_scroll_lock = a;
  }

  inline void iconUpdate(void)
  {
    iconmenu->update();
  }

  /*
#ifdef HAVE_STRFTIME
  inline const char *getStrftimeFormat(void)
  { return resource.strftime_format.c_str(); }

  void saveStrftimeFormat(const std::string& format);

#else
  inline int getDateFormat(void)
  { return resource.date_format; }

  inline void saveDateFormat(int f)
  { resource.date_format = f; }

  inline bool isClock24Hour(void)
  { return resource.clock24hour; }

  inline void saveClock24Hour(bool c)
  { resource.clock24hour = c; }

#endif
  */

  inline WindowStyle *getWindowStyle(void)
  {
    return &resource.wstyle;
  }

  inline MenuStyle *getMenuStyle(void)
  {
    return &resource.mstyle;
  }

  HackedboxWindow *getIcon(unsigned int index);

  const Rect& availableArea(void) const;
  void updateAvailableArea(void);

  void addStrut(Strut *strut);
  void removeStrut(Strut *strut);

  unsigned int addWorkspace(void);
  unsigned int removeLastWorkspace(void);

  void removeWorkspaceNames(void);
  void addWorkspaceName(const std::string& name);
  const std::string getNameOfWorkspace(unsigned int id);

  void changeWorkspaceID(unsigned int id);

  void addNetizen(Netizen *n);
  void removeNetizen(Window w);

  void addIcon(HackedboxWindow *w);
  void removeIcon(HackedboxWindow *w);

  void manageWindow(Window w);
  void unmanageWindow(HackedboxWindow *w, bool remap);

  void raiseWindows(Window *workspace_stack, unsigned int num);

  void reassociateWindow(HackedboxWindow *w,
                         unsigned int wkspc_id,
                         bool ignore_sticky);

  void propagateWindowName(const HackedboxWindow *bw);

  void prevFocus(void);
  void nextFocus(void);
  void raiseFocus(void);

  void reconfigure(void);
  void toggleFocusModel(FocusModel model);
  void rereadMenu(void);
  void shutdown(void);

  void showPosition(int x, int y);
  void showGeometry(unsigned int gx, unsigned int gy);
  void hideGeometry(void);

  void buttonPressEvent(const XButtonEvent *xbutton);

  void updateNetizenCurrentWorkspace(void);
  void updateNetizenWorkspaceCount(void);
  void updateNetizenWindowFocus(void);

  void updateNetizenWindowAdd(Window w, unsigned long p);
  void updateNetizenWindowDel(Window w);
  void updateNetizenConfigNotify(XEvent *e);
  void updateNetizenWindowRaise(Window w);
  void updateNetizenWindowLower(Window w);
};

#endif // __Screen_hpp
