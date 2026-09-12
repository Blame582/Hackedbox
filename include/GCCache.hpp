// GCCache.hpp for Hackedbox - an X Window manager
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

#ifndef GCCACHE_HPP
#define GCCACHE_HPP

#include <X11/Xlib.h>

#include "Hackedbox.hpp"
#include "BaseDisplay.hpp"
#include "Color.hpp"

class HbFont;
class GCCacheItem;

class GCCacheContext {
public:
  void set(const HbColor &_color,
           const XFontStruct * const _font,
           int _function,
           int _subwindow);

  void set(const XFontStruct * const _font);

  ~GCCacheContext();

private:
  GCCacheContext(const BaseDisplay * const _display)
    : display(_display),
      gc(nullptr),
      pixel(0ul),
      fontid(0ul),
      function(0),
      subwindow(0),
      used(false),
      screen(~0u) {}

  const BaseDisplay *display;
  GC gc;
  unsigned long pixel;
  unsigned long fontid;
  int function;
  int subwindow;
  bool used;
  unsigned int screen;

  GCCacheContext(const GCCacheContext &) = delete;
  GCCacheContext &operator=(const GCCacheContext &) = delete;

  friend class GCCache;
  friend class GCCacheItem;
};


class GCCacheItem {
public:
  const GC &gc() const { return ctx->gc; }

private:
  GCCacheItem()
    : ctx(nullptr),
      count(0),
      hits(0),
      fault(false) {}

  GCCacheContext *ctx;
  unsigned int count;
  unsigned int hits;
  bool fault;

  GCCacheItem(const GCCacheItem &) = delete;
  GCCacheItem &operator=(const GCCacheItem &) = delete;

  friend class GCCache;
};


class GCCache {
public:
  GCCache(const BaseDisplay * const _display, unsigned int screen_count);
  ~GCCache();

  // Clean up the cache.
  void purge();

  GCCacheItem *find(const HbColor &_color,
                     const XFontStruct * const _font = nullptr,
                     int _function = GXcopy,
                     int _subwindow = ClipByChildren);

  void release(GCCacheItem *_item);

private:
  GCCacheContext *nextContext(unsigned int _screen);
  void release(GCCacheContext *ctx);

  // This is closely modelled after the Qt GC cache,
  // but with some of the complexity stripped out.
  const BaseDisplay *display;

  const unsigned int context_count;
  const unsigned int cache_size;
  const unsigned int cache_buckets;
  const unsigned int cache_total_size;

  GCCacheContext **contexts;
  GCCacheItem **cache;
};

class HbPen {
public:
  // Legacy X11 font constructor.
  HbPen(const HbColor &_color,
        const XFontStruct * const _font = nullptr,
        int _function = GXcopy,
        int _subwindow = ClipByChildren)
    : color(_color),
      font(_font),
      hbFont(nullptr),
      function(_function),
      subwindow(_subwindow),
      cache(_color.display()->gcCache()),
      item(nullptr) {}

  // New HbFont/Xft font constructor.
  HbPen(const HbColor &_color,
        const HbFont * const _font,
        int _function = GXcopy,
        int _subwindow = ClipByChildren)
    : color(_color),
      font(nullptr),
      hbFont(_font),
      function(_function),
      subwindow(_subwindow),
      cache(_color.display()->gcCache()),
      item(nullptr) {}

  ~HbPen()
  {
    if (item)
      cache->release(item);
  }

  const GC &gc() const
  {
    if (!item)
      item = cache->find(color, font, function, subwindow);

    return item->gc();
  }

private:
  const HbColor &color;

  // Legacy X11 font.
  const XFontStruct *font;

  // New Xft/HbFont font.
  const HbFont *hbFont;

  int function;
  int subwindow;

  mutable GCCache *cache;
  mutable GCCacheItem *item;
};


#endif // GCCACHE_HPP

