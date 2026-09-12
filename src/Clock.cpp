// Clock.cpp for Hackedbox - an X Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "Clock.hpp"

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <ctime>
#include <cstring>

Clock::Clock(const BaseDisplay *display)
  : dpy(display),
    x(20),
    y(30),
    rootEnabled(false),
    timeFormat("%I:%M:%S %p"),
    dateFormat("%m/%d/%Y"),
    colorName("#FFFFFF"),
    fontName("Hack-10")
{
  update();
}


Clock::~Clock()
{
}


void Clock::setDisplay(const BaseDisplay *display)
{
  dpy = display;
}


void Clock::setPosition(int newX, int newY)
{
  x = newX;
  y = newY;
}


void Clock::setTimeFormat(const std::string &format)
{
  timeFormat = format;
}


void Clock::setDateFormat(const std::string &format)
{
  dateFormat = format;
}


void Clock::setColor(const std::string &color)
{
  colorName = color;
}


void Clock::setFont(const std::string &font)
{
  fontName = font;
}


void Clock::setRoot(bool enabled)
{
  rootEnabled = enabled;
}


const std::string &Clock::time() const
{
  return currentTime;
}


const std::string &Clock::date() const
{
  return currentDate;
}


void Clock::update()
{
  std::time_t now = std::time(0);
  std::tm *local = std::localtime(&now);

  if (!local)
    return;

  char buffer[256];

  if (!timeFormat.empty()) {
    std::strftime(buffer,
                  sizeof(buffer),
                  timeFormat.c_str(),
                  local);

    currentTime = buffer;
  }

  if (!dateFormat.empty()) {
    std::strftime(buffer,
                  sizeof(buffer),
                  dateFormat.c_str(),
                  local);

    currentDate = buffer;
  }
}


void Clock::drawRoot()
{
  if (!rootEnabled || !dpy)
    return;

  Display *display = dpy->getXDisplay();

  int screen = DefaultScreen(display);

  Window root = RootWindow(display, screen);

  XftDraw *draw =
    XftDrawCreate(display,
                  root,
                  DefaultVisual(display, screen),
                  DefaultColormap(display, screen));

  if (!draw)
    return;

  XftColor color;

  if (!XftColorAllocName(display,
                         DefaultVisual(display, screen),
                         DefaultColormap(display, screen),
                         colorName.c_str(),
                         &color)) {

    XftDrawDestroy(draw);
    return;
  }

  XftFont *font =
    XftFontOpenName(display,
                    screen,
                    fontName.c_str());

  if (!font) {
    XftColorFree(display,
                 DefaultVisual(display, screen),
                 DefaultColormap(display, screen),
                 &color);

    XftDrawDestroy(draw);
    return;
  }

  update();

  XftDrawStringUtf8(draw,
                    &color,
                    font,
                    x,
                    y,
                    reinterpret_cast<const FcChar8 *>(
                      currentTime.c_str()),
                    static_cast<int>(currentTime.length()));

  if (!currentDate.empty()) {
    XftDrawStringUtf8(
      draw,
      &color,
      font,
      x,
      y + font->height,
      reinterpret_cast<const FcChar8 *>(
        currentDate.c_str()),
      static_cast<int>(currentDate.length()));
  }

  XftFontClose(display, font);

  XftColorFree(display,
               DefaultVisual(display, screen),
               DefaultColormap(display, screen),
               &color);

  XftDrawDestroy(draw);

  XFlush(display);
}

void Clock::drawMenu(Window window,
                     unsigned int width,
                     unsigned int height) {
  if (!dpy || window == None)
    return;

  Display *display = dpy->getXDisplay();

  update();

  XftDraw *draw = XftDrawCreate(
    display,
    window,
    DefaultVisual(display, DefaultScreen(display)),
    DefaultColormap(display, DefaultScreen(display))
  );

  if (!draw)
    return;

  XftColor color;

  if (!XftColorAllocName(
        display,
        DefaultVisual(display, DefaultScreen(display)),
        DefaultColormap(display, DefaultScreen(display)),
        colorName.c_str(),
        &color)) {
    XftDrawDestroy(draw);
    return;
  }

  XftFont *font = XftFontOpenName(
    display,
    DefaultScreen(display),
    fontName.c_str()
  );

  if (!font) {
    XftColorFree(
      display,
      DefaultVisual(display, DefaultScreen(display)),
      DefaultColormap(display, DefaultScreen(display)),
      &color
    );
    XftDrawDestroy(draw);
    return;
  }

  const std::string text = currentTime + " " + currentDate;

  XGlyphInfo extents;

  XftTextExtentsUtf8(
    display,
    font,
    reinterpret_cast<const FcChar8 *>(text.c_str()),
    text.length(),
    &extents
  );

  int x = static_cast<int>(width) -
          static_cast<int>(extents.width) -
          6;

  if (x < 0)
    x = 0;

  int y = (static_cast<int>(height) -
           font->ascent -
           font->descent) / 2 +
          font->ascent;

  XftDrawStringUtf8(
    draw,
    &color,
    font,
    x,
    y,
    reinterpret_cast<const FcChar8 *>(text.c_str()),
    text.length()
  );

  XftFontClose(display, font);

  XftColorFree(
    display,
    DefaultVisual(display, DefaultScreen(display)),
    DefaultColormap(display, DefaultScreen(display)),
    &color
  );

  XftDrawDestroy(draw);
}
