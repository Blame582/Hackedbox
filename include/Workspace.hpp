// Workspace.hpp for Hackedbox - an X Window Manager
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

#ifndef HACKEDBOX_WORKSPACE_HPP
#define HACKEDBOX_WORKSPACE_HPP

#include <X11/Xlib.h>

#include <list>
#include <string>
#include <vector>

#include "Util.hpp"

class HbScreen;
class ClientMenu;
class Workspace;
class HackedboxWindow;
class Netizen;

using HackedboxWindowList = std::list<HackedboxWindow*>;
using StackVector = std::vector<Window>;

class Workspace {
private:
  HbScreen *screen;
  HackedboxWindow *lastFocus;
  ClientMenu *clientMenu;

  HackedboxWindowList stackingList;
  HackedboxWindowList windowList;

  std::string name;
  unsigned int id;
  unsigned int cascadeX;
  unsigned int cascadeY;

  Workspace(const Workspace&);
  Workspace& operator=(const Workspace&);

  void raiseTransients(
    const HackedboxWindow *window,
    StackVector::iterator &stack
  );

  void lowerTransients(
    const HackedboxWindow *window,
    StackVector::iterator &stack
  );

  void placeWindow(HackedboxWindow *window);

  bool cascadePlacement(
    Rect &window,
    const Rect &availableArea
  );

  bool smartPlacement(
    Rect &window,
    const Rect &availableArea
  );

public:
  explicit Workspace(HbScreen *screen, unsigned int id = 0);

  inline HbScreen *getScreen() {
    return screen;
  }

  inline HackedboxWindow *getLastFocusedWindow() {
    return lastFocus;
  }

  inline ClientMenu *getMenu() {
    return clientMenu;
  }

  inline const std::string& getName() const {
    return name;
  }

  inline unsigned int getID() const {
    return id;
  }

  inline void setLastFocusedWindow(HackedboxWindow *window) {
    lastFocus = window;
  }

  HackedboxWindow *getWindow(unsigned int index);

  HackedboxWindow *getNextWindowInList(
    HackedboxWindow *window
  );

  HackedboxWindow *getPrevWindowInList(
    HackedboxWindow *window
  );

  HackedboxWindow *getTopWindowOnStack() const;

  void sendWindowList(Netizen &netizen);

  void focusFallback(
    const HackedboxWindow *oldWindow
  );

  bool isCurrent() const;
  bool isLastWindow(
    const HackedboxWindow *window
  ) const;

  void addWindow(
    HackedboxWindow *window,
    bool place = False
  );

  unsigned int removeWindow(
    HackedboxWindow *window
  );

  unsigned int getCount() const;

  void show();
  void hide();
  void removeAll();

  void raiseWindow(
    HackedboxWindow *window
  );

  void lowerWindow(
    HackedboxWindow *window
  );

  void reconfigure();
  void setCurrent();

  void setName(
    const std::string &newName
  );
};

#endif // HACKEDBOX_WORKSPACE_HPP
