// Rootmenu.cpp for Hackedbox - an X Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// See AUTHORS for additional contributors and historical copyright holders.
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

#include <stdlib.h>
#include <string>
#include <sys/param.h>

#include "Hackedbox.hpp"
#include "RootMenu.hpp"
#include "Runbox.hpp"
#include "Screen.hpp"
#include "Util.hpp"

Rootmenu::Rootmenu(HbScreen *scrn)
  : HbBasemenu(scrn),
    m_runbox(new Runbox())
{
}

Rootmenu::~Rootmenu()
{
  delete m_runbox;
}

void Rootmenu::itemSelected(int button, unsigned int index)
{
  if (button != 1)
    return;

  HbBasemenuItem *item = find(index);

  if (!item->function())
    return;

  if (!(getScreen()->getRootmenu()->isTorn() || isTorn()) &&
      item->function() != HbScreen::Reconfigure &&
      item->function() != HbScreen::SetStyle &&
      item->function() != HbScreen::Runbox)
    hide();

  switch (item->function()) {

  case HbScreen::Runbox:
    if (m_runbox)
      m_runbox->showRunbox();
    return;

  case HbScreen::Execute:
    if (item->exec()) {
      std::string command = item->exec();

      const char *menuFile =
        getScreen()->getHackedbox()->getMenuFilename();

      const char *styleFile =
        getScreen()->getHackedbox()->getStyleFilename();

      std::string::size_type pos;

      pos = command.find("$menu");
      if (pos != std::string::npos)
        command.replace(pos, 5, menuFile);

      pos = command.find("$style");
      if (pos != std::string::npos)
        command.replace(pos, 6, styleFile);

      hbexec(command, getScreen()->displayString());
    }
    break;

  case HbScreen::Restart:
    getScreen()->getHackedbox()->restart();
    break;

  case HbScreen::RestartOther:
    if (item->exec())
      getScreen()->getHackedbox()->restart(item->exec());
    break;

  case HbScreen::Exit:
    getScreen()->getHackedbox()->shutdown();
    break;

  case HbScreen::SetStyle:
    if (item->exec())
      getScreen()->getHackedbox()->saveStyleFilename(item->exec());
    [[fallthrough]];

  case HbScreen::Reconfigure:
    getScreen()->getHackedbox()->reconfigure();
    return;
  }

  if (!(getScreen()->getRootmenu()->isTorn() || isTorn()) &&
      item->function() != HbScreen::Reconfigure &&
      item->function() != HbScreen::SetStyle)
    hide();
}
