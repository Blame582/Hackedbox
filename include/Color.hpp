// Color.hpp for Hackedbox - an X Window manager
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

#ifndef HACKEDBOX_COLOR_HPP
#define HACKEDBOX_COLOR_HPP

#include "BaseDisplay.hpp"

#include <X11/Xlib.h>

#include <map>
#include <string>


class HbColor {
public:
  HbColor(const BaseDisplay * const _display = 0,
         unsigned int _screen = ~(0u));

  HbColor(int _r,
         int _g,
         int _b,
         const BaseDisplay * const _display,
         unsigned int _screen = ~(0u));

  HbColor(const std::string &_name,
         const BaseDisplay * const _display,
         unsigned int _screen = ~(0u));

  ~HbColor(void);


  inline const std::string &name(void) const {
    return colorname;
  }


  inline int red(void) const {
    return r;
  }

  inline int green(void) const {
    return g;
  }

  inline int blue(void) const {
    return b;
  }

  inline int alpha(void) const {
    return a;
  }


  void setRGBA(int _r,
               int _g,
               int _b,
               int _a) {
    deallocate();

    r = _r;
    g = _g;
    b = _b;
    a = _a;
  }

  void setRGB(int _r,
              int _g,
              int _b) {
    setRGBA(_r, _g, _b, 255);
  }


  inline const BaseDisplay *display(void) const {
    return dpy;
  }

  inline unsigned int screen(void) const {
    return scrn;
  }


  void setDisplay(const BaseDisplay * const _display,
                  unsigned int _screen = ~(0u));


  inline bool isAllocated(void) const {
    return allocated;
  }

  inline bool isValid(void) const {
    return r != -1 &&
           g != -1 &&
           b != -1;
  }


  unsigned long pixel(void) const;


  // operators
  HbColor &operator=(const HbColor &c);

  inline bool operator==(const HbColor &c) const {
    return r == c.r &&
           g == c.g &&
           b == c.b &&
           a == c.a;
  }

  inline bool operator!=(const HbColor &c) const {
    return !operator==(c);
  }


  static void cleanupColorCache(void);


private:
  void parseColorName(void);
  void allocate(void);
  void deallocate(void);


  bool allocated;

  int r;
  int g;
  int b;
  int a;

  unsigned long p;

  const BaseDisplay *dpy;
  unsigned int scrn;

  std::string colorname;


  /*
   * Global color allocator/deallocator.
   */
  struct RGB {
    const BaseDisplay * const display;
    const unsigned int screen;

    const int r;
    const int g;
    const int b;


    RGB(void)
      : display(0),
        screen(~(0u)),
        r(-1),
        g(-1),
        b(-1)
    {}


    RGB(const BaseDisplay * const a,
        const unsigned int b,
        const int x,
        const int y,
        const int z)
      : display(a),
        screen(b),
        r(x),
        g(y),
        b(z)
    {}


    RGB(const RGB &x)
      : display(x.display),
        screen(x.screen),
        r(x.r),
        g(x.g),
        b(x.b)
    {}


    inline bool operator==(const RGB &x) const {
      return display == x.display &&
             screen == x.screen &&
             r == x.r &&
             g == x.g &&
             b == x.b;
    }


    inline bool operator<(const RGB &x) const {
      unsigned long p1;
      unsigned long p2;

      p1 =
        (screen << 24 |
         r << 16 |
         g << 8 |
         b) &
        0x00ffffff;

      p2 =
        (x.screen << 24 |
         x.r << 16 |
         x.g << 8 |
         x.b) &
        0x00ffffff;

      return p1 < p2;
    }
  };


  struct PixelRef {
    const unsigned long p;
    unsigned int count;


    inline PixelRef(void)
      : p(0),
        count(0)
    {}


    inline PixelRef(const unsigned long x)
      : p(x),
        count(1)
    {}
  };


  typedef std::map<RGB, PixelRef> ColorCache;
  typedef ColorCache::value_type ColorCacheItem;


  static ColorCache colorcache;
  static bool cleancache;

  static void doCacheCleanup(void);
};


#endif // HACKEDBOX_COLOR_HPP