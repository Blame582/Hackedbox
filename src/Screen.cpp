// Screen.cpp for Hackedbox - an X Window manager
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
#endif

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <functional>
#include <locale.h>
#include <stdarg.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using std::string;

#include "Hackedbox.hpp"
#include "BaseMenu.hpp"
#include "ClientMenu.hpp"
#include "IconMenu.hpp"
#include "ImageControl.hpp"
#include "GCCache.hpp"
#include "MenuManager.hpp"
#include "Screen.hpp"
#include "Util.hpp"
#include "Window.hpp"
#include "Workspace.hpp"
#include "WorkspaceMenu.hpp"

#ifndef FONT_ELEMENT_SIZE
#define FONT_ELEMENT_SIZE 50
#endif

static bool running = True;


static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr,
          "HbScreen::HbScreen: an error occurred while querying the X server.\n"
          "  another window manager already running on display %s.\n",
          DisplayString(display));

  running = False;

  return -1;
}


HbScreen::HbScreen(Hackedbox *hb, unsigned int scrn)
  : ScreenInfo(hb, scrn) {

  hackedbox = hb;

  event_mask = ColormapChangeMask |
               EnterWindowMask |
               PropertyChangeMask |
               SubstructureRedirectMask |
               ButtonPressMask |
               ButtonReleaseMask;

  XErrorHandler oldHandler = XSetErrorHandler(anotherWMRunning);

  XSelectInput(getBaseDisplay()->getXDisplay(),
               getRootWindow(),
               event_mask);

  XSync(getBaseDisplay()->getXDisplay(), False);

  XSetErrorHandler(oldHandler);

  managed = running;

  if (!managed)
    return;

  fprintf(stderr,
          "HbScreen::HbScreen: managing screen %d "
          "using visual 0x%lx, depth %d\n",
          getScreenNumber(),
          XVisualIDFromVisual(getVisual()),
          getDepth());

  rootmenu = 0;
  resource.stylerc = 0;

  resource.mstyle.t_fontset =
    resource.mstyle.f_fontset =
    resource.mstyle.clock_fontset =
    resource.mstyle.date_fontset =
    resource.wstyle.fontset = (XFontSet) 0;

  resource.mstyle.t_font =
    resource.mstyle.f_font =
    resource.mstyle.clock_font =
    resource.mstyle.date_font =
    resource.wstyle.font = 0;

  geom_pixmap = None;

#ifdef HAVE_GETPID
  pid_t bpid = getpid();

  XChangeProperty(hackedbox->getXDisplay(),
                  getRootWindow(),
                  hackedbox->getHackedboxPidAtom(),
                  XA_CARDINAL,
                  sizeof(pid_t) * 8,
                  PropModeReplace,
                  reinterpret_cast<unsigned char *>(&bpid),
                  1);
#endif

  Window wm_window = XCreateSimpleWindow(
    hackedbox->getXDisplay(),
    getRootWindow(),
    0, 0, 1, 1, 0, 0, 0);

  XChangeProperty(
    hackedbox->getXDisplay(),
    getRootWindow(),
    hackedbox->getNETSupportingWMCheckAtom(),
    XA_WINDOW,
    32,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(&wm_window),
    1);

  XChangeProperty(
    hackedbox->getXDisplay(),
    wm_window,
    hackedbox->getNETSupportingWMCheckAtom(),
    XA_WINDOW,
    32,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(&wm_window),
    1);

  XStoreName(
    hackedbox->getXDisplay(),
    wm_window,
    "Hackedbox");

  Atom utf8_string =
    XInternAtom(hackedbox->getXDisplay(), "UTF8_STRING", False);

  XChangeProperty(
    hackedbox->getXDisplay(),
    wm_window,
    hackedbox->getNETWMNameAtom(),
    utf8_string,
    8,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(
      const_cast<char *>("Hackedbox")),
    9);

  XDefineCursor(hackedbox->getXDisplay(),
                getRootWindow(),
                hackedbox->getSessionCursor());

  usableArea.setSize(getWidth(), getHeight());

  image_control =
    new HbImageControl(hackedbox,
                       this,
                       True,
                       hackedbox->getColorsPerChannel(),
                       hackedbox->getCacheLife(),
                       hackedbox->getCacheMax());

  image_control->installRootColormap();
  root_colormap_installed = True;

  hackedbox->load_rc(this);

  image_control->setDither(resource.image_dither);

  LoadStyle();

  XGCValues gcv;

  gcv.foreground =
    WhitePixel(hackedbox->getXDisplay(), getScreenNumber()) ^
    BlackPixel(hackedbox->getXDisplay(), getScreenNumber());

  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;

  opGC = XCreateGC(hackedbox->getXDisplay(),
                   getRootWindow(),
                   GCForeground | GCFunction | GCSubwindowMode,
                   &gcv);

  const char *s = "0: 0000 x 0: 0000";
  int l = strlen(s);

  if (MB_CUR_MAX > 1) {
    XRectangle ink;
    XRectangle logical;

    XmbTextExtents(resource.wstyle.fontset,
                   s,
                   l,
                   &ink,
                   &logical);

    geom_w = logical.width;
    geom_h =
      resource.wstyle.fontset_extents->max_ink_extent.height;
  } else {
    geom_h =
      resource.wstyle.font->ascent() +
      resource.wstyle.font->descent();

    geom_w =
      XTextWidth(resource.wstyle.font->xfont(),
                 s,
                 l);
  }

  geom_w += resource.bevel_width * 2;
  geom_h += resource.bevel_width * 2;

  XSetWindowAttributes attrib;

  unsigned long mask =
    CWBorderPixel |
    CWColormap |
    CWSaveUnder;

  attrib.border_pixel = getBorderColor()->pixel();
  attrib.colormap = getColormap();
  attrib.save_under = True;

  geom_window =
    XCreateWindow(hackedbox->getXDisplay(),
                  getRootWindow(),
                  0,
                  0,
                  geom_w,
                  geom_h,
                  resource.border_width,
                  getDepth(),
                  InputOutput,
                  getVisual(),
                  mask,
                  &attrib);

  geom_visible = False;

  HbTexture *texture = &resource.wstyle.l_focus;

  geom_pixmap =
    texture->render(geom_w,
                    geom_h,
                    geom_pixmap);

  if (geom_pixmap == ParentRelative) {
    texture = &resource.wstyle.t_focus;

    geom_pixmap =
      texture->render(geom_w,
                      geom_h,
                      geom_pixmap);
  }

  if (!geom_pixmap) {
    XSetWindowBackground(hackedbox->getXDisplay(),
                         geom_window,
                         texture->color().pixel());
  } else {
    XSetWindowBackgroundPixmap(hackedbox->getXDisplay(),
                               geom_window,
                               geom_pixmap);
  }

  workspacemenu = new Workspacemenu(this);
  iconmenu = new Iconmenu(this);
  configmenu = new Configmenu(this);

  Workspace *workspace = 0;

  if (resource.workspaces != 0) {
    for (unsigned int i = 0; i < resource.workspaces; ++i) {
      workspace =
        new Workspace(this,
                      workspacesList.size());

      workspacesList.push_back(workspace);

      workspacemenu->insert(workspace->getName(),
                            workspace->getMenu());
    }
  } else {
    workspace =
      new Workspace(this,
                    workspacesList.size());

    workspacesList.push_back(workspace);

    workspacemenu->insert(workspace->getName(),
                          workspace->getMenu());
  }

  workspacemenu->insert("Icons", iconmenu);
  workspacemenu->update();

  current_workspace = workspacesList.front();

  workspacemenu->setItemSelected(2, True);

  removeWorkspaceNames();

  InitMenu();

  raiseWindows(0, 0);

  rootmenu->update();

  updateAvailableArea();

  changeWorkspaceID(0);

  unsigned int i;
  unsigned int j;
  unsigned int nchild;

  Window root;
  Window parent;
  Window *children = 0;

  XQueryTree(hackedbox->getXDisplay(),
             getRootWindow(),
             &root,
             &parent,
             &children,
             &nchild);

  for (i = 0; i < nchild; ++i) {
    if (children[i] == None)
      continue;

    XWMHints *wmhints =
      XGetWMHints(hackedbox->getXDisplay(),
                  children[i]);

    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
          (wmhints->icon_window != children[i])) {

        for (j = 0; j < nchild; ++j) {
          if (children[j] == wmhints->icon_window) {
            children[j] = None;
            break;
          }
        }
      }

      XFree(wmhints);
    }
  }

  for (i = 0; i < nchild; ++i) {
    if (children[i] == None ||
        !hackedbox->validateWindow(children[i]))
      continue;

    XWindowAttributes attributes;

    if (XGetWindowAttributes(hackedbox->getXDisplay(),
                             children[i],
                             &attributes)) {

      if (attributes.override_redirect)
        continue;

      if (attributes.map_state != IsUnmapped)
        manageWindow(children[i]);
    }
  }

  if (children)
    XFree(children);

  updateAvailableArea();
}


HbScreen::~HbScreen(void) {
  if (!managed)
    return;

  if (geom_pixmap != None)
    image_control->removeImage(geom_pixmap);

  if (geom_window != None)
    XDestroyWindow(hackedbox->getXDisplay(),
                   geom_window);

  std::for_each(workspacesList.begin(),
                workspacesList.end(),
                PointerAssassin());

  std::for_each(iconList.begin(),
                iconList.end(),
                PointerAssassin());

  std::for_each(netizenList.begin(),
                netizenList.end(),
                PointerAssassin());

  delete rootmenu;
  delete workspacemenu;
  delete iconmenu;
  delete configmenu;
  delete image_control;

  if (resource.wstyle.fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.wstyle.fontset);

  if (resource.mstyle.t_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.t_fontset);

  if (resource.mstyle.f_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.f_fontset);

  if (resource.mstyle.clock_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.clock_fontset);

  if (resource.mstyle.date_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.date_fontset);

  delete resource.wstyle.font;
  delete resource.mstyle.t_font;
  delete resource.mstyle.f_font;
  delete resource.mstyle.clock_font;
  delete resource.mstyle.date_font;

  XFreeGC(hackedbox->getXDisplay(), opGC);
}


void HbScreen::removeWorkspaceNames(void) {
  workspaceNames.clear();
}


void HbScreen::InitMenu(void) {
  if (rootmenu) {
    rootmenuList.clear();

    while (rootmenu->getCount())
      rootmenu->remove(0);
  } else {
    rootmenu = new MenuManager(this);
  }

  bool defaultMenu = True;

  if (hackedbox->getMenuFilename()) {
    FILE *menuFile =
      fopen(hackedbox->getMenuFilename(), "r");

    if (!menuFile) {
      fprintf(stderr,
              "HbScreen::InitMenu: "
              "unable to open menu file '%s'\n",
              hackedbox->getMenuFilename());
    } else {
      struct stat buffer;

      if (fstat(fileno(menuFile), &buffer) ||
          !S_ISREG(buffer.st_mode)) {

        fprintf(stderr,
                "HbScreen::InitMenu: "
                "'%s' is not a regular file\n",
                hackedbox->getMenuFilename());

        fclose(menuFile);
      } else {
        MenuManager menuManager(this);

        if (!menuManager.parseFile(menuFile,
                                   rootmenu)) {
          defaultMenu = False;
        }

        fclose(menuFile);
      }
    }
  }

  if (defaultMenu) {
    rootmenu->insert("xterm",
                     HbScreen::Execute,
                     "xterm");

    rootmenu->insert("Restart",
                     HbScreen::Restart);

    rootmenu->insert("Exit",
                     HbScreen::Exit);
  }

  rootmenu->update();
}


void HbScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;

  gcv.foreground =
    WhitePixel(hackedbox->getXDisplay(),
               getScreenNumber());

  gcv.function = GXinvert;
  gcv.subwindow_mode = IncludeInferiors;

  XChangeGC(hackedbox->getXDisplay(),
            opGC,
            GCForeground |
            GCFunction |
            GCSubwindowMode,
            &gcv);

  const char *s = "0: 0000 x 0: 0000";
  int l = strlen(s);

  if (MB_CUR_MAX > 1) {
    XRectangle ink;
    XRectangle logical;

    XmbTextExtents(resource.wstyle.fontset,
                   s,
                   l,
                   &ink,
                   &logical);

    geom_w = logical.width;

    geom_h =
      resource.wstyle.fontset_extents->max_ink_extent.height;
  } else {
    geom_w =
      XTextWidth(resource.wstyle.font->xfont(),
                 s,
                 l);

    geom_h =
      resource.wstyle.font->ascent() +
      resource.wstyle.font->descent();
  }

  geom_w += resource.bevel_width * 2;
  geom_h += resource.bevel_width * 2;

  HbTexture *texture = &resource.wstyle.l_focus;

  geom_pixmap =
    texture->render(geom_w,
                    geom_h,
                    geom_pixmap);

  if (geom_pixmap == ParentRelative) {
    texture = &resource.wstyle.t_focus;

    geom_pixmap =
      texture->render(geom_w,
                      geom_h,
                      geom_pixmap);
  }

  if (!geom_pixmap) {
    XSetWindowBackground(hackedbox->getXDisplay(),
                         geom_window,
                         texture->color().pixel());
  } else {
    XSetWindowBackgroundPixmap(hackedbox->getXDisplay(),
                               geom_window,
                               geom_pixmap);
  }

  XSetWindowBorderWidth(hackedbox->getXDisplay(),
                        geom_window,
                        resource.border_width);

  XSetWindowBorder(hackedbox->getXDisplay(),
                   geom_window,
                   resource.border_color.pixel());

  workspacemenu->reconfigure();
  iconmenu->reconfigure();

  int rememberSubmenu =
    rootmenu->getCurrentSubmenu();

  InitMenu();

  raiseWindows(0, 0);

  rootmenu->reconfigure();
  rootmenu->drawSubmenu(rememberSubmenu);

  configmenu->reconfigure();

  for (auto *workspace : workspacesList)
    workspace->reconfigure();

  HackedboxWindowList::iterator iconIterator =
    iconList.begin();

  for (; iconIterator != iconList.end(); ++iconIterator) {
    HackedboxWindow *window = *iconIterator;

    if (window->validateClient())
      window->reconfigure();
  }

  image_control->timeout();
}


void HbScreen::rereadMenu(void) {
  InitMenu();

  raiseWindows(0, 0);

  rootmenu->reconfigure();
}


void HbScreen::LoadStyle(void) {
  if (resource.stylerc)
    XrmDestroyDatabase(resource.stylerc);

  resource.stylerc =
    XrmGetFileDatabase(hackedbox->getStyleFilename());

  if (!resource.stylerc)
    resource.stylerc =
      XrmGetFileDatabase(DEFAULTSTYLE);

  resource.backgroundFolder =
    expandTilde("~/.hackedbox/backgrounds");

  resource.backgroundTimer = 0;

  XrmValue value;
  char *valueType;

  if (resource.wstyle.fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.wstyle.fontset);

  if (resource.mstyle.f_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.f_fontset);

  if (resource.mstyle.t_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.t_fontset);

  if (resource.mstyle.clock_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.clock_fontset);

  if (resource.mstyle.date_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 resource.mstyle.date_fontset);

  resource.wstyle.fontset = 0;
  resource.wstyle.fontset_extents = 0;

  resource.mstyle.f_fontset = 0;
  resource.mstyle.f_fontset_extents = 0;

  resource.mstyle.t_fontset = 0;
  resource.mstyle.t_fontset_extents = 0;

  resource.mstyle.clock_fontset = 0;
  resource.mstyle.clock_fontset_extents = 0;

  resource.mstyle.date_fontset = 0;
  resource.mstyle.date_fontset_extents = 0;

  delete resource.wstyle.font;
  delete resource.mstyle.f_font;
  delete resource.mstyle.t_font;
  delete resource.mstyle.clock_font;
  delete resource.mstyle.date_font;

  resource.wstyle.font = 0;
  resource.mstyle.f_font = 0;
  resource.mstyle.t_font = 0;
  resource.mstyle.clock_font = 0;
  resource.mstyle.date_font = 0;

  resource.wstyle.font =
    readDatabaseFont("window.font",
                     "Window.Font");

  resource.mstyle.t_font =
    readDatabaseFont("menu.title.font",
                     "Menu.Title.Font");

  resource.mstyle.f_font =
    readDatabaseFont("menu.frame.font",
                     "Menu.Frame.Font");

  resource.mstyle.clock_font =
    readDatabaseFont("menu.clock.font",
                     "Menu.Clock.Font");

  resource.mstyle.date_font =
    readDatabaseFont("menu.date.font",
                     "Menu.Date.Font");

  if (MB_CUR_MAX > 1) {
    resource.wstyle.fontset =
      readDatabaseFontSet("window.font",
                          "Window.Font");

    resource.mstyle.t_fontset =
      readDatabaseFontSet("menu.title.font",
                          "Menu.Title.Font");

    resource.mstyle.f_fontset =
      readDatabaseFontSet("menu.frame.font",
                          "Menu.Frame.Font");

    resource.mstyle.clock_fontset =
      readDatabaseFontSet("menu.clock.font",
                          "Menu.Clock.Font");

    resource.mstyle.date_fontset =
      readDatabaseFontSet("menu.date.font",
                          "Menu.Date.Font");

    resource.mstyle.t_fontset_extents =
      XExtentsOfFontSet(resource.mstyle.t_fontset);

    resource.mstyle.f_fontset_extents =
      XExtentsOfFontSet(resource.mstyle.f_fontset);

    resource.mstyle.clock_fontset_extents =
      XExtentsOfFontSet(resource.mstyle.clock_fontset);

    resource.mstyle.date_fontset_extents =
      XExtentsOfFontSet(resource.mstyle.date_fontset);

    resource.wstyle.fontset_extents =
      XExtentsOfFontSet(resource.wstyle.fontset);
  }

  resource.wstyle.t_focus =
    readDatabaseTexture("window.title.focus",
                        "Window.Title.Focus",
                        "white");

  resource.wstyle.t_unfocus =
    readDatabaseTexture("window.title.unfocus",
                        "Window.Title.Unfocus",
                        "black");

  resource.wstyle.l_focus =
    readDatabaseTexture("window.label.focus",
                        "Window.Label.Focus",
                        "white");

  resource.wstyle.l_unfocus =
    readDatabaseTexture("window.label.unfocus",
                        "Window.Label.Unfocus",
                        "black");

  resource.wstyle.h_focus =
    readDatabaseTexture("window.handle.focus",
                        "Window.Handle.Focus",
                        "white");

  resource.wstyle.h_unfocus =
    readDatabaseTexture("window.handle.unfocus",
                        "Window.Handle.Unfocus",
                        "black");

  resource.wstyle.g_focus =
    readDatabaseTexture("window.grip.focus",
                        "Window.Grip.Focus",
                        "white");

  resource.wstyle.g_unfocus =
    readDatabaseTexture("window.grip.unfocus",
                        "Window.Grip.Unfocus",
                        "black");

  resource.wstyle.b_focus =
    readDatabaseTexture("window.button.focus",
                        "Window.Button.Focus",
                        "white");

  resource.wstyle.b_unfocus =
    readDatabaseTexture("window.button.unfocus",
                        "Window.Button.Unfocus",
                        "black");

  resource.wstyle.b_pressed =
    readDatabaseTexture("window.button.pressed",
                        "Window.Button.Pressed",
                        "black");

  HbColor color =
    readDatabaseColor("window.frame.focusColor",
                      "Window.Frame.FocusColor",
                      "white");

  resource.wstyle.f_focus =
    HbTexture("solid flat",
              getBaseDisplay(),
              getScreenNumber(),
              image_control);

  resource.wstyle.f_focus.setColor(color);

  color =
    readDatabaseColor("window.frame.unfocusColor",
                      "Window.Frame.UnfocusColor",
                      "white");

  resource.wstyle.f_unfocus =
    HbTexture("solid flat",
              getBaseDisplay(),
              getScreenNumber(),
              image_control);

  resource.wstyle.f_unfocus.setColor(color);

  resource.wstyle.l_text_focus =
    readDatabaseColor("window.label.focus.textColor",
                      "Window.Label.Focus.TextColor",
                      "black");

  resource.wstyle.l_text_unfocus =
    readDatabaseColor("window.label.unfocus.textColor",
                      "Window.Label.Unfocus.TextColor",
                      "white");

  resource.wstyle.b_pic_focus =
    readDatabaseColor("window.button.focus.picColor",
                      "Window.Button.Focus.PicColor",
                      "black");

  resource.wstyle.b_pic_unfocus =
    readDatabaseColor("window.button.unfocus.picColor",
                      "Window.Button.Unfocus.PicColor",
                      "white");

  resource.wstyle.justify = LeftJustify;

  if (XrmGetResource(resource.stylerc,
                     "window.justify",
                     "Window.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      resource.wstyle.justify = RightJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      resource.wstyle.justify = CenterJustify;
    }
  }

  if (resource.wstyle.t_focus.texture() ==
      HbTexture::ParentRelativeTexture)
    resource.wstyle.t_focus = resource.wstyle.f_focus;

  if (resource.wstyle.t_unfocus.texture() ==
      HbTexture::ParentRelativeTexture)
    resource.wstyle.t_unfocus = resource.wstyle.f_unfocus;

  if (resource.wstyle.h_focus.texture() ==
      HbTexture::ParentRelativeTexture)
    resource.wstyle.h_focus = resource.wstyle.f_focus;

  if (resource.wstyle.h_unfocus.texture() ==
      HbTexture::ParentRelativeTexture)
    resource.wstyle.h_unfocus = resource.wstyle.f_unfocus;

  resource.mstyle.title =
    readDatabaseTexture("menu.title",
                        "Menu.Title",
                        "white");

  resource.mstyle.frame =
    readDatabaseTexture("menu.frame",
                        "Menu.Frame",
                        "black");

  resource.mstyle.hilite =
    readDatabaseTexture("menu.hilite",
                        "Menu.Hilite",
                        "white");

  resource.mstyle.t_text =
    readDatabaseColor("menu.title.textColor",
                      "Menu.Title.TextColor",
                      "black");

  resource.mstyle.f_text =
    readDatabaseColor("menu.frame.textColor",
                      "Menu.Frame.TextColor",
                      "white");

  resource.mstyle.d_text =
    readDatabaseColor("menu.frame.disableColor",
                      "Menu.Frame.DisableColor",
                      "black");

  resource.mstyle.h_text =
    readDatabaseColor("menu.hilite.textColor",
                      "Menu.Hilite.TextColor",
                      "black");

  resource.mstyle.clock_text =
    readDatabaseColor("menu.clock.textColor",
                      "Menu.Clock.TextColor",
                      "black");

  resource.mstyle.date_text =
    readDatabaseColor("menu.date.textColor",
                      "Menu.Date.TextColor",
                      "black");

  resource.mstyle.clock_format =
    "%I:%M:%S %p";

  if (XrmGetResource(resource.stylerc,
                     "menu.clock.format",
                     "Menu.Clock.Format",
                     &valueType,
                     &value)) {

    resource.mstyle.clock_format = value.addr;
  }

  resource.mstyle.date_format =
    "%m/%d/%Y";

  if (XrmGetResource(resource.stylerc,
                     "menu.date.format",
                     "Menu.Date.Format",
                     &valueType,
                     &value)) {

    resource.mstyle.date_format = value.addr;
  }

  resource.mstyle.t_justify = LeftJustify;

  if (XrmGetResource(resource.stylerc,
                     "menu.title.justify",
                     "Menu.Title.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      resource.mstyle.t_justify = RightJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      resource.mstyle.t_justify = CenterJustify;
    }
  }

  resource.mstyle.f_justify = LeftJustify;

  if (XrmGetResource(resource.stylerc,
                     "menu.frame.justify",
                     "Menu.Frame.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      resource.mstyle.f_justify = RightJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      resource.mstyle.f_justify = CenterJustify;
    }
  }

  resource.mstyle.clock_justify = CenterJustify;

  if (XrmGetResource(resource.stylerc,
                     "menu.clock.justify",
                     "Menu.Clock.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      resource.mstyle.clock_justify = RightJustify;

    } else if (strstr(value.addr, "left") ||
               strstr(value.addr, "Left")) {

      resource.mstyle.clock_justify = LeftJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      resource.mstyle.clock_justify = CenterJustify;
    }
  }

  resource.mstyle.date_justify = CenterJustify;

  if (XrmGetResource(resource.stylerc,
                     "menu.date.justify",
                     "Menu.Date.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      resource.mstyle.date_justify = RightJustify;

    } else if (strstr(value.addr, "left") ||
               strstr(value.addr, "Left")) {

      resource.mstyle.date_justify = LeftJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      resource.mstyle.date_justify = CenterJustify;
    }
  }

  resource.mstyle.bullet = HbBasemenu::Triangle;

  if (XrmGetResource(resource.stylerc,
                     "menu.bullet",
                     "Menu.Bullet",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "empty",
                     value.size)) {

      resource.mstyle.bullet = HbBasemenu::Empty;

    } else if (!strncasecmp(value.addr,
                            "square",
                            value.size)) {

      resource.mstyle.bullet = HbBasemenu::Square;

    } else if (!strncasecmp(value.addr,
                            "diamond",
                            value.size)) {

      resource.mstyle.bullet = HbBasemenu::Diamond;
    }
  }

  resource.mstyle.bullet_pos = HbBasemenu::Left;

  if (XrmGetResource(resource.stylerc,
                     "menu.bullet.position",
                     "Menu.Bullet.Position",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "right",
                     value.size)) {

      resource.mstyle.bullet_pos = HbBasemenu::Right;
    }
  }

  resource.mstyle.icon = true;

  if (XrmGetResource(resource.stylerc,
                     "menu.icon",
                     "Menu.Icon",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "false",
                     value.size) ||
        !strncasecmp(value.addr,
                     "no",
                     value.size) ||
        !strncasecmp(value.addr,
                     "off",
                     value.size)) {

      resource.mstyle.icon = false;
    }
  }

resource.mstyle.icon_pos = HbBasemenu::Left;

if (XrmGetResource(resource.stylerc,
                   "menu.icon.position",
                   "Menu.Icon.Position",
                   &valueType,
                   &value)) {

  if (!strncasecmp(value.addr,
                   "right",
                   value.size)) {

    resource.mstyle.icon_pos = HbBasemenu::Right;

  } else if (!strncasecmp(value.addr,
                          "left",
                          value.size)) {

    resource.mstyle.icon_pos = HbBasemenu::Left;
  }
}

  if (resource.mstyle.frame.texture() ==
      HbTexture::ParentRelativeTexture) {

    resource.mstyle.frame =
      HbTexture("solid flat",
                getBaseDisplay(),
                getScreenNumber(),
                image_control);

    resource.mstyle.frame.setColor(
      HbColor("black",
              getHackedbox(),
              getScreenNumber()));
  }

  resource.border_color =
    readDatabaseColor("borderColor",
                      "BorderColor",
                      "black");

  unsigned int uintValue;

  resource.handle_width = 6;

  if (XrmGetResource(resource.stylerc,
                     "handleWidth",
                     "HandleWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1 &&
      uintValue <= (getWidth() / 2) &&
      uintValue != 0) {

    resource.handle_width = uintValue;
  }

  resource.border_width = 1;

  if (XrmGetResource(resource.stylerc,
                     "borderWidth",
                     "BorderWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1) {

    resource.border_width = uintValue;
  }

  resource.bevel_width = 3;

  if (XrmGetResource(resource.stylerc,
                     "bevelWidth",
                     "BevelWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1 &&
      uintValue <= (getWidth() / 2) &&
      uintValue != 0) {

    resource.bevel_width = uintValue;
  }

  resource.frame_width = resource.bevel_width;

  if (XrmGetResource(resource.stylerc,
                     "frameWidth",
                     "FrameWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1 &&
      uintValue <= (getWidth() / 2)) {

    resource.frame_width = uintValue;
  }

  if (XrmGetResource(resource.stylerc,
                     "rootCommand",
                     "RootCommand",
                     &valueType,
                     &value)) {

    hbexec(value.addr, displayString());
  }

  if (XrmGetResource(resource.stylerc,
                     "root.background",
                     "Root.Background",
                     &valueType,
                     &value)) {

    std::string background =
      expandTilde(value.addr);

    std::string mode = "center";

    XrmValue modeValue{};
    char *modeType = nullptr;

    if (XrmGetResource(resource.stylerc,
                       "root.background.mode",
                       "Root.Background.Mode",
                       &modeType,
                       &modeValue)) {

      mode = modeValue.addr;
    }

    std::string command = "hbsetbg";

    if (mode == "center")
      command += " -center";
    else if (mode == "tile")
      command += " -tile";
    else if (mode == "stretch2center")
      command += " -s2c";
    else if (mode == "stretch2edge")
      command += " -s2e";

    command += " \"" + background + "\"";

    hbexec(command.c_str(), displayString());
  }

  if (XrmGetResource(resource.stylerc,
                     "root.background.folder",
                     "Root.Background.Folder",
                     &valueType,
                     &value)) {

    resource.backgroundFolder =
      expandTilde(value.addr);
  }

  if (XrmGetResource(resource.stylerc,
                     "root.background.timer",
                     "Root.Background.Timer",
                     &valueType,
                     &value)) {

    resource.backgroundTimer =
      atoi(value.addr);
  }

  XrmDestroyDatabase(resource.stylerc);
  resource.stylerc = 0;
}


void HbScreen::addIcon(HackedboxWindow *window) {
  if (!window)
    return;

  window->setWorkspace(BSENTINEL);
  window->setWindowNumber(iconList.size());

  iconList.push_back(window);

  const char *title = window->getIconTitle();

  iconmenu->insert(title);
  iconmenu->update();
}


void HbScreen::removeIcon(HackedboxWindow *window) {
  if (!window)
    return;

  iconList.remove(window);

  iconmenu->remove(window->getWindowNumber());
  iconmenu->update();

  HackedboxWindowList::iterator iterator =
    iconList.begin();

  HackedboxWindowList::iterator end =
    iconList.end();

  int number = 0;

  for (; iterator != end; ++iterator)
    (*iterator)->setWindowNumber(number++);
}


HackedboxWindow *HbScreen::getIcon(unsigned int index) {
  if (index < iconList.size()) {
    HackedboxWindowList::iterator iterator =
      iconList.begin();

    while (index-- > 0)
      ++iterator;

    return *iterator;
  }

  return 0;
}


unsigned int HbScreen::addWorkspace(void) {
  Workspace *workspace =
    new Workspace(this,
                  workspacesList.size());

  workspacesList.push_back(workspace);

  workspacemenu->insert(workspace->getName(),
                        workspace->getMenu(),
                        workspace->getID() + 2);

  workspacemenu->update();

  updateNetizenWorkspaceCount();

  return workspacesList.size();
}


unsigned int HbScreen::removeLastWorkspace(void) {
  if (workspacesList.size() == 1)
    return 1;

  Workspace *workspace =
    workspacesList.back();

  if (current_workspace->getID() ==
      workspace->getID()) {

    changeWorkspaceID(
      current_workspace->getID() - 1);
  }

  workspace->removeAll();

  workspacemenu->remove(workspace->getID() + 2);
  workspacemenu->update();

  workspacesList.pop_back();

  delete workspace;

  updateNetizenWorkspaceCount();

  return workspacesList.size();
}


void HbScreen::changeWorkspaceID(unsigned int id) {
  if (!current_workspace ||
      id == current_workspace->getID())
    return;

  current_workspace->hide();

  workspacemenu->setItemSelected(
    current_workspace->getID() + 2,
    False);

  current_workspace = getWorkspace(id);

  current_workspace->show();

  workspacemenu->setItemSelected(
    current_workspace->getID() + 2,
    True);

  updateNetizenCurrentWorkspace();
}


void HbScreen::manageWindow(Window window) {
  XWMHints *wmHints =
    XGetWMHints(hackedbox->getXDisplay(),
                window);

  if (wmHints &&
      (wmHints->flags & StateHint) &&
      wmHints->initial_state == WithdrawnState) {

    XFree(wmHints);
    return;
  }

  if (wmHints)
    XFree(wmHints);

  new HackedboxWindow(hackedbox,
                      window,
                      this);

  HackedboxWindow *managedWindow =
    hackedbox->searchWindow(window);

  if (!managedWindow)
    return;

  windowList.push_back(managedWindow);

  XMapRequestEvent mapRequest;
  memset(&mapRequest, 0, sizeof(mapRequest));

  mapRequest.window = window;

  if (hackedbox->isStartup())
    managedWindow->restoreAttributes();

  managedWindow->mapRequestEvent(&mapRequest);
}


void HbScreen::unmanageWindow(HackedboxWindow *window,
                              bool remap) {
  window->restore(remap);

  if (window->isModal())
    window->setModal(False);

  if (window->getWorkspaceNumber() != BSENTINEL &&
      window->getWindowNumber() != BSENTINEL) {

    getWorkspace(window->getWorkspaceNumber())
      ->removeWindow(window);

  } else if (window->isIconic()) {

    removeIcon(window);
  }

  windowList.remove(window);

  if (hackedbox->getFocusedWindow() == window)
    hackedbox->setFocusedWindow(0);

  removeNetizen(window->getClientWindow());

  HbWindowGroup *group =
    hackedbox->searchGroup(window->getClientWindow());

  delete group;
  delete window;
}


void HbScreen::addNetizen(Netizen *netizen) {
  netizenList.push_back(netizen);

  netizen->sendWorkspaceCount();
  netizen->sendCurrentWorkspace();

  WorkspaceList::iterator iterator =
    workspacesList.begin();

  const WorkspaceList::iterator end =
    workspacesList.end();

  for (; iterator != end; ++iterator)
    (*iterator)->sendWindowList(*netizen);

  Window focusedWindow =
    hackedbox->getFocusedWindow()
      ? hackedbox->getFocusedWindow()->getClientWindow()
      : None;

  netizen->sendWindowFocus(focusedWindow);
}


void HbScreen::removeNetizen(Window window) {
  NetizenList::iterator iterator =
    netizenList.begin();

  while (iterator != netizenList.end()) {
    if ((*iterator)->getWindowID() == window) {
      delete *iterator;
      netizenList.erase(iterator);
      break;
    }

    ++iterator;
  }
}


void HbScreen::updateNetizenCurrentWorkspace(void) {
  std::for_each(netizenList.begin(),
                netizenList.end(),
                std::mem_fn(&Netizen::sendCurrentWorkspace));
}


void HbScreen::updateNetizenWorkspaceCount(void) {
  std::for_each(netizenList.begin(),
                netizenList.end(),
                std::mem_fn(&Netizen::sendWorkspaceCount));
}


void HbScreen::updateNetizenWindowFocus(void) {
  Window focusedWindow =
    hackedbox->getFocusedWindow()
      ? hackedbox->getFocusedWindow()->getClientWindow()
      : None;

  NetizenList::iterator iterator =
    netizenList.begin();

  for (; iterator != netizenList.end(); ++iterator)
    (*iterator)->sendWindowFocus(focusedWindow);
}


void HbScreen::updateNetizenWindowAdd(Window window,
                                      unsigned long property) {
  NetizenList::iterator iterator =
    netizenList.begin();

  for (; iterator != netizenList.end(); ++iterator)
    (*iterator)->sendWindowAdd(window, property);
}


void HbScreen::updateNetizenWindowDel(Window window) {
  NetizenList::iterator iterator =
    netizenList.begin();

  for (; iterator != netizenList.end(); ++iterator)
    (*iterator)->sendWindowDel(window);
}


void HbScreen::updateNetizenWindowRaise(Window window) {
  NetizenList::iterator iterator =
    netizenList.begin();

  for (; iterator != netizenList.end(); ++iterator)
    (*iterator)->sendWindowRaise(window);
}


void HbScreen::updateNetizenWindowLower(Window window) {
  NetizenList::iterator iterator =
    netizenList.begin();

  for (; iterator != netizenList.end(); ++iterator)
    (*iterator)->sendWindowLower(window);
}


void HbScreen::updateNetizenConfigNotify(XEvent *event) {
  NetizenList::iterator iterator =
    netizenList.begin();

  for (; iterator != netizenList.end(); ++iterator)
    (*iterator)->sendConfigNotify(event);
}


void HbScreen::raiseWindows(Window *workspaceStack,
                            unsigned int number) {
  Window *sessionStack =
    new Window[number +
               workspacesList.size() +
               rootmenuList.size() +
               13];

  unsigned int index = 0;
  unsigned int stackIndex = number;

  XRaiseWindow(hackedbox->getXDisplay(),
               iconmenu->getWindowID());

  sessionStack[index++] =
    iconmenu->getWindowID();

  WorkspaceList::iterator workspaceIterator =
    workspacesList.begin();

  const WorkspaceList::iterator workspaceEnd =
    workspacesList.end();

  for (; workspaceIterator != workspaceEnd;
       ++workspaceIterator) {

    sessionStack[index++] =
      (*workspaceIterator)->getMenu()->getWindowID();
  }

  sessionStack[index++] =
    workspacemenu->getWindowID();

  sessionStack[index++] =
    configmenu->getFocusmenu()->getWindowID();

  sessionStack[index++] =
    configmenu->getPlacementmenu()->getWindowID();

  sessionStack[index++] =
    configmenu->getWindowID();

  RootmenuList::iterator rootIterator =
    rootmenuList.begin();

  for (; rootIterator != rootmenuList.end();
       ++rootIterator) {

    sessionStack[index++] =
      (*rootIterator)->getWindowID();
  }

  sessionStack[index++] =
    rootmenu->getWindowID();

  while (stackIndex--)
    sessionStack[index++] =
      workspaceStack[stackIndex];

  XRestackWindows(hackedbox->getXDisplay(),
                  sessionStack,
                  index);

  delete [] sessionStack;
}


void HbScreen::addWorkspaceName(const string &name) {
  workspaceNames.push_back(name);
}


const string HbScreen::getNameOfWorkspace(unsigned int id) {
  if (id < workspaceNames.size())
    return workspaceNames[id];

  return string("");
}


void HbScreen::reassociateWindow(HackedboxWindow *window,
                                 unsigned int workspaceId,
                                 bool ignoreSticky) {
  if (!window)
    return;

  if (workspaceId == BSENTINEL)
    workspaceId = current_workspace->getID();

  if (window->getWorkspaceNumber() == workspaceId)
    return;

  if (window->isIconic()) {
    removeIcon(window);

    getWorkspace(workspaceId)
      ->addWindow(window);

  } else if (ignoreSticky ||
             !window->isStuck()) {

    getWorkspace(window->getWorkspaceNumber())
      ->removeWindow(window);

    getWorkspace(workspaceId)
      ->addWindow(window);
  }
}


void HbScreen::propagateWindowName(
  const HackedboxWindow *window) {

  if (window->isIconic()) {

    iconmenu->changeItemLabel(
      window->getWindowNumber(),
      window->getIconTitle());

    iconmenu->update();

  } else {

    ClientMenu *clientmenu =
      getWorkspace(window->getWorkspaceNumber())
        ->getMenu();

    clientmenu->changeItemLabel(
      window->getWindowNumber(),
      window->getTitle());

    clientmenu->update();
  }
}


void HbScreen::nextFocus(void) {
  HackedboxWindow *focused =
    hackedbox->getFocusedWindow();

  HackedboxWindow *next = focused;

  if (focused) {
    if (focused->getScreen()->getScreenNumber() !=
        getScreenNumber()) {

      focused = 0;
    }
  }

  if (focused &&
      current_workspace->getCount() > 1) {

    HackedboxWindow *current;

    do {
      current = next;

      next =
        current_workspace
          ->getNextWindowInList(current);

    } while (!next->setInputFocus() &&
             next != focused);

    if (next != focused)
      current_workspace->raiseWindow(next);

  } else if (current_workspace->getCount() >= 1) {

    next =
      current_workspace->getTopWindowOnStack();

    current_workspace->raiseWindow(next);
    next->setInputFocus();
  }
}


void HbScreen::prevFocus(void) {
  HackedboxWindow *focused =
    hackedbox->getFocusedWindow();

  HackedboxWindow *next = focused;

  if (focused) {
    if (focused->getScreen()->getScreenNumber() !=
        getScreenNumber()) {

      focused = 0;
    }
  }

  if (focused &&
      current_workspace->getCount() > 1) {

    HackedboxWindow *current;

    do {
      current = next;

      next =
        current_workspace
          ->getPrevWindowInList(current);

    } while (!next->setInputFocus() &&
             next != focused);

    if (next != focused)
      current_workspace->raiseWindow(next);

  } else if (current_workspace->getCount() >= 1) {

    next =
      current_workspace->getTopWindowOnStack();

    current_workspace->raiseWindow(next);
    next->setInputFocus();
  }
}


void HbScreen::raiseFocus(void) {
  HackedboxWindow *focused =
    hackedbox->getFocusedWindow();

  if (!focused)
    return;

  if (focused->getScreen()->getScreenNumber() ==
      getScreenNumber()) {

    Workspace *workspace =
      getWorkspace(focused->getWorkspaceNumber());

    workspace->raiseWindow(focused);
  }
}


void HbScreen::shutdown(void) {
  XSelectInput(hackedbox->getXDisplay(),
               getRootWindow(),
               NoEventMask);

  XSync(hackedbox->getXDisplay(), False);

  while (!windowList.empty())
    unmanageWindow(windowList.front(), True);
}


void HbScreen::showPosition(int x, int y) {
  if (!geom_visible) {
    XMoveResizeWindow(
      hackedbox->getXDisplay(),
      geom_window,
      (getWidth() - geom_w) / 2,
      (getHeight() - geom_h) / 2,
      geom_w,
      geom_h);

    XMapWindow(hackedbox->getXDisplay(),
               geom_window);

    XRaiseWindow(hackedbox->getXDisplay(),
                 geom_window);

    geom_visible = True;
  }

  char label[1024];

  snprintf(label,
           sizeof(label),
           "X: %4d x Y: %4d",
           x,
           y);

  XClearWindow(
    hackedbox->getXDisplay(),
    geom_window);

  HbPen pen(resource.wstyle.l_text_focus,
            resource.wstyle.font->xfont());

  if (MB_CUR_MAX > 1) {
    XmbDrawString(
      hackedbox->getXDisplay(),
      geom_window,
      resource.wstyle.fontset,
      pen.gc(),
      resource.bevel_width,
      resource.bevel_width -
        resource.wstyle.fontset_extents
          ->max_ink_extent.y,
      label,
      strlen(label));
  } else {
    XDrawString(
      hackedbox->getXDisplay(),
      geom_window,
      pen.gc(),
      resource.bevel_width,
      resource.wstyle.font->ascent() +
        resource.bevel_width,
      label,
      strlen(label));
  }
}


void HbScreen::showGeometry(unsigned int width,
                            unsigned int height) {
  if (!geom_visible) {
    XMoveResizeWindow(
      hackedbox->getXDisplay(),
      geom_window,
      (getWidth() - geom_w) / 2,
      (getHeight() - geom_h) / 2,
      geom_w,
      geom_h);

    XMapWindow(hackedbox->getXDisplay(),
               geom_window);

    XRaiseWindow(hackedbox->getXDisplay(),
                 geom_window);

    geom_visible = True;
  }

  char label[1024];

  snprintf(label,
           sizeof(label),
           "W: %4d x H: %4d",
           width,
           height);

  XClearWindow(
    hackedbox->getXDisplay(),
    geom_window);

  HbPen pen(resource.wstyle.l_text_focus,
            resource.wstyle.font->xfont());

  if (MB_CUR_MAX > 1) {
    XmbDrawString(
      hackedbox->getXDisplay(),
      geom_window,
      resource.wstyle.fontset,
      pen.gc(),
      resource.bevel_width,
      resource.bevel_width -
        resource.wstyle.fontset_extents
          ->max_ink_extent.y,
      label,
      strlen(label));
  } else {
    XDrawString(
      hackedbox->getXDisplay(),
      geom_window,
      pen.gc(),
      resource.bevel_width,
      resource.wstyle.font->ascent() +
        resource.bevel_width,
      label,
      strlen(label));
  }
}


void HbScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(hackedbox->getXDisplay(),
                 geom_window);

    geom_visible = False;
  }
}


void HbScreen::addStrut(Strut *strut) {
  if (!strut)
    return;

  strutList.push_back(strut);
}


void HbScreen::removeStrut(Strut *strut) {
  if (!strut)
    return;

  strutList.remove(strut);
}


const Rect &HbScreen::availableArea(void) const {
  if (doFullMax())
    return getRect();

  return usableArea;
}


void HbScreen::updateAvailableArea(void) {
  Rect oldArea = usableArea;

  usableArea = getRect();

  unsigned int currentLeft = 0;
  unsigned int currentRight = 0;
  unsigned int currentTop = 0;
  unsigned int currentBottom = 0;

  StrutList::const_iterator iterator =
    strutList.begin();

  StrutList::const_iterator end =
    strutList.end();

  for (; iterator != end; ++iterator) {
    Strut *strut = *iterator;

    if (strut->left > currentLeft)
      currentLeft = strut->left;

    if (strut->top > currentTop)
      currentTop = strut->top;

    if (strut->right > currentRight)
      currentRight = strut->right;

    if (strut->bottom > currentBottom)
      currentBottom = strut->bottom;
  }

  usableArea.setPos(currentLeft,
                    currentTop);

  usableArea.setSize(
    usableArea.width() -
      (currentLeft + currentRight),
    usableArea.height() -
      (currentTop + currentBottom));

  if (oldArea != usableArea) {
    HackedboxWindowList::iterator windowIterator =
      windowList.begin();

    HackedboxWindowList::iterator windowEnd =
      windowList.end();

    for (; windowIterator != windowEnd;
         ++windowIterator) {

      if ((*windowIterator)->isMaximized())
        (*windowIterator)->remaximize();
    }
  }
}


Workspace *HbScreen::getWorkspace(unsigned int index) {
  assert(index < workspacesList.size());

  return workspacesList[index];
}


void HbScreen::buttonPressEvent(
  const XButtonEvent *button) {

  if (button->button == 1) {

    if (!isRootColormapInstalled())
      image_control->installRootColormap();

    if (workspacemenu->isVisible())
      workspacemenu->hide();

    if (rootmenu->isVisible())
      rootmenu->hide();

  } else if (button->button == 2) {

    int menuX =
      button->x_root -
      (workspacemenu->getWidth() / 2);

    int menuY =
      button->y_root -
      (workspacemenu->getTitleHeight() / 2);

    if (menuX < 0)
      menuX = 0;

    if (menuY < 0)
      menuY = 0;

    if (menuX + workspacemenu->getWidth() >
        getWidth()) {

      menuX =
        getWidth() -
        workspacemenu->getWidth() -
        getBorderWidth();
    }

    if (menuY + workspacemenu->getHeight() >
        getHeight()) {

      menuY =
        getHeight() -
        workspacemenu->getHeight() -
        getBorderWidth();
    }

    workspacemenu->move(menuX, menuY);

    if (!workspacemenu->isVisible()) {
      workspacemenu->removeParent();
      workspacemenu->show();
    }

  } else if (button->button == 3) {

    int menuX =
      button->x_root -
      (rootmenu->getWidth() / 2);

    int menuY =
      button->y_root -
      (rootmenu->getTitleHeight() / 2);

    if (menuX < 0)
      menuX = 0;

    if (menuY < 0)
      menuY = 0;

    if (menuX + rootmenu->getWidth() >
        getWidth()) {

      menuX =
        getWidth() -
        rootmenu->getWidth() -
        getBorderWidth();
    }

    if (menuY + rootmenu->getHeight() >
        getHeight()) {

      menuY =
        getHeight() -
        rootmenu->getHeight() -
        getBorderWidth();
    }

    rootmenu->move(menuX, menuY);

    if (!rootmenu->isVisible()) {
      hackedbox->checkMenu();
      rootmenu->show();
    }
  }
}


void HbScreen::toggleFocusModel(FocusModel model) {
  std::for_each(
    windowList.begin(),
    windowList.end(),
    std::mem_fn(&HackedboxWindow::ungrabButtons));

  if (model == SloppyFocus) {
    saveSloppyFocus(True);
  } else {
    saveSloppyFocus(False);
    saveAutoRaise(False);
    saveClickRaise(False);
  }

  std::for_each(
    windowList.begin(),
    windowList.end(),
    std::mem_fn(&HackedboxWindow::grabButtons));
}


HbTexture HbScreen::readDatabaseTexture(
  const string &resourceName,
  const string &resourceClass,
  const string &defaultColor) {

  HbTexture texture;

  XrmValue value;
  char *valueType;

  if (XrmGetResource(resource.stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    texture = HbTexture(value.addr);

  } else {

    texture.setHbTexture(
      HbTexture::Solid |
      HbTexture::Flat);
  }

  texture.setDisplay(getBaseDisplay(),
                     getScreenNumber());

  texture.setHbImageControl(image_control);

  texture.setColor(
    readDatabaseColor(resourceName + ".color",
                      resourceClass + ".Color",
                      defaultColor));

  texture.setColorTo(
    readDatabaseColor(resourceName + ".colorTo",
                      resourceClass + ".ColorTo",
                      defaultColor));

  return texture;
}


HbColor HbScreen::readDatabaseColor(
  const string &resourceName,
  const string &resourceClass,
  const string &defaultColor) {

  HbColor color;

  XrmValue value;
  char *valueType;

  if (XrmGetResource(resource.stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    color =
      HbColor(value.addr,
              getHackedbox(),
              getScreenNumber());

  } else {

    color =
      HbColor(defaultColor,
              getHackedbox(),
              getScreenNumber());
  }

  return color;
}


XFontSet HbScreen::readDatabaseFontSet(
  const string &resourceName,
  const string &resourceClass) {

  const char *defaultFont = "fixed";

  bool loadDefault = True;

  XrmValue value;
  char *valueType;

  XFontSet fontSet = 0;

  if (XrmGetResource(resource.stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value) &&
      (fontSet = createFontSet(value.addr))) {

    loadDefault = False;
  }

  if (loadDefault) {
    fontSet = createFontSet(defaultFont);

    if (!fontSet) {
      fprintf(stderr,
              "HbScreen::readDatabaseFontSet(): "
              "couldn't load default font.\n");

      exit(2);
    }
  }

  return fontSet;
}


HbFont *HbScreen::readDatabaseFont(
  const string &resourceName,
  const string &resourceClass) {

  const char *defaultFont = "fixed";

  XrmValue value;
  char *valueType;

  string fontName;

  if (XrmGetResource(resource.stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    fontName = value.addr;

  } else {

    fontName = defaultFont;
  }

  HbFont *font = new HbFont();

  if (!font->load(hackedbox->getXDisplay(),
                  getScreenNumber(),
                  fontName)) {

    fprintf(stderr,
            "HbScreen::readDatabaseFont(): "
            "couldn't load font '%s'\n",
            fontName.c_str());

    delete font;

    font = new HbFont();

    if (!font->load(hackedbox->getXDisplay(),
                    getScreenNumber(),
                    defaultFont)) {

      fprintf(stderr,
              "HbScreen::readDatabaseFont(): "
              "couldn't load default font.\n");

      delete font;
      exit(2);
    }
  }

  return font;
}


static const char *getFontElement(
  const char *pattern,
  char *buffer,
  int bufferSize,
  ...) {

  const char *position;
  const char *value;

  char *bufferPosition;

  va_list arguments;

  va_start(arguments, bufferSize);

  buffer[bufferSize - 1] = 0;
  buffer[bufferSize - 2] = '*';

  while ((value = va_arg(arguments, char *)) != NULL) {

    position = strcasestr(pattern, value);

    if (position) {
      strncpy(buffer,
              position + 1,
              bufferSize - 2);

      buffer[bufferSize - 1] = 0;

      bufferPosition = strchr(buffer, '-');

      if (bufferPosition)
        *bufferPosition = 0;

      va_end(arguments);

      return position;
    }
  }

  va_end(arguments);

  strncpy(buffer, "*", bufferSize - 1);
  buffer[bufferSize - 1] = 0;

  return NULL;
}


static const char *getFontSize(
  const char *pattern,
  int *size) {

  const char *position;
  const char *previousPosition = nullptr;

  int number = 0;

  for (position = pattern; ; ++position) {

    if (!*position) {

      if (previousPosition &&
          number > 1 &&
          number < 72) {

        *size = number;
        return previousPosition + 1;

      } else {

        *size = 16;
        return NULL;
      }

    } else if (*position == '-') {

      if (number > 1 &&
          number < 72 &&
          previousPosition) {

        *size = number;
        return previousPosition + 1;
      }

      previousPosition = position;
      number = 0;

    } else if (*position >= '0' &&
               *position <= '9' &&
               previousPosition) {

      number *= 10;
      number += *position - '0';

    } else {

      previousPosition = NULL;
      number = 0;
    }
  }
}


XFontSet HbScreen::createFontSet(
  const string &fontName) {

  XFontSet fontSet;

  char **missing = 0;
  char *defaultString = const_cast<char *>("-");

  int missingCount = 0;
  int pixelSize = 0;
  int bufferSize = 0;

  char weight[FONT_ELEMENT_SIZE];
  char slant[FONT_ELEMENT_SIZE];

  fontSet =
    XCreateFontSet(hackedbox->getXDisplay(),
                   fontName.c_str(),
                   &missing,
                   &missingCount,
                   &defaultString);

  if (fontSet && !missingCount)
    return fontSet;

#ifdef HAVE_SETLOCALE
  if (!fontSet) {
    if (missingCount)
      XFreeStringList(missing);

    missing = 0;
    missingCount = 0;

    setlocale(LC_CTYPE, "C");

    fontSet =
      XCreateFontSet(hackedbox->getXDisplay(),
                     fontName.c_str(),
                     &missing,
                     &missingCount,
                     &defaultString);

    setlocale(LC_CTYPE, "");

    if (fontSet && !missingCount)
      return fontSet;
  }
#endif

  const char *nativeFontName =
    fontName.c_str();

  if (fontSet) {
    XFontStruct **fontStructs = 0;
    char **fontNames = 0;

    XFontsOfFontSet(fontSet,
                    &fontStructs,
                    &fontNames);

    if (fontNames && fontNames[0])
      nativeFontName = fontNames[0];
  }

  getFontElement(
    nativeFontName,
    weight,
    FONT_ELEMENT_SIZE,
    "-medium-",
    "-bold-",
    "-demibold-",
    "-regular-",
    NULL);

  getFontElement(
    nativeFontName,
    slant,
    FONT_ELEMENT_SIZE,
    "-r-",
    "-i-",
    "-o-",
    "-ri-",
    "-ro-",
    NULL);

  getFontSize(nativeFontName,
              &pixelSize);

  if (!strcmp(weight, "*"))
    strncpy(weight,
            "medium",
            FONT_ELEMENT_SIZE - 1);

  weight[FONT_ELEMENT_SIZE - 1] = 0;

  if (!strcmp(slant, "*"))
    strncpy(slant,
            "r",
            FONT_ELEMENT_SIZE - 1);

  slant[FONT_ELEMENT_SIZE - 1] = 0;

  if (pixelSize < 3)
    pixelSize = 3;
  else if (pixelSize > 97)
    pixelSize = 97;

  bufferSize =
    strlen(nativeFontName) +
    (FONT_ELEMENT_SIZE * 2) +
    64;

  char *pattern =
    new char[bufferSize];

  snprintf(
    pattern,
    bufferSize,
    "%s,"
    "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
    "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
    nativeFontName,
    weight,
    slant,
    pixelSize,
    pixelSize);

  if (missingCount) {
    XFreeStringList(missing);
    missing = 0;
    missingCount = 0;
  }

  if (fontSet) {
    XFreeFontSet(hackedbox->getXDisplay(),
                 fontSet);
    fontSet = 0;
  }

  fontSet =
    XCreateFontSet(hackedbox->getXDisplay(),
                   pattern,
                   &missing,
                   &missingCount,
                   &defaultString);

  if (missingCount)
    XFreeStringList(missing);

  delete [] pattern;

  return fontSet;
}