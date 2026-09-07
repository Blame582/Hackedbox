// HbFont.hpp for Hackedbox - an XLibre Window manager
// Copyright (c) 2026 Kevin Day (blame582@gmail.com)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef HFONT_HH
#define HFONT_HH

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <string>

class HbFont {
public:
  HbFont();
  ~HbFont();

  bool load(Display *display, int screen, const std::string &name);

  bool isXft() const;

  XFontStruct *xfont() const;
  XftFont *xftfont() const;

  int ascent() const;
  int descent() const;
  int height() const;

  HbFont(const HbFont &) = delete;
  HbFont &operator=(const HbFont &) = delete;

private:
  Display *display;
  int screen;

  XFontStruct *x_font;
  XftFont *xft_font;
};

#endif // HFONT_HH
