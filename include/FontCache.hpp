// FontCache.hpp for Hackedbox - an X Window manager
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

#ifndef FONTCACHE_HPP
#define FONTCACHE_HPP

#include <map>
#include <string>

#include "Font.hpp"

class BaseDisplay;

class FontCache {
public:
    explicit FontCache(const BaseDisplay * const _display);
    ~FontCache();

    HbFont *find(unsigned int _screen, const std::string &_name);

    HbFont *load(unsigned int _screen, const std::string &_name);

    void purge();

private:
    struct FontKey {
        unsigned int screen;
        std::string name;

        bool operator<(const FontKey &_other) const
        {
            if (screen != _other.screen)
                return screen < _other.screen;

            return name < _other.name;
        }
    };

    const BaseDisplay *display;
    std::map<FontKey, HbFont *> fonts;

    FontCache(const FontCache &) = delete;
    FontCache &operator=(const FontCache &) = delete;
};

#endif // FONTCACHE_HPP