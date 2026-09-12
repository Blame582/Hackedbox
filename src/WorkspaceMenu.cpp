// Workspacemenu.cpp for Hackedbox - an X Window Manager
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

#include "Hackedbox.hpp"
#include "Screen.hpp"
#include "WorkspaceMenu.hpp"
#include "Workspace.hpp"

Workspacemenu::Workspacemenu(HbScreen *screen)
  : HbBasemenu(screen)
{
  setInternalMenu();

  setLabel("Workspaces");
  insert("New Workspace");
  insert("Remove Last");
}

void Workspacemenu::itemSelected(
  int button,
  unsigned int index
)
{
  if (button != 1)
    return;

  if (index == 0) {
    getScreen()->addWorkspace();
  } else if (index == 1) {
    getScreen()->removeLastWorkspace();
  } else {
    index -= 2;

    const Workspace *workspace =
      getScreen()->getCurrentWorkspace();

    if (workspace->getID() != index &&
        index < getScreen()->getWorkspaceCount()) {
      getScreen()->changeWorkspaceID(index);
    }
  }

  if (!(
    getScreen()->getWorkspacemenu()->isTorn() ||
    isTorn()
  )) {
    hide();
  }
}
