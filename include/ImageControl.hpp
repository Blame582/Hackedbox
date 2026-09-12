// ImageControl.hpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
//
// Additional historical authors and contributors are credited in the
// Authors file.
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

#ifndef HACKEDBOX_IMAGECONTROL_HPP
#define HACKEDBOX_IMAGECONTROL_HPP

#include <list>

#include <X11/Xlib.h>

#include "Timer.hpp"
#include "Screen.hpp"

class BaseDisplay;
class HbColor;
class HbTexture;
class ScreenInfo;


class HbImageControl : public TimeoutHandler {
public:
  struct CachedImage {
    Pixmap pixmap;

    unsigned int count;
    unsigned int width;
    unsigned int height;

    unsigned long pixel1;
    unsigned long pixel2;
    unsigned long texture;
  };

  HbImageControl(BaseDisplay *display,
                 const ScreenInfo *screen,
                 bool dither = false,
                 int colors_per_channel = 4,
                 unsigned long cache_timeout = 300000UL,
                 unsigned long cache_max = 200UL);

  virtual ~HbImageControl(void);

  inline BaseDisplay *getBaseDisplay(void) const
  {
    return basedisplay;
  }

  inline bool doDither(void) const
  {
    return dither;
  }

  inline const ScreenInfo *getScreenInfo(void) const
  {
    return screeninfo;
  }

  inline Window getDrawable(void) const
  {
    return window;
  }

  inline Visual *getVisual(void) const
  {
    return screeninfo->getVisual();
  }

  inline int getBitsPerPixel(void) const
  {
    return bits_per_pixel;
  }

  inline int getDepth(void) const
  {
    return screen_depth;
  }

  inline int getColorsPerChannel(void) const
  {
    return colors_per_channel;
  }

  unsigned long getSqrt(unsigned int x);

  Pixmap renderImage(unsigned int width,
                     unsigned int height,
                     const HbTexture &texture);

  void installRootColormap(void);

  void removeImage(Pixmap pixmap);

  void getColorTables(unsigned char **rmt,
                      unsigned char **gmt,
                      unsigned char **bmt,
                      int *roff,
                      int *goff,
                      int *boff,
                      int *rbit,
                      int *gbit,
                      int *bbit);

  void getXColorTable(XColor **colors,
                      int *count);

  void getGradientBuffers(unsigned int width,
                          unsigned int height,
                          unsigned int **xbuffer,
                          unsigned int **ybuffer);

  void setDither(bool dither);

  void setColorsPerChannel(int colors_per_channel);

  virtual void timeout(void);

private:
  bool dither;

  BaseDisplay *basedisplay;

  const ScreenInfo *screeninfo;

#ifdef TIMEDCACHE
  HbTimer *timer;
#endif // TIMEDCACHE

  Colormap colormap;

  Window window;

  XColor *colors;

  int colors_per_channel;
  int ncolors;

  int screen_number;
  int screen_depth;
  int bits_per_pixel;

  int red_offset;
  int green_offset;
  int blue_offset;

  int red_bits;
  int green_bits;
  int blue_bits;

  unsigned char red_color_table[256];
  unsigned char green_color_table[256];
  unsigned char blue_color_table[256];

  unsigned int *grad_xbuffer;
  unsigned int *grad_ybuffer;

  unsigned int grad_buffer_width;
  unsigned int grad_buffer_height;

  unsigned long *sqrt_table;

  unsigned long cache_max;

  using CacheContainer = std::list<CachedImage>;

  CacheContainer cache;

  Pixmap searchCache(unsigned int width,
                     unsigned int height,
                     unsigned long texture,
                     const HbColor &color1,
                     const HbColor &color2);
};

extern HbImageControl *ctrl;

#endif // HACKEDBOX_IMAGECONTROL_HPP
