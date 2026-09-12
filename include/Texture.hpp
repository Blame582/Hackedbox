// Texture.hpp for Hackedbox - an X Window Manager
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

#ifndef HACKEDBOX_TEXTURE_HPP
#define HACKEDBOX_TEXTURE_HPP

#include <limits>
#include <string>

#include "Hackedbox.hpp"
#include "Color.hpp"

class HbImageControl;

class HbTexture {
public:
  enum Type {
    // Bevel options
    Flat          = (1UL << 0),
    Sunken        = (1UL << 1),
    Raised        = (1UL << 2),

    // HbTextures
    Solid         = (1UL << 3),
    Gradient      = (1UL << 4),

    // Gradients
    Horizontal    = (1UL << 5),
    Vertical      = (1UL << 6),
    Diagonal      = (1UL << 7),
    CrossDiagonal = (1UL << 8),
    Rectangle     = (1UL << 9),
    Pyramid       = (1UL << 10),
    PipeCross     = (1UL << 11),
    Elliptic      = (1UL << 12),

    // Bevel types
    Bevel1        = (1UL << 13),
    Bevel2        = (1UL << 14),

    // Inverted image
    Invert        = (1UL << 15),

    // Parent-relative image
    ParentRelativeTexture = (1UL << 16),

    // Fake interlaced image
    Interlaced    = (1UL << 17)
  };

  HbTexture(
    const  BaseDisplay * display = nullptr,
    unsigned int screen = std::numeric_limits<unsigned int>::max(),
    HbImageControl *control = nullptr
  );

  HbTexture(
    const std::string &description,
    const  BaseDisplay * display = nullptr,
    unsigned int screen = std::numeric_limits<unsigned int>::max(),
    HbImageControl *control = nullptr
  );

  void setColor(const HbColor &color);

  void setColorTo(const HbColor &colorTo) {
    ct = colorTo;
  }

  const HbColor &color() const {
    return c;
  }

  const HbColor &colorTo() const {
    return ct;
  }

  const HbColor &lightColor() const {
    return lc;
  }

  const HbColor &shadowColor() const {
    return sc;
  }

  unsigned long texture() const {
    return t;
  }

  void setHbTexture(unsigned long texture) {
    t = texture;
  }

  void addHbTexture(unsigned long texture) {
    t |= texture;
  }

  HbTexture &operator=(const HbTexture &texture);

  bool operator==(const HbTexture &texture) const {
    return c == texture.c &&
           ct == texture.ct &&
           lc == texture.lc &&
           sc == texture.sc &&
           t == texture.t;
  }

  bool operator!=(const HbTexture &texture) const {
    return !(*this == texture);
  }

  const BaseDisplay *display() const {
    return dpy;
  }

  unsigned int screen() const {
    return scrn;
  }

  void setDisplay(
    const BaseDisplay * display,
    unsigned int screen
  );

  void setHbImageControl(HbImageControl *control) {
    ctrl = control;
  }

  const std::string &description() const {
    return descr;
  }

  void setDescription(const std::string &description);

  Pixmap render(
    unsigned int width,
    unsigned int height,
    Pixmap old = 0
  );

private:
  HbColor c;
  HbColor ct;
  HbColor lc;
  HbColor sc;

  std::string descr;

  unsigned long t;

  const BaseDisplay *dpy;
  HbImageControl *ctrl;

  unsigned int scrn;
};

#endif // HACKEDBOX_TEXTURE_HPP
