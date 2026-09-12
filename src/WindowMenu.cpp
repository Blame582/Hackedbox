// WindowMenu.cpp for Hackedbox - an X Window Manager
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

#include <X11/Xlib.h>

#include "Hackedbox.hpp"
#include "Screen.hpp"
#include "Window.hpp"
#include "WindowMenu.hpp"
#include "Workspace.hpp"


WindowMenu::WindowMenu(HackedboxWindow *window)
  : HbBasemenu(window->getScreen()),
    window(window) {

  setTitleVisibility(false);
  setMovable(false);
  setInternalMenu();

  sendToMenu = new SendtoWorkspaceMenu(this);

  insert("Send To ...", sendToMenu);
  insert("Shade", HbScreen::WindowShade);
  insert("Iconify", HbScreen::WindowIconify);
  insert("Maximize", HbScreen::WindowMaximize);
  insert("Raise", HbScreen::WindowRaise);
  insert("Lower", HbScreen::WindowLower);
  insert("Stick", HbScreen::WindowStick);
  insert("Kill Client", HbScreen::WindowKill);
  insert("Close", HbScreen::WindowClose);

  update();

  setItemEnabled(1, window->hasTitlebar());
  setItemEnabled(2, window->isIconifiable());
  setItemEnabled(3, window->isMaximizable());
  setItemEnabled(8, window->isClosable());
}


WindowMenu::~WindowMenu() {
  delete sendToMenu;
  sendToMenu = nullptr;
}


void WindowMenu::show() {
  if (isItemEnabled(1))
    setItemSelected(1, window->isShaded());

  if (isItemEnabled(3))
    setItemSelected(3, window->isMaximized());

  if (isItemEnabled(6))
    setItemSelected(6, window->isStuck());

  HbBasemenu::show();
}


void WindowMenu::itemSelected(
  int button,
  unsigned int index) {

  if (button != 1)
    return;

  HbBasemenuItem *item = find(index);

  if (!item)
    return;

  hide();

  switch (item->function()) {

  case HbScreen::WindowShade:
    window->shade();
    break;

  case HbScreen::WindowIconify:
    window->iconify();
    break;

  case HbScreen::WindowMaximize:
    window->maximize(
      static_cast<unsigned int>(button)
    );
    break;

  case HbScreen::WindowClose:
    window->close();
    break;

  case HbScreen::WindowRaise: {
    Workspace *workspace =
      getScreen()->getWorkspace(
        window->getWorkspaceNumber()
      );

    if (workspace)
      workspace->raiseWindow(window);

    break;
  }

  case HbScreen::WindowLower: {
    Workspace *workspace =
      getScreen()->getWorkspace(
        window->getWorkspaceNumber()
      );

    if (workspace)
      workspace->lowerWindow(window);

    break;
  }

  case HbScreen::WindowStick:
    window->stick();
    break;

  case HbScreen::WindowKill:
    XKillClient(
      getScreen()
        ->getBaseDisplay()
        ->getXDisplay(),
      window->getClientWindow()
    );
    break;
  }
}


void WindowMenu::reconfigure() {
  setItemEnabled(
    1,
    window->hasTitlebar()
  );

  setItemEnabled(
    2,
    window->isIconifiable()
  );

  setItemEnabled(
    3,
    window->isMaximizable()
  );

  setItemEnabled(
    8,
    window->isClosable()
  );

  sendToMenu->reconfigure();

  HbBasemenu::reconfigure();
}


WindowMenu::SendtoWorkspaceMenu::SendtoWorkspaceMenu(
  WindowMenu *windowMenu)
  : HbBasemenu(windowMenu->getScreen()),
    window(windowMenu->window) {

  setTitleVisibility(false);
  setMovable(false);
  setInternalMenu();

  update();
}


void WindowMenu::SendtoWorkspaceMenu::itemSelected(
  int button,
  unsigned int index) {

  if (button > 2)
    return;

  if (index <= getScreen()->getWorkspaceCount()) {

    if (index ==
        getScreen()->getCurrentWorkspaceID()) {
      return;
    }

    if (window->isStuck())
      window->stick();

    if (button == 1)
      window->withdraw();

    getScreen()->reassociateWindow(
      window,
      index,
      true
    );

    if (button == 2)
      getScreen()->changeWorkspaceID(index);
  }

  hide();
}


void WindowMenu::SendtoWorkspaceMenu::update() {
  unsigned int count = getCount();

  const unsigned int workspaceCount =
    getScreen()->getWorkspaceCount();


  if (count > workspaceCount) {

    for (unsigned int i = workspaceCount;
         i < count;
         ++i) {
      remove(0);
    }

    count = getCount();
  }


  for (unsigned int i = 0;
       i < workspaceCount;
       ++i) {

    if (count < workspaceCount) {

      insert(
        getScreen()
          ->getWorkspace(i)
          ->getName()
      );

      ++count;

    } else {

      changeItemLabel(
        i,
        getScreen()
          ->getWorkspace(i)
          ->getName()
      );

      setItemEnabled(
        i,
        i != getScreen()->getCurrentWorkspaceID()
      );
    }
  }

  HbBasemenu::update();
}


void WindowMenu::SendtoWorkspaceMenu::show() {
  update();
  HbBasemenu::show();
}
