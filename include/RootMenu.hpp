// Rootmenu.hpp for Hackedbox - an XLibre Window Manager
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

#ifndef HACKEDBOX_ROOTMENU_HPP
#define HACKEDBOX_ROOTMENU_HPP

class HbScreen;
class Runbox;

#include "BaseMenu.hpp"

class Rootmenu : public HbBasemenu {
private:
  Rootmenu(const Rootmenu&);
  Rootmenu& operator=(const Rootmenu&);

  Runbox *m_runbox;

protected:
  virtual void itemSelected(int button, unsigned int index)override;

public:
  Rootmenu(HbScreen *scrn);
  ~Rootmenu() override;
};

#endif // HACKEDBOX_ROOTMENU_HPP
