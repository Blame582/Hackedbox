// Util.hpp for Hackedbox - an X Window Manager
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

#ifndef HACKEDBOX_UTIL_HPP
#define HACKEDBOX_UTIL_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <string>


class Rect {
public:
  Rect()
    : _x1(0),
      _y1(0),
      _x2(0),
      _y2(0) {
  }

  Rect(
    int x,
    int y,
    unsigned int width,
    unsigned int height
  )
    : _x1(x),
      _y1(y),
      _x2(width + x - 1),
      _y2(height + y - 1) {
  }

  explicit Rect(const XRectangle &rectangle)
    : _x1(rectangle.x),
      _y1(rectangle.y),
      _x2(rectangle.width + rectangle.x - 1),
      _y2(rectangle.height + rectangle.y - 1) {
  }

  int left() const {
    return _x1;
  }

  int top() const {
    return _y1;
  }

  int right() const {
    return _x2;
  }

  int bottom() const {
    return _y2;
  }

  int x() const {
    return _x1;
  }

  int y() const {
    return _y1;
  }

  void setX(int x);
  void setY(int y);
  void setPos(int x, int y);

  unsigned int width() const {
    return _x2 - _x1 + 1;
  }

  unsigned int height() const {
    return _y2 - _y1 + 1;
  }

  void setWidth(unsigned int width);
  void setHeight(unsigned int height);
  void setSize(unsigned int width, unsigned int height);

  void setRect(
    int x,
    int y,
    unsigned int width,
    unsigned int height
  );

  void setCoords(
    int left,
    int top,
    int right,
    int bottom
  );

  bool operator==(const Rect &rect) const {
    return _x1 == rect._x1 &&
           _y1 == rect._y1 &&
           _x2 == rect._x2 &&
           _y2 == rect._y2;
  }

  bool operator!=(const Rect &rect) const {
    return !(*this == rect);
  }

  Rect operator|(const Rect &rect) const;
  Rect operator&(const Rect &rect) const;

  Rect &operator|=(const Rect &rect) {
    *this = *this | rect;
    return *this;
  }

  Rect &operator&=(const Rect &rect) {
    *this = *this & rect;
    return *this;
  }

  bool valid() const {
    return _x2 > _x1 &&
           _y2 > _y1;
  }

  bool intersects(const Rect &rect) const;

private:
  int _x1;
  int _y1;
  int _x2;
  int _y2;
};


/*
 * Sentinel value used where a valid unsigned value is required.
 */
const unsigned int BSENTINEL = 65535;


std::string expandTilde(
  const std::string &path
);


void hbexec(
  const std::string &command,
  const std::string &displayString
);


#ifndef HAVE_BASENAME
std::string basename(
  const std::string &path
);
#endif // HAVE_BASENAME


std::string textPropertyToString(
  Display *display,
  XTextProperty &textProperty
);


struct timeval;

timeval normalizeTimeval(
  const timeval &time
);


struct PointerAssassin {
  template <typename Type>
  void operator()(Type pointer) const {
    delete pointer;
  }
};


std::string itostring(
  unsigned long value
);


std::string itostring(
  long value
);


template <typename Type>
Type next_it(Type value) {
  return ++value;
}


template <typename Type>
Type prior_it(Type value) {
  return --value;
}


#endif // HACKEDBOX_UTIL_HPP
