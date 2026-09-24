// Basemenu.hpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// look in the Authors file for credits and copyrights
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

#ifndef HACKEDBOX_BASEMENU_HPP
#define HACKEDBOX_BASEMENU_HPP

#include <X11/Xlib.h>

#include "Hackedbox.hpp"

#include <string>
#include <vector>

class Hackedbox;
class HbImageControl;
class HbScreen;
class HbBasemenu;
class HbBasemenuItem;


class HbBasemenu {
private:
  typedef std::vector<HbBasemenuItem*> MenuItems;
  MenuItems menuitems;

  Hackedbox *hackedbox;
  HbBasemenu *parent;
  HbImageControl *image_ctrl;
  HbScreen *screen;

  bool moving;
  bool visible;
  bool movable;
  bool torn;
  bool internal_menu;
  bool title_vis;
  bool shifted;
  bool hide_tree;

  /*
   * Menu-specific ARGB visual state.
   *
   * Normal Hackedbox windows continue to use the visual, depth, and
   * colormap supplied by ScreenInfo. These values are used only when
   * a menu needs an ARGB visual for translucent rendering.
   */
  Visual *menu_visual;
  Colormap menu_colormap;
  int menu_depth;
  bool menu_argb;

  Display *display;

  int which_sub;
  int which_press;
  int which_sbl;
  int alignment;
  int clock_item;
  int date_item;

  struct _menu {
    Pixmap frame_pixmap;
    Pixmap title_pixmap;
    Pixmap hilite_pixmap;
    Pixmap sel_pixmap;

    Window window;
    Window frame;
    Window title;

    std::string label;

    int x;
    int y;
    int x_move;
    int y_move;
    int x_shift;
    int y_shift;

    int sublevels;
    int persub;
    int minsub;

    unsigned int width;
    unsigned int height;
    unsigned int title_h;
    unsigned int frame_h;

    unsigned int item_w;
    unsigned int item_h;
    unsigned int bevel_w;
    unsigned int bevel_h;
  } menu;

protected:
  inline void setTitleVisibility(bool b)
    { title_vis = b; }

  inline void setMovable(bool b)
    { movable = b; }

  inline void setHideTree(bool h)
    { hide_tree = h; }

  inline void setMinimumSublevels(int m)
    { menu.minsub = m; }

  virtual void itemSelected(int button, unsigned int index) = 0;

  virtual void drawItem(int index,
                        bool highlight = False,
                        bool clear = False,
                        int x = -1,
                        int y = -1,
                        unsigned int w = 0,
                        unsigned int h = 0);

  virtual void redrawTitle(void);
  virtual void internal_hide(void);

public:
  HbBasemenu(const HbBasemenu &) = delete;
  HbBasemenu(HbBasemenu &&) = delete;

  HbBasemenu &operator=(const HbBasemenu &) = delete;
  HbBasemenu &operator=(HbBasemenu &&) = delete;

  HbBasemenu(HbScreen *scrn);
  virtual ~HbBasemenu(void);

  inline bool isTorn(void) const
    { return torn; }

  inline bool isVisible(void) const
    { return visible; }

  inline HbScreen *getScreen(void)
    { return screen; }

  inline Window getWindowID(void) const
    { return menu.window; }

  inline const char *getLabel(void) const
    { return menu.label.c_str(); }

int insert(HbBasemenuItem *item, int pos);

int insert(const std::string& label,
           int function = 0,
           const std::string& exec = "",
           int pos = -1);

int insert(const std::string& label,
           int function,
           const std::string& exec,
           const std::string& icon,
           int pos = -1);

int insert(const std::string& label,
           HbBasemenu *submenu,
           int pos = -1);

int insert(const std::string& label,
           HbBasemenu *submenu,
           const std::string& icon,
           int pos = -1);

int remove(int index);

  void changeItemLabel(unsigned int index,
                       const std::string& label);

  inline int getX(void) const
    { return menu.x; }

  inline int getY(void) const
    { return menu.y; }

  inline unsigned int getCount(void) const
    { return menuitems.size(); }

  inline int getCurrentSubmenu(void) const
    { return which_sub; }

  inline unsigned int getWidth(void) const
    { return menu.width; }

  inline unsigned int getHeight(void) const
    { return menu.height; }

  inline unsigned int getTitleHeight(void) const
    { return menu.title_h; }

  inline void setInternalMenu(void)
    { internal_menu = True; }

  inline void setAlignment(int a)
    { alignment = a; }

  inline void setTorn(void)
    { torn = True; }

  inline void removeParent(void)
    {
      if (internal_menu)
        parent = (HbBasemenu *) 0;
    }

  bool hasSubmenu(int index);
  bool isItemSelected(int index);
  bool isItemEnabled(int index);

  HbBasemenuItem *find(int index);

  void buttonPressEvent(XButtonEvent *be);
  void buttonReleaseEvent(XButtonEvent *re);
  void motionNotifyEvent(XMotionEvent *me);
  void enterNotifyEvent(XCrossingEvent *ce);
  void leaveNotifyEvent(XCrossingEvent *ce);
  void exposeEvent(XExposeEvent *ee);
  void reconfigure(void);

  void redrawClock();
  void setClockItem(int index);
  void setDateItem(int index);

  void setLabel(const std::string& label);
  void move(int x, int y);
  void update(void);

  void setItemSelected(int index, bool sel);
  void setItemEnabled(int index, bool enable);

  virtual void drawSubmenu(int index);
  virtual void show(void);
  virtual void hide(void);

  enum {
    AlignDontCare = 1,
    AlignTop,
    AlignBottom
  };

  enum {
    Right = 1,
    Left
  };

  enum {
    Empty = 0,
    Square,
    Triangle,
    Diamond
  };
};


class HbBasemenuItem {
private:
  HbBasemenu *sub;

  std::string l;
  std::string e;
  std::string i;

  int f;
  int enabled;
  int selected;

public:
  HbBasemenuItem(const std::string& lp,
                 int fp = 0,
                 const std::string& ep = "",
                 const std::string& ip = ""):
    sub(0),
    l(lp),
    e(ep),
    i(ip),
    f(fp),
    enabled(1),
    selected(0) {}

HbBasemenuItem(const std::string& lp,
               HbBasemenu *mp,
               const std::string& ip = ""):
  sub(mp),
  l(lp),
  i(ip),
  f(0),
  enabled(1),
  selected(0) {}

  inline const char *exec(void) const
    { return e.c_str(); }

  inline const char *label(void) const
    { return l.c_str(); }

  inline const char *icon(void) const
    { return i.c_str(); }

  inline int function(void) const
    { return f; }

  inline HbBasemenu *submenu(void)
    { return sub; }

  inline void newLabel(const std::string& label)
    { l = label; }

  inline int isEnabled(void) const
    { return enabled; }

  inline void setEnabled(int e)
    { enabled = e; }

  inline int isSelected(void) const
    { return selected; }

  inline void setSelected(int s)
    { selected = s; }
};

#endif // HACKEDBOX_BASEMENU_HPP