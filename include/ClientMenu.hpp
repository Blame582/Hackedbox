// ClientMenu.hpp for Hackedbox - an XLibre Window manager
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

#ifndef HACKEDBOX_CLIENTMENU_HPP
#define HACKEDBOX_CLIENTMENU_HPP

#include "BaseMenu.hpp"

class Workspace;

class ClientMenu : public HbBasemenu {
private:
  Workspace *wkspc;

  ClientMenu(const ClientMenu&);
  ClientMenu& operator=(const ClientMenu&);

protected:
  virtual void itemSelected(int button, unsigned int index);

public:
  ClientMenu(Workspace *ws);
};

#endif // HACKEDBOX_CLIENTMENU_HPP
