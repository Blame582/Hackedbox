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
#include <X11/Xft/Xft.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <functional>
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
  style_engine = 0;

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

  style_engine =
    new StyleEngine(hackedbox,
                    image_control,
                    getScreenNumber());

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

  HbFont *font =
    style_engine->getWindowStyle()->font;

  if (font && font->xftfont()) {
    XGlyphInfo extents;

    XftTextExtentsUtf8(
      hackedbox->getXDisplay(),
      font->xftfont(),
      reinterpret_cast<const FcChar8 *>(s),
      strlen(s),
      &extents);

    geom_w = extents.xOff;
    geom_h = font->height();
  } else {
    geom_w = 0;
    geom_h = 0;
  }

  geom_w += style_engine->getBevelWidth() * 2;
  geom_h += style_engine->getBevelWidth() * 2;

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
                  style_engine->getBorderWidth(),
                  getDepth(),
                  InputOutput,
                  getVisual(),
                  mask,
                  &attrib);

  geom_visible = False;

  HbTexture *texture = &style_engine->getWindowStyle()->l_focus;

  geom_pixmap =
    texture->render(geom_w,
                    geom_h,
                    geom_pixmap);

  if (geom_pixmap == ParentRelative) {
    texture = &style_engine->getWindowStyle()->t_focus;

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
  delete style_engine;
  delete image_control;

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

  HbFont *font =
    style_engine->getWindowStyle()->font;

  if (font && font->xftfont()) {
    XGlyphInfo extents;

    XftTextExtentsUtf8(
      hackedbox->getXDisplay(),
      font->xftfont(),
      reinterpret_cast<const FcChar8 *>(s),
      strlen(s),
      &extents);

    geom_w = extents.xOff;
    geom_h = font->height();
  } else {
    geom_w = 0;
    geom_h = 0;
  }

  geom_w += style_engine->getBevelWidth() * 2;
  geom_h += style_engine->getBevelWidth() * 2;

  HbTexture *texture = &style_engine->getWindowStyle()->l_focus;

  geom_pixmap =
    texture->render(geom_w,
                    geom_h,
                    geom_pixmap);

  if (geom_pixmap == ParentRelative) {
    texture = &style_engine->getWindowStyle()->t_focus;

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
                        style_engine->getBorderWidth());

  XSetWindowBorder(hackedbox->getXDisplay(),
                   geom_window,
                   style_engine->getBorderColor()->pixel());

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
  style_engine->load(hackedbox->getStyleFilename());
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

  HbFont *font =
    style_engine->getWindowStyle()->font;

  if (!font || !font->xftfont())
    return;

  XftDraw *draw =
    XftDrawCreate(
      hackedbox->getXDisplay(),
      geom_window,
      getVisual(),
      getColormap());

  if (!draw)
    return;

  XRenderColor render_color;

  const HbColor &text_color =
    style_engine->getWindowStyle()->l_text_focus;

  render_color.red =
    static_cast<unsigned short>(text_color.red() * 257U);
  render_color.green =
    static_cast<unsigned short>(text_color.green() * 257U);
  render_color.blue =
    static_cast<unsigned short>(text_color.blue() * 257U);
  render_color.alpha =
    static_cast<unsigned short>(text_color.alpha() * 257U);

  XftColor color;

  if (XftColorAllocValue(
        hackedbox->getXDisplay(),
        getVisual(),
        getColormap(),
        &render_color,
        &color)) {

    XftDrawStringUtf8(
      draw,
      &color,
      font->xftfont(),
      style_engine->getBevelWidth(),
      font->ascent() +
        style_engine->getBevelWidth(),
      reinterpret_cast<const FcChar8 *>(label),
      strlen(label));

    XftColorFree(
      hackedbox->getXDisplay(),
      getVisual(),
      getColormap(),
      &color);
  }

  XftDrawDestroy(draw);
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

  HbFont *font =
    style_engine->getWindowStyle()->font;

  if (!font || !font->xftfont())
    return;

  XftDraw *draw =
    XftDrawCreate(
      hackedbox->getXDisplay(),
      geom_window,
      getVisual(),
      getColormap());

  if (!draw)
    return;

  XRenderColor render_color;

  const HbColor &text_color =
    style_engine->getWindowStyle()->l_text_focus;

  render_color.red =
    static_cast<unsigned short>(text_color.red() * 257U);
  render_color.green =
    static_cast<unsigned short>(text_color.green() * 257U);
  render_color.blue =
    static_cast<unsigned short>(text_color.blue() * 257U);
  render_color.alpha =
    static_cast<unsigned short>(text_color.alpha() * 257U);

  XftColor color;

  if (XftColorAllocValue(
        hackedbox->getXDisplay(),
        getVisual(),
        getColormap(),
        &render_color,
        &color)) {

    XftDrawStringUtf8(
      draw,
      &color,
      font->xftfont(),
      style_engine->getBevelWidth(),
      font->ascent() +
        style_engine->getBevelWidth(),
      reinterpret_cast<const FcChar8 *>(label),
      strlen(label));

    XftColorFree(
      hackedbox->getXDisplay(),
      getVisual(),
      getColormap(),
      &color);
  }

  XftDrawDestroy(draw);
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