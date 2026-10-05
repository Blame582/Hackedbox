// Font.cpp for Hackedbox - an X Window manager
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

#include "Font.hpp"

#include <cstdio>

HbFont::HbFont()
  : display(nullptr),
    screen(0),
    xft_font(nullptr)
{
}

HbFont::~HbFont()
{
  if (xft_font)
    XftFontClose(display, xft_font);
}

bool HbFont::load(Display *dpy, int scr, const std::string &name)
{
  display = dpy;
  screen = scr;

  if (xft_font) {
    XftFontClose(display, xft_font);
    xft_font = nullptr;
  }

  xft_font = XftFontOpenName(display, screen, name.c_str());

  if (xft_font)
    return true;

  std::fprintf(stderr,
               "HbFont: unable to load Xft font '%s'\n",
               name.c_str());

  return false;
}

XftFont *HbFont::xftfont() const
{
  return xft_font;
}

int HbFont::ascent() const
{
  if (xft_font)
    return xft_font->ascent;

  return 0;
}

int HbFont::descent() const
{
  if (xft_font)
    return xft_font->descent;

  return 0;
}

int HbFont::height() const
{
  return ascent() + descent();
}