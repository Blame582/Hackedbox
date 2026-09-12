// Workspace.cpp for Hackedbox - an X Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// Look in the Authors file for credits and copyrights.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without
// limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <assert.h>
#include <algorithm>
#include <string>
#include <vector>

using std::string;

#include "Hackedbox.hpp"
#include "ClientMenu.hpp"
#include "Netizen.hpp"
#include "Screen.hpp"
#include "Util.hpp"
#include "Window.hpp"
#include "Workspace.hpp"
#include "WindowMenu.hpp"


Workspace::Workspace(HbScreen *scrn, unsigned int i) {
  screen = scrn;

  cascadeX = cascadeY = 32;
  id = i;

  clientMenu = new ClientMenu(this);
  lastFocus = nullptr;

  setName(screen->getNameOfWorkspace(id));
}


void Workspace::addWindow(HackedboxWindow *w, bool place) {
  assert(w != nullptr);

  if (place)
    placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList.size());

  stackingList.push_front(w);
  windowList.push_back(w);

  clientMenu->insert(w->getTitle());
  clientMenu->update();

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);
}


unsigned int Workspace::removeWindow(HackedboxWindow *w) {
  assert(w != nullptr);

  stackingList.remove(w);

  if ((w->isFocused() || w == lastFocus) &&
      !screen->getHackedbox()->doShutdown()) {
    focusFallback(w);

    if (w->isStuck()) {
      for (unsigned int i = 0; i < screen->getWorkspaceCount(); ++i) {
        if (i != id)
          screen->getWorkspace(i)->focusFallback(w);
      }
    }
  }

  windowList.remove(w);
  clientMenu->remove(w->getWindowNumber());
  clientMenu->update();

  screen->updateNetizenWindowDel(w->getClientWindow());

  unsigned int i = 0;
  for (HackedboxWindow *window : windowList)
    window->setWindowNumber(i++);

  if (i == 0)
    cascadeX = cascadeY = 32;

  return i;
}


void Workspace::focusFallback(const HackedboxWindow *old_window) {
  HackedboxWindow *newfocus = nullptr;

  if (id == screen->getCurrentWorkspaceID()) {
    if (old_window && old_window->isTransient()) {
      newfocus = old_window->getTransientFor();

      if (!newfocus ||
          newfocus->isIconic() ||
          newfocus->getWorkspaceNumber() != id ||
          !newfocus->setInputFocus())
        newfocus = nullptr;
    }

    if (!newfocus) {
      for (HackedboxWindow *window : stackingList) {
        if (window && window->setInputFocus()) {
          newfocus = window;
          break;
        }
      }
    }

    screen->getHackedbox()->setFocusedWindow(newfocus);
  } else {
    if (old_window && lastFocus == old_window) {
      HackedboxWindow *window = nullptr;

      if (!stackingList.empty())
        window = stackingList.front();

      setLastFocusedWindow(window);
    }
  }
}


void Workspace::removeAll(void) {
  while (!windowList.empty())
    windowList.front()->iconify();
}


/*
 * Returns the number of transients for win, plus the number of transients
 * associated with each transient of win.
 */
static unsigned int countTransients(const HackedboxWindow * const win) {
  const HackedboxWindowList &transients = win->getTransients();

  if (transients.empty())
    return 0;

  unsigned int count = transients.size();

  for (HackedboxWindow *window : transients)
    count += countTransients(window);

  return count;
}


/*
 * Puts the transients of win into the stack.
 */
void Workspace::raiseTransients(const HackedboxWindow * const win,
                                StackVector::iterator &stack) {
  if (win->getTransients().empty())
    return;

  for (HackedboxWindow *window : win->getTransients()) {
    *stack++ = window->getFrameWindow();

    screen->updateNetizenWindowRaise(window->getClientWindow());

    if (!window->isIconic()) {
      Workspace *workspace =
        screen->getWorkspace(window->getWorkspaceNumber());

      workspace->stackingList.remove(window);
      workspace->stackingList.push_front(window);
    }
  }

  for (HackedboxWindow *window : win->getTransients())
    raiseTransients(window, stack);
}


void Workspace::lowerTransients(const HackedboxWindow * const win,
                                StackVector::iterator &stack) {
  if (win->getTransients().empty())
    return;

  const HackedboxWindowList &transients = win->getTransients();

  for (HackedboxWindowList::const_reverse_iterator it = transients.rbegin();
       it != transients.rend(); ++it)
    lowerTransients(*it, stack);

  for (HackedboxWindowList::const_reverse_iterator it = transients.rbegin();
       it != transients.rend(); ++it) {
    HackedboxWindow *window = *it;

    *stack++ = window->getFrameWindow();

    screen->updateNetizenWindowLower(window->getClientWindow());

    if (!window->isIconic()) {
      Workspace *workspace =
        screen->getWorkspace(window->getWorkspaceNumber());

      workspace->stackingList.remove(window);
      workspace->stackingList.push_back(window);
    }
  }
}


void Workspace::raiseWindow(HackedboxWindow *w) {
  HackedboxWindow *window = w;

  while (window->isTransient() && window->getTransientFor())
    window = window->getTransientFor();

  unsigned int count = 1 + countTransients(window);

  StackVector stack_vector(count);
  StackVector::iterator stack = stack_vector.begin();

  *(stack++) = window->getFrameWindow();

  screen->updateNetizenWindowRaise(window->getClientWindow());

  if (!window->isIconic()) {
    Workspace *workspace =
      screen->getWorkspace(window->getWorkspaceNumber());

    workspace->stackingList.remove(window);
    workspace->stackingList.push_front(window);
  }

  raiseTransients(window, stack);

  screen->raiseWindows(stack_vector.data(), stack_vector.size());
}


void Workspace::lowerWindow(HackedboxWindow *w) {
  HackedboxWindow *window = w;

  while (window->isTransient() && window->getTransientFor())
    window = window->getTransientFor();

  unsigned int count = 1 + countTransients(window);

  StackVector stack_vector(count);
  StackVector::iterator stack = stack_vector.begin();

  lowerTransients(window, stack);

  *(stack++) = window->getFrameWindow();

  screen->updateNetizenWindowLower(window->getClientWindow());

  if (!window->isIconic()) {
    Workspace *workspace =
      screen->getWorkspace(window->getWorkspaceNumber());

    workspace->stackingList.remove(window);
    workspace->stackingList.push_back(window);
  }

  Display *display = screen->getBaseDisplay()->getXDisplay();

  XLowerWindow(display, stack_vector.front());
  XRestackWindows(display, stack_vector.data(), stack_vector.size());
}


void Workspace::reconfigure(void) {
  clientMenu->reconfigure();

  for (HackedboxWindow *window : windowList)
    window->reconfigure();
}


HackedboxWindow *Workspace::getWindow(unsigned int index) {
  if (index >= windowList.size())
    return nullptr;

  HackedboxWindowList::iterator it = windowList.begin();

  while (index-- > 0)
    ++it;

  return *it;
}


HackedboxWindow *Workspace::getNextWindowInList(HackedboxWindow *w) {
  HackedboxWindowList::iterator it =
    std::find(windowList.begin(), windowList.end(), w);

  assert(it != windowList.end());

  ++it;

  if (it == windowList.end())
    return windowList.front();

  return *it;
}


HackedboxWindow *Workspace::getPrevWindowInList(HackedboxWindow *w) {
  HackedboxWindowList::iterator it =
    std::find(windowList.begin(), windowList.end(), w);

  assert(it != windowList.end());

  if (it == windowList.begin())
    return windowList.back();

  return *(--it);
}


HackedboxWindow *Workspace::getTopWindowOnStack(void) const {
  assert(!stackingList.empty());
  return stackingList.front();
}


void Workspace::sendWindowList(Netizen &n) {
  for (HackedboxWindow *window : windowList)
    n.sendWindowAdd(window->getClientWindow(), getID());
}


unsigned int Workspace::getCount(void) const {
  return windowList.size();
}


void Workspace::hide(void) {
  HackedboxWindow *focused =
    screen->getHackedbox()->getFocusedWindow();

  if (focused && focused->getScreen() == screen) {
    assert(focused->isStuck() ||
           focused->getWorkspaceNumber() == id);

    lastFocus = focused;
  } else {
    lastFocus = nullptr;
  }

  screen->getHackedbox()->setFocusedWindow(nullptr);

  for (HackedboxWindowList::reverse_iterator it = stackingList.rbegin();
       it != stackingList.rend(); ++it) {
    HackedboxWindow *window = *it;

    if (!window->isStuck())
      window->withdraw();
  }
}


void Workspace::show(void) {
  for (HackedboxWindow *window : stackingList)
    window->show();

  XSync(screen->getHackedbox()->getXDisplay(), False);

  if (screen->doFocusLast()) {
    if (!screen->isSloppyFocus() &&
        !lastFocus &&
        !stackingList.empty())
      lastFocus = stackingList.front();

    if (lastFocus)
      lastFocus->setInputFocus();
  }
}


bool Workspace::isCurrent(void) const {
  return id == screen->getCurrentWorkspaceID();
}


bool Workspace::isLastWindow(const HackedboxWindow * const w) const {
  return w == windowList.back();
}


void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(const string& new_name) {
  if (!new_name.empty()) {
    name = new_name;
  } else {
    char default_name[32];

    snprintf(default_name,
             sizeof(default_name),
             "Workspace %u",
             id + 1);

    name = default_name;
  }

  clientMenu->setLabel(name);
  clientMenu->update();
}


/*
 * Calculate free space available for window placement.
 */
using RectList = std::vector<Rect>;


static RectList calcSpace(const Rect &win, const RectList &spaces) {
  Rect isect;
  Rect extra;
  RectList result;

  for (const Rect &curr : spaces) {
    if (!win.intersects(curr)) {
      result.push_back(curr);
      continue;
    }

    isect = curr & win;

    // Left.
    extra.setCoords(curr.left(),
                    curr.top(),
                    isect.left() - 1,
                    curr.bottom());

    if (extra.valid())
      result.push_back(extra);

    // Top.
    extra.setCoords(curr.left(),
                    curr.top(),
                    curr.right(),
                    isect.top() - 1);

    if (extra.valid())
      result.push_back(extra);

    // Right.
    extra.setCoords(isect.right() + 1,
                    curr.top(),
                    curr.right(),
                    curr.bottom());

    if (extra.valid())
      result.push_back(extra);

    // Bottom.
    extra.setCoords(curr.left(),
                    isect.bottom() + 1,
                    curr.right(),
                    curr.bottom());

    if (extra.valid())
      result.push_back(extra);
  }

  return result;
}


static bool rowRLBT(const Rect &first, const Rect &second) {
  if (first.bottom() == second.bottom())
    return first.right() > second.right();

  return first.bottom() > second.bottom();
}


static bool rowRLTB(const Rect &first, const Rect &second) {
  if (first.y() == second.y())
    return first.right() > second.right();

  return first.y() < second.y();
}


static bool rowLRBT(const Rect &first, const Rect &second) {
  if (first.bottom() == second.bottom())
    return first.x() < second.x();

  return first.bottom() > second.bottom();
}


static bool rowLRTB(const Rect &first, const Rect &second) {
  if (first.y() == second.y())
    return first.x() < second.x();

  return first.y() < second.y();
}


static bool colLRTB(const Rect &first, const Rect &second) {
  if (first.x() == second.x())
    return first.y() < second.y();

  return first.x() < second.x();
}


static bool colLRBT(const Rect &first, const Rect &second) {
  if (first.x() == second.x())
    return first.bottom() > second.bottom();

  return first.x() < second.x();
}


static bool colRLTB(const Rect &first, const Rect &second) {
  if (first.right() == second.right())
    return first.y() < second.y();

  return first.right() > second.right();
}


static bool colRLBT(const Rect &first, const Rect &second) {
  if (first.right() == second.right())
    return first.bottom() > second.bottom();

  return first.right() > second.right();
}


bool Workspace::smartPlacement(Rect& win, const Rect& availableArea) {
  RectList spaces;
  spaces.push_back(availableArea);

  for (const HackedboxWindow *window : windowList) {
    if (window->isShaded())
      continue;

    Rect tmp;

    tmp.setRect(
      window->frameRect().x(),
      window->frameRect().y(),
      window->frameRect().width() + screen->getBorderWidth(),
      window->frameRect().height() + screen->getBorderWidth()
    );

    spaces = calcSpace(tmp, spaces);
  }

  if (screen->getPlacementPolicy() == HbScreen::RowSmartPlacement) {
    if (screen->getRowPlacementDirection() == HbScreen::LeftRight) {
      if (screen->getColPlacementDirection() == HbScreen::TopBottom)
        std::sort(spaces.begin(), spaces.end(), rowLRTB);
      else
        std::sort(spaces.begin(), spaces.end(), rowLRBT);
    } else {
      if (screen->getColPlacementDirection() == HbScreen::TopBottom)
        std::sort(spaces.begin(), spaces.end(), rowRLTB);
      else
        std::sort(spaces.begin(), spaces.end(), rowRLBT);
    }
  } else {
    if (screen->getColPlacementDirection() == HbScreen::TopBottom) {
      if (screen->getRowPlacementDirection() == HbScreen::LeftRight)
        std::sort(spaces.begin(), spaces.end(), colLRTB);
      else
        std::sort(spaces.begin(), spaces.end(), colRLTB);
    } else {
      if (screen->getRowPlacementDirection() == HbScreen::LeftRight)
        std::sort(spaces.begin(), spaces.end(), colLRBT);
      else
        std::sort(spaces.begin(), spaces.end(), colRLBT);
    }
  }

  for (const Rect &space : spaces) {
    if (space.width() >= win.width() &&
        space.height() >= win.height()) {

      win.setX(space.x());
      win.setY(space.y());

      if (screen->getPlacementPolicy() == HbScreen::RowSmartPlacement) {
        if (screen->getRowPlacementDirection() == HbScreen::RightLeft)
          win.setX(space.right() - win.width());

        if (screen->getColPlacementDirection() == HbScreen::BottomTop)
          win.setY(space.bottom() - win.height());
      } else {
        if (screen->getColPlacementDirection() == HbScreen::BottomTop)
          win.setY(win.y() + space.height() - win.height());

        if (screen->getRowPlacementDirection() == HbScreen::RightLeft)
          win.setX(win.x() + space.width() - win.width());
      }

      return true;
    }
  }

  return false;
}


bool Workspace::cascadePlacement(Rect &win,
                                 const Rect &availableArea) {
  if (cascadeX > availableArea.width() / 2 ||
      cascadeY > availableArea.height() / 2)
    cascadeX = cascadeY = 32;

  if (cascadeX == 32) {
    cascadeX += availableArea.x();
    cascadeY += availableArea.y();
  }

  win.setPos(cascadeX, cascadeY);

  return true;
}


void Workspace::placeWindow(HackedboxWindow *win) {
  Rect availableArea(screen->availableArea());

  Rect new_win(
    availableArea.x(),
    availableArea.y(),
    win->frameRect().width(),
    win->frameRect().height()
  );

  bool placed = false;

  switch (screen->getPlacementPolicy()) {
  case HbScreen::RowSmartPlacement:
  case HbScreen::ColSmartPlacement:
    placed = smartPlacement(new_win, availableArea);
    break;

  default:
    break;
  }

  if (!placed) {
    cascadePlacement(new_win, availableArea);

    cascadeX +=
      win->getTitleHeight() +
      (screen->getBorderWidth() * 2);

    cascadeY +=
      win->getTitleHeight() +
      (screen->getBorderWidth() * 2);
  }

  if (new_win.right() > availableArea.right())
    new_win.setX(availableArea.left());

  if (new_win.bottom() > availableArea.bottom())
    new_win.setY(availableArea.top());

  win->configure(
    new_win.x(),
    new_win.y(),
    new_win.width(),
    new_win.height()
  );
}
