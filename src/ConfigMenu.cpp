// Configmenu.cpp for Hackedbox - an XLibre Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// look in the Authors file for credits and Copyrights
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
# include "../config.h"
#endif // HAVE_CONFIG_H

#include <sys/types.h>
#include <unistd.h>

#include "ConfigMenu.hpp"
#include "ImageControl.hpp"
#include "Screen.hpp"

Configmenu::Configmenu(HbScreen *scr)
  : HbBasemenu(scr)
{
  setLabel("Config options");
  setInternalMenu();

  focusmenu = new Focusmenu(this);
  placementmenu = new Placementmenu(this);

  insert("Focus Model", focusmenu);
  insert("Window Placement", placementmenu);
  insert("Image Dithering", 1);
  insert("Opaque Window Moving", 2);
  insert("Full Maximization", 3);
  insert("Focus New Windows", 4);
  insert("Focus Last Window on Workspace", 5);
  insert("Disable Bindings with Scroll Lock", 6);

  update();

  setItemSelected(2, getScreen()->getImageControl()->doDither());
  setItemSelected(3, getScreen()->doOpaqueMove());
  setItemSelected(4, getScreen()->doFullMax());
  setItemSelected(5, getScreen()->doFocusNew());
  setItemSelected(6, getScreen()->doFocusLast());
  setItemSelected(7, getScreen()->allowScrollLock());
}


Configmenu::~Configmenu()
{
  delete focusmenu;
  delete placementmenu;
}


void Configmenu::itemSelected(
  int button,
  unsigned int index
)
{
  if (button != 1)
    return;

  HbBasemenuItem *item = find(index);

  if (!item->function())
    return;

  switch (item->function()) {

  case 1: {
    getScreen()->getImageControl()->
      setDither(!getScreen()->getImageControl()->doDither());

    setItemSelected(
      index,
      getScreen()->getImageControl()->doDither()
    );

    break;
  }

  case 2: {
    getScreen()->saveOpaqueMove(!getScreen()->doOpaqueMove());

    setItemSelected(
      index,
      getScreen()->doOpaqueMove()
    );

    break;
  }

  case 3: {
    getScreen()->saveFullMax(!getScreen()->doFullMax());

    setItemSelected(
      index,
      getScreen()->doFullMax()
    );

    break;
  }

  case 4: {
    getScreen()->saveFocusNew(!getScreen()->doFocusNew());

    setItemSelected(
      index,
      getScreen()->doFocusNew()
    );

    break;
  }

  case 5: {
    getScreen()->saveFocusLast(!getScreen()->doFocusLast());

    setItemSelected(
      index,
      getScreen()->doFocusLast()
    );

    break;
  }

  case 6: {
    getScreen()->saveAllowScrollLock(
      !getScreen()->allowScrollLock()
    );

    setItemSelected(
      index,
      getScreen()->allowScrollLock()
    );

    getScreen()->reconfigure();

    break;
  }
  }
}

void Configmenu::reconfigure()
{
  focusmenu->reconfigure();
  placementmenu->reconfigure();
}

Configmenu::Focusmenu::Focusmenu(Configmenu *cm)
  : HbBasemenu(cm->getScreen())
{
  setLabel("Focus Model"
  );

  setInternalMenu();

  insert("Click To Focus", 1);
  insert("Sloppy Focus", 2);
  insert("Auto Raise",3);
  insert("Click Raise",4);

  update();

  setItemSelected(
    0,
    !getScreen()->isSloppyFocus()
  );

  setItemSelected(
    1,
    getScreen()->isSloppyFocus()
  );

  setItemEnabled(
    2,
    getScreen()->isSloppyFocus()
  );

  setItemSelected(
    2,
    getScreen()->doAutoRaise()
  );

  setItemEnabled(
    3,
    getScreen()->isSloppyFocus()
  );

  setItemSelected(
    3,
    getScreen()->doClickRaise()
  );
}


void Configmenu::Focusmenu::itemSelected(
  int button,
  unsigned int index
)
{
  if (button != 1)
    return;

  HbBasemenuItem *item = find(index);

  if (!item->function())
    return;

  switch (item->function()) {

  case 1:
    getScreen()->toggleFocusModel(
      HbScreen::ClickToFocus
    );
    break;

  case 2:
    getScreen()->toggleFocusModel(
      HbScreen::SloppyFocus
    );
    break;

  case 3:
    getScreen()->saveAutoRaise(
      !getScreen()->doAutoRaise()
    );
    break;

  case 4:
    getScreen()->saveClickRaise(
      !getScreen()->doClickRaise()
    );

    getScreen()->toggleFocusModel(
      HbScreen::SloppyFocus
    );

    break;
  }

  setItemSelected(
    0,
    !getScreen()->isSloppyFocus()
  );

  setItemSelected(
    1,
    getScreen()->isSloppyFocus()
  );

  setItemEnabled(
    2,
    getScreen()->isSloppyFocus()
  );

  setItemSelected(
    2,
    getScreen()->doAutoRaise()
  );

  setItemEnabled(
    3,
    getScreen()->isSloppyFocus()
  );

  setItemSelected(
    3,
    getScreen()->doClickRaise()
  );
}


Configmenu::Placementmenu::Placementmenu(Configmenu *cm)
  : HbBasemenu(cm->getScreen())
{
  setLabel("Window Placement");
  setInternalMenu();
  insert("Smart Placement (Rows)", HbScreen::RowSmartPlacement);
  insert("Smart Placement (Columns)", HbScreen::ColSmartPlacement);
  insert("Cascade Placement", HbScreen::CascadePlacement);
  insert("Left to Right", HbScreen::LeftRight);
  insert("Right to Left", HbScreen::RightLeft);
  insert("Top to Bottom", HbScreen::TopBottom);
  insert( "Bottom to Top", HbScreen::BottomTop);

  update();

  switch (getScreen()->getPlacementPolicy()) {

  case HbScreen::RowSmartPlacement:
    setItemSelected(0, True);
    break;

  case HbScreen::ColSmartPlacement:
    setItemSelected(1, True);
    break;

  case HbScreen::CascadePlacement:
    setItemSelected(2, True);
    break;
  }

  reconfigure();
}


void Configmenu::Placementmenu::reconfigure()
{
  bool cascade =
    getScreen()->getPlacementPolicy() ==
    HbScreen::CascadePlacement;

  setItemEnabled(3, !cascade);
  setItemEnabled(4, !cascade);
  setItemEnabled(5, !cascade);
  setItemEnabled(6, !cascade);

  bool rl =
    getScreen()->getRowPlacementDirection() ==
    HbScreen::LeftRight;

  bool tb =
    getScreen()->getColPlacementDirection() ==
    HbScreen::TopBottom;

  // Cascade is always LeftRight, TopBottom.

  setItemSelected(3, cascade || rl);
  setItemSelected(4, !cascade && !rl);

  setItemSelected(5, cascade || tb);
  setItemSelected(6, !cascade && !tb);

}


void Configmenu::Placementmenu::itemSelected(
  int button,
  unsigned int index
)
{
  if (button != 1)
    return;

  HbBasemenuItem *item = find(index);

  if (!item->function())
    return;

  switch (item->function()) {

  case HbScreen::RowSmartPlacement:
    getScreen()->savePlacementPolicy(
      item->function()
    );

    setItemSelected(0, True);
    setItemSelected(1, False);
    setItemSelected(2, False);
    break;

  case HbScreen::ColSmartPlacement:
    getScreen()->savePlacementPolicy(
      item->function()
    );

    setItemSelected(0, False);
    setItemSelected(1, True);
    setItemSelected(2, False);
    break;

  case HbScreen::CascadePlacement:
    getScreen()->savePlacementPolicy(
      item->function()
    );

    setItemSelected(0, False);
    setItemSelected(1, False);
    setItemSelected(2, True);
    break;

  case HbScreen::LeftRight:
    getScreen()->saveRowPlacementDirection(
      HbScreen::LeftRight
    );
    break;

  case HbScreen::RightLeft:
    getScreen()->saveRowPlacementDirection(
      HbScreen::RightLeft
    );
    break;

  case HbScreen::TopBottom:
    getScreen()->saveColPlacementDirection(
      HbScreen::TopBottom
    );
    break;

  case HbScreen::BottomTop:
    getScreen()->saveColPlacementDirection(
      HbScreen::BottomTop
    );
    break;
  }

  reconfigure();
}
