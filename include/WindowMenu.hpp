// WindowMenu.hpp for Hackedbox - an XLibre Window Manager
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

#ifndef HACKEDBOX_WINDOWMENU_HPP
#define HACKEDBOX_WINDOWMENU_HPP

#include "BaseMenu.hpp"

// Forward declarations.
class HackedboxWindow;

class WindowMenu : public HbBasemenu {
private:
  HackedboxWindow *window;

  class SendtoWorkspaceMenu : public HbBasemenu {
  private:
    HackedboxWindow *window;

    SendtoWorkspaceMenu(const SendtoWorkspaceMenu&);
    SendtoWorkspaceMenu& operator=(const SendtoWorkspaceMenu&);

  protected:
    virtual void itemSelected(
      int button,
      unsigned int index
    );

  public:
    explicit SendtoWorkspaceMenu(WindowMenu *menu);

    void update();

    virtual void show();
  };

  SendtoWorkspaceMenu *sendToMenu;

  friend class SendtoWorkspaceMenu;

  WindowMenu(const WindowMenu&);
  WindowMenu& operator=(const WindowMenu&);

protected:
  virtual void itemSelected(
    int button,
    unsigned int index
  );

public:
  explicit WindowMenu(HackedboxWindow *window);
  virtual ~WindowMenu();

  SendtoWorkspaceMenu *getSendToMenu() {
    return sendToMenu;
  }

  void reconfigure();
  void setClosable();

  virtual void show();
};

#endif // HACKEDBOX_WINDOWMENU_HPP
