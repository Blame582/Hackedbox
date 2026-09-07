// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// Util.cpp for Hackedbox - an XLibre Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// Look in the Authors file for credits and copyrights.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished, subject to the following conditions:
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

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Util.hpp"


void Rect::setX(int x) {
  _x2 += x - _x1;
  _x1 = x;
}


void Rect::setY(int y) {
  _y2 += y - _y1;
  _y1 = y;
}


void Rect::setPos(int x, int y) {
  _x2 += x - _x1;
  _x1 = x;

  _y2 += y - _y1;
  _y1 = y;
}


void Rect::setWidth(unsigned int width) {
  _x2 = width + _x1 - 1;
}


void Rect::setHeight(unsigned int height) {
  _y2 = height + _y1 - 1;
}


void Rect::setSize(
  unsigned int width,
  unsigned int height
) {
  _x2 = width + _x1 - 1;
  _y2 = height + _y1 - 1;
}


void Rect::setRect(
  int x,
  int y,
  unsigned int width,
  unsigned int height
) {
  *this = Rect(x, y, width, height);
}


void Rect::setCoords(
  int left,
  int top,
  int right,
  int bottom
) {
  _x1 = left;
  _y1 = top;
  _x2 = right;
  _y2 = bottom;
}


Rect Rect::operator|(const Rect &rect) const {
  Rect result;

  result._x1 = std::min(_x1, rect._x1);
  result._y1 = std::min(_y1, rect._y1);
  result._x2 = std::max(_x2, rect._x2);
  result._y2 = std::max(_y2, rect._y2);

  return result;
}


Rect Rect::operator&(const Rect &rect) const {
  Rect result;

  result._x1 = std::max(_x1, rect._x1);
  result._y1 = std::max(_y1, rect._y1);
  result._x2 = std::min(_x2, rect._x2);
  result._y2 = std::min(_y2, rect._y2);

  return result;
}


bool Rect::intersects(const Rect &rect) const {
  return std::max(_x1, rect._x1) <= std::min(_x2, rect._x2) &&
         std::max(_y1, rect._y1) <= std::min(_y2, rect._y2);
}


std::string expandTilde(const std::string &path) {
  if (path.empty() || path[0] != '~')
    return path;

  const char *home = std::getenv("HOME");

  if (!home)
    return path;

  return std::string(home) +
         path.substr(path.find('/'));
}


void hbexec(
  const std::string &command,
  const std::string &displayString
) {
  if (!fork()) {
    setsid();

    const int result =
      putenv(
        const_cast<char *>(displayString.c_str())
      );

    assert(result != -1);

    std::string cmd = "exec ";
    cmd += command;

    const int exitCode =
      execl(
        "/bin/sh",
        "/bin/sh",
        "-c",
        cmd.c_str(),
        static_cast<char *>(nullptr)
      );

    std::exit(exitCode);
  }
}


#ifndef HAVE_BASENAME

std::string basename(const std::string &path) {
  const std::string::size_type slash =
    path.rfind('/');

  if (slash == std::string::npos)
    return path;

  return path.substr(slash + 1);
}

#endif // HAVE_BASENAME


std::string textPropertyToString(
  Display *display,
  XTextProperty &textProperty
) {
  std::string result;

  if (textProperty.value &&
      textProperty.nitems > 0) {

    if (textProperty.encoding == XA_STRING) {

      result =
        reinterpret_cast<char *>(
          textProperty.value
        );

    } else {

      textProperty.nitems =
        std::strlen(
          reinterpret_cast<char *>(
            textProperty.value
          )
        );

      char **list = nullptr;
      int count = 0;

      if (XmbTextPropertyToTextList(
            display,
            &textProperty,
            &list,
            &count
          ) == Success &&
          count > 0 &&
          list &&
          *list) {

        result = *list;

        XFreeStringList(list);
      }
    }
  }

  return result;
}


timeval normalizeTimeval(
  const timeval &time
) {
  timeval result = time;

  while (result.tv_usec < 0) {

    if (result.tv_sec > 0) {
      --result.tv_sec;
      result.tv_usec += 1000000;

    } else {
      result.tv_usec = 0;
    }
  }


  if (result.tv_usec >= 1000000) {

    result.tv_sec +=
      result.tv_usec / 1000000;

    result.tv_usec %=
      1000000;
  }


  if (result.tv_sec < 0)
    result.tv_sec = 0;

  return result;
}


std::string itostring(unsigned long value) {
  if (value == 0)
    return "0";

  const char numbers[] = "0123456789";

  std::string result;

  for (; value > 0; value /= 10)
    result.insert(
      result.begin(),
      numbers[value % 10]
    );

  return result;
}


std::string itostring(long value) {
  std::string result =
    itostring(
      static_cast<unsigned long>(
        std::abs(value)
      )
    );

  if (value < 0)
    result.insert(
      result.begin(),
      '-'
    );

  return result;
}
