// GCCache.cpp for Hackedbox - an X Window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "BaseDisplay.hpp"
#include "GCCache.hpp"
#include "Color.hpp"
#include "Util.hpp"


GCCacheContext::~GCCacheContext()
{
  if (gc)
    XFreeGC(display->getXDisplay(), gc);
}


void GCCacheContext::set(const HbColor &_color,
                         int _function,
                         int _subwindow)
{
  XGCValues gcv;

  pixel = gcv.foreground = _color.pixel();
  function = gcv.function = _function;
  subwindow = gcv.subwindow_mode = _subwindow;

  XChangeGC(display->getXDisplay(),
             gc,
             GCForeground | GCFunction | GCSubwindowMode,
             &gcv);
}


GCCache::GCCache(const BaseDisplay * const _display,
                 unsigned int screen_count)
  : display(_display),
    context_count(128u),
    cache_size(16u),
    cache_buckets(8u * screen_count),
    cache_total_size(cache_size * cache_buckets)
{
  contexts = new GCCacheContext *[context_count];

  for (unsigned int i = 0; i < context_count; ++i)
    contexts[i] = new GCCacheContext(display);

  cache = new GCCacheItem *[cache_total_size];

  for (unsigned int i = 0; i < cache_total_size; ++i)
    cache[i] = new GCCacheItem;
}


GCCache::~GCCache()
{
  std::for_each(contexts,
                contexts + context_count,
                PointerAssassin());

  std::for_each(cache,
                cache + cache_total_size,
                PointerAssassin());

  delete [] cache;
  delete [] contexts;
}


GCCacheContext *GCCache::nextContext(unsigned int scr)
{
  Window hd = display->getScreenInfo(scr)->getRootWindow();

  for (unsigned int i = 0; i < context_count; ++i) {
    GCCacheContext *c = contexts[i];

    if (!c->gc) {
      c->gc = XCreateGC(display->getXDisplay(),
                        hd,
                        0,
                        nullptr);
      c->used = false;
      c->screen = scr;
    }

    if (!c->used && c->screen == scr)
      return c;
  }

  std::fprintf(stderr, "GCCache: context fault!\n");
  std::abort();

  return nullptr;
}


void GCCache::release(GCCacheContext *ctx)
{
  ctx->used = false;
}


GCCacheItem *GCCache::find(const HbColor &_color,
                           int _function,
                           int _subwindow)
{
  const unsigned long pixel = _color.pixel();
  const unsigned int screen = _color.screen();
  const int key = _color.red() ^ _color.green() ^ _color.blue();

  int k = (key % cache_size) * cache_buckets;
  unsigned int i = 0;

  GCCacheItem *c = cache[k];
  GCCacheItem *prev = nullptr;

  while (c->ctx &&
         (c->ctx->pixel != pixel ||
          c->ctx->function != _function ||
          c->ctx->subwindow != _subwindow ||
          c->ctx->screen != screen)) {

    if (i < (cache_buckets - 1)) {
      prev = c;
      c = cache[++k];
      ++i;
      continue;
    }

    if (c->count == 0 && c->ctx->screen == screen) {
      c->ctx->set(_color, _function, _subwindow);
      c->ctx->used = true;
      c->count = 1;
      c->hits = 1;

      return c;
    }

    std::fprintf(stderr,
                 "GCCache: cache fault, count: %d, screen: %u, "
                 "item screen: %u\n",
                 c->count,
                 screen,
                 c->ctx->screen);

    std::abort();
  }

  if (c->ctx) {
    ++c->count;
    ++c->hits;

    if (prev && c->hits > prev->hits) {
      cache[k] = prev;
      cache[k - 1] = c;
    }
  } else {
    c->ctx = nextContext(screen);
    c->ctx->set(_color, _function, _subwindow);
    c->ctx->used = true;
    c->count = 1;
    c->hits = 1;
  }

  return c;
}


void GCCache::release(GCCacheItem *_item)
{
  if (_item && _item->count > 0)
    --_item->count;
}


void GCCache::purge()
{
  for (unsigned int i = 0; i < cache_total_size; ++i) {
    GCCacheItem *d = cache[i];

    if (d->ctx && d->count == 0) {
      release(d->ctx);
      d->ctx = nullptr;
    }
  }
}