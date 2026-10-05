// FontCache.cpp for Hackedbox - an X Window manager
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

#include "BaseDisplay.hpp"
#include "FontCache.hpp"


FontCache::FontCache(const BaseDisplay * const _display)
    : display(_display)
{
}


FontCache::~FontCache()
{
    purge();
}


HbFont *FontCache::find(unsigned int _screen,
                        const std::string &_name)
{
    FontKey key;
    key.screen = _screen;
    key.name = _name;

    std::map<FontKey, HbFont *>::const_iterator it = fonts.find(key);

    if (it == fonts.end())
        return nullptr;

    return it->second;
}


HbFont *FontCache::load(unsigned int _screen,
                        const std::string &_name)
{
    HbFont *font = find(_screen, _name);

    if (font)
        return font;

    font = new HbFont;

    if (!font->load(display->getXDisplay(), _screen, _name)) {
        delete font;
        return nullptr;
    }

    FontKey key;
    key.screen = _screen;
    key.name = _name;

    fonts[key] = font;

    return font;
}


void FontCache::purge()
{
    for (std::map<FontKey, HbFont *>::iterator it = fonts.begin();
         it != fonts.end();
         ++it) {
        delete it->second;
    }

    fonts.clear();
}