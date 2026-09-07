// Window.hpp for Hackedbox - an XLibre Window Manager
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

#ifndef HACKEDBOX_WINDOW_HPP
#define HACKEDBOX_WINDOW_HPP


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <string>

#include "Timer.hpp"
#include "Util.hpp"
#include "WindowMenu.hpp"
#include "Screen.hpp"

class BaseDisplay;

#define MwmHintsFunctions     (1l << 0)
#define MwmHintsDecorations   (1l << 1)

#define MwmFuncAll            (1l << 0)
#define MwmFuncResize         (1l << 1)
#define MwmFuncMove           (1l << 2)
#define MwmFuncIconify        (1l << 3)
#define MwmFuncMaximize       (1l << 4)
#define MwmFuncClose          (1l << 5)

#define MwmDecorAll           (1l << 0)
#define MwmDecorBorder        (1l << 1)
#define MwmDecorHandle        (1l << 2)
#define MwmDecorTitle         (1l << 3)
#define MwmDecorMenu          (1l << 4) // not used
#define MwmDecorIconify       (1l << 5)
#define MwmDecorMaximize      (1l << 6)


// This structure only contains 3 elements. The Motif 2.0 structure contains
// 5, but we only need the first 3.
struct MwmHints {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
};

#define PropMwmHintsElements 3


class HbWindowGroup {
private:
  Hackedbox *hackedbox;
  Window group;
  HackedboxWindowList windowList;

public:
  HbWindowGroup(
    Hackedbox *hackedbox,
    Window groupWindow
  );

  ~HbWindowGroup();

  inline Window groupWindow() const {
    return group;
  }

  inline bool empty() const {
    return windowList.empty();
  }

  void addWindow(HackedboxWindow *window) {
    windowList.push_back(window);
  }

  void removeWindow(HackedboxWindow *window) {
    windowList.remove(window);
  }

  /*
   * Find a window on the specified screen. The focused window, if any, is
   * checked first. Otherwise the first matching window found is returned.
   * Transients are returned only if allow_transients is true.
   */
  HackedboxWindow *find(
    HbScreen *screen,
    bool allow_transients = false
  ) const;
};


class HackedboxWindow : public TimeoutHandler {
public:
  enum Function {
    Func_Resize   = (1l << 0),
    Func_Move     = (1l << 1),
    Func_Iconify  = (1l << 2),
    Func_Maximize = (1l << 3),
    Func_Close    = (1l << 4)
  };

  typedef unsigned char FunctionFlags;


  enum Decoration {
    Decor_Titlebar = (1l << 0),
    Decor_Handle   = (1l << 1),
    Decor_Border   = (1l << 2),
    Decor_Iconify  = (1l << 3),
    Decor_Maximize = (1l << 4),
    Decor_Close    = (1l << 5)
  };

  typedef unsigned char DecorationFlags;


private:
  Hackedbox *hackedbox;
  HbScreen *screen;
  HbTimer *timer;
  HackedboxAttributes hackedbox_attrib;

  Time lastButtonPressTime;
  WindowMenu *windowmenu;

  unsigned int window_number;
  unsigned long current_state;


  enum FocusMode {
    F_NoInput = 0,
    F_Passive,
    F_LocallyActive,
    F_GloballyActive
  };

  FocusMode focus_mode;


  struct _flags {
    bool moving;
    bool resizing;
    bool shaded;
    bool visible;
    bool iconic;
    bool focused;
    bool stuck;
    bool modal;
    bool send_focus_message;
    bool shaped;

    unsigned int maximized;
    /*
     * Maximize is special. The number corresponds with a mouse button:
     * 0 = not maximized
     * 1 = Horizontal + Vertical
     * 2 = Vertical
     * 3 = Horizontal
     */
  } flags;


  struct _client {
    Window window;
    Window window_group;

    HackedboxWindow *transient_for;
    HackedboxWindowList transientList;

    std::string title;
    std::string icon_title;

    Rect rect;

    int old_bw;

    unsigned int min_width;
    unsigned int min_height;
    unsigned int max_width;
    unsigned int max_height;
    unsigned int width_inc;
    unsigned int height_inc;

#if 0
    // Not supported at the moment.
    unsigned int min_aspect_x;
    unsigned int min_aspect_y;
    unsigned int max_aspect_x;
    unsigned int max_aspect_y;
#endif

    unsigned int base_width;
    unsigned int base_height;
    unsigned int win_gravity;

    unsigned long initial_state;
    unsigned long normal_hint_flags;
  } client;


  FunctionFlags functions;

  /*
   * What decorations do we have?
   *
   * This is based on the type of the client window as well as user input.
   * The menu is not really decor, but it goes hand in hand with the decor.
   */
  DecorationFlags decorations;


  /*
   * Client window = the application's window.
   *
   * Frame window = the window drawn around the outside of the client window
   * by the window manager which contains items like the titlebar and buttons.
   *
   * Title = the titlebar drawn above the client window. It displays the
   * window's name and any buttons for interacting with the window, such as
   * iconify, maximize, and close.
   *
   * Label = the window in the titlebar where the title is drawn.
   *
   * Buttons = maximize, iconify, close.
   *
   * Handle = the bar drawn at the bottom of the window, which contains the
   * left and right grips used for resizing the window.
   *
   * Grips = the smaller rectangles in the handle, one on each side of it.
   * When clicked and dragged, these resize the window interactively.
   *
   * Border = the line drawn around the outside edge of the frame window,
   * between the title, the bordered client window, and the handle.
   * Also drawn between the grips and the handle.
   */


  struct _frame {
    // u -> unfocused, f -> has focus
    unsigned long ulabel_pixel;
    unsigned long flabel_pixel;
    unsigned long utitle_pixel;
    unsigned long ftitle_pixel;
    unsigned long uhandle_pixel;
    unsigned long fhandle_pixel;
    unsigned long ubutton_pixel;
    unsigned long fbutton_pixel;
    unsigned long pbutton_pixel;
    unsigned long uborder_pixel;
    unsigned long fborder_pixel;
    unsigned long ugrip_pixel;
    unsigned long fgrip_pixel;

    Pixmap ulabel;
    Pixmap flabel;
    Pixmap utitle;
    Pixmap ftitle;
    Pixmap uhandle;
    Pixmap fhandle;
    Pixmap ubutton;
    Pixmap fbutton;
    Pixmap pbutton;
    Pixmap ugrip;
    Pixmap fgrip;


    Window window;
    Window plate;
    Window title;
    Window label;
    Window handle;

    Window close_button;
    Window iconify_button;
    Window maximize_button;

    Window right_grip;
    Window left_grip;


    /*
     * Size and location of the box drawn while the window dimensions or
     * location is being changed, i.e. resized or moved.
     */
    Rect changing;

    Rect rect;
    Strut margin;

    int grab_x;
    int grab_y;

    unsigned int inside_w;
    unsigned int inside_h;
    unsigned int title_h;
    unsigned int label_w;
    unsigned int label_h;
    unsigned int handle_h;
    unsigned int button_w;
    unsigned int grip_w;
    unsigned int mwm_border_w;
    unsigned int border_w;
    unsigned int bevel_w;
  } frame;


  struct _icon {
    // u -> unfocused, f -> has focus
    Pixmap ulabel;
    Pixmap flabel;
    Pixmap ubutton;
    Pixmap fbutton;
    Pixmap pbutton;
    Pixmap pixmap;

    Window window;
    Window title;
    Window label;
  } icon;


  HackedboxWindow(const HackedboxWindow&);
  HackedboxWindow& operator=(const HackedboxWindow&);


  bool getState();

  Window createToplevelWindow();

  Window createChildWindow(
    Window parent,
    unsigned long event_mask,
    Cursor cursor = None
  );


  void getWMName();
  void getWMIconName();
  void getWMNormalHints();
  void getWMProtocols();
  void getWMHints();
  void getMWMHints();
  bool getHackedboxHints();
  void getTransientInfo();

  void setNetWMAttributes();

  void associateClientWindow();

  void decorate();
  void decorateLabel();

  void positionButtons(
    bool redecorate_label = false
  );

  void positionWindows();

  void createHandle();
  void destroyHandle();

  void createTitlebar();
  void destroyTitlebar();

  void createCloseButton();
  void destroyCloseButton();

  void createIconifyButton();
  void destroyIconifyButton();

  void createMaximizeButton();
  void destroyMaximizeButton();

  void redrawWindowFrame() const;
  void redrawLabel() const;
  void redrawAllButtons() const;

  void redrawCloseButton(bool pressed) const;
  void redrawIconifyButton(bool pressed) const;
  void redrawMaximizeButton(bool pressed) const;

  void applyGravity(Rect &rect);
  void restoreGravity(Rect &rect);

  void setState(unsigned long new_state);

  void upsize();
  void showIcon();


  enum Corner {
    TopLeft,
    TopRight
  };

  void constrain(
    Corner anchor,
    unsigned int *width = nullptr,
    unsigned int *height = nullptr
  );


public:
  HackedboxWindow(
    Hackedbox *hackedbox,
    Window window,
    HbScreen *screen
  );

  virtual ~HackedboxWindow();


  inline bool isTransient() const {
    return client.transient_for != nullptr;
  }

  inline bool isFocused() const {
    return flags.focused;
  }

  inline bool isVisible() const {
    return flags.visible;
  }

  inline bool isIconic() const {
    return flags.iconic;
  }

  inline bool isShaded() const {
    return flags.shaded;
  }

  inline bool isMaximized() const {
    return flags.maximized;
  }

  inline bool isModal() const {
    return flags.modal;
  }

  inline bool isStuck() const {
    return flags.stuck;
  }

  inline bool isIconifiable() const {
    return functions & Func_Iconify;
  }

  inline bool isMaximizable() const {
    return functions & Func_Maximize;
  }

  inline bool isResizable() const {
    return functions & Func_Resize;
  }

  inline bool isClosable() const {
    return functions & Func_Close;
  }

  inline bool hasTitlebar() const {
    return decorations & Decor_Titlebar;
  }


  inline const HackedboxWindowList &getTransients() const {
    return client.transientList;
  }

  HackedboxWindow *getTransientFor() const;


  inline HbScreen *getScreen() const {
    return screen;
  }


  inline Window getFrameWindow() const {
    return frame.window;
  }

  inline Window getClientWindow() const {
    return client.window;
  }

  inline Window getGroupWindow() const {
    return client.window_group;
  }


  inline WindowMenu *getWindowMenu() const {
    return windowmenu;
  }


  inline const char *getTitle() const {
    return client.title.c_str();
  }

  inline const char *getIconTitle() const {
    return client.icon_title.c_str();
  }


  inline unsigned int getWorkspaceNumber() const {
    return hackedbox_attrib.workspace;
  }

  inline unsigned int getWindowNumber() const {
    return window_number;
  }


  inline const Rect &frameRect() const {
    return frame.rect;
  }

  inline const Rect &clientRect() const {
    return client.rect;
  }


  inline unsigned int getTitleHeight() const {
    return frame.title_h;
  }


  inline void setWindowNumber(int number) {
    window_number = static_cast<unsigned int>(number);
  }

  inline void setModal(bool flag) {
    flags.modal = flag;
  }


  bool validateClient() const;
  bool setInputFocus();

  void setFocusFlag(bool focus);

  void iconify();
  void deiconify(
    bool reassoc = true,
    bool raise = true
  );

  void show();
  void close();
  void withdraw();

  void maximize(unsigned int button);
  void remaximize();

  void shade();
  void stick();
  void reconfigure();

  void grabButtons();
  void ungrabButtons();

  void installColormap(bool install);
  void restore(bool remap);

  void configure(
    int dx,
    int dy,
    unsigned int dw,
    unsigned int dh
  );

  void setWorkspace(unsigned int number);

  void changeHackedboxHints(
    const HackedboxHints *net
  );

  void restoreAttributes();


  void buttonPressEvent(
    const XButtonEvent *event
  );

  void buttonReleaseEvent(
    const XButtonEvent *event
  );

  void motionNotifyEvent(
    const XMotionEvent *event
  );

  void destroyNotifyEvent(
    const XDestroyWindowEvent *
  );

  void mapRequestEvent(
    const XMapRequestEvent *event
  );

  void unmapNotifyEvent(
    const XUnmapEvent *
  );

  void reparentNotifyEvent(
    const XReparentEvent *
  );

  void propertyNotifyEvent(
    const XPropertyEvent *event
  );

  void exposeEvent(
    const XExposeEvent *event
  );

  void configureRequestEvent(
    const XConfigureRequestEvent *event
  );

  void enterNotifyEvent(
    const XCrossingEvent *event
  );

  void leaveNotifyEvent(
    const XCrossingEvent *
  );


#ifdef SHAPE
  void configureShape();
  void shapeEvent(
    XShapeEvent *
  );
#endif // SHAPE


  virtual void timeout();
};


#endif // HACKEDBOX_WINDOW_HPP
