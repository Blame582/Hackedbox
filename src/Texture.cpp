// Texture.cpp for Hackedbox - an XLibre Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
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

#include "Hackedbox.hpp"

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <cassert>
#include <cctype>
#include <limits>
#include <string>

#include "Texture.hpp"
#include "ImageControl.hpp"


HbTexture::HbTexture(
    const BaseDisplay *display,
    unsigned int screen,
    HbImageControl *control)
  : c(display, screen),
    ct(display, screen),
    lc(display, screen),
    sc(display, screen),
    descr(),
    t(0),
    dpy(display),
    ctrl(control),
    scrn(screen) {
}


HbTexture::HbTexture(
    const std::string &description,
    const BaseDisplay *display,
    unsigned int screen,
    HbImageControl *control)
  : c(display, screen),
    ct(display, screen),
    lc(display, screen),
    sc(display, screen),
    descr(),
    t(0),
    dpy(display),
    ctrl(control),
    scrn(screen) {

  setDescription(description);
}


void HbTexture::setColor(const HbColor &color) {
  c = color;
  c.setDisplay(display(), screen());

  unsigned char r = c.red();
  unsigned char g = c.green();
  unsigned char b = c.blue();

  unsigned char rr = r + (r >> 1);
  unsigned char gg = g + (g >> 1);
  unsigned char bb = b + (b >> 1);

  if (rr < r)
    rr = ~0;

  if (gg < g)
    gg = ~0;

  if (bb < b)
    bb = ~0;

  lc = HbColor(
    rr,
    gg,
    bb,
    display(),
    screen()
  );

  r = c.red();
  g = c.green();
  b = c.blue();

  rr = (r >> 2) + (r >> 1);
  gg = (g >> 2) + (g >> 1);
  bb = (b >> 2) + (b >> 1);

  if (rr > r)
    rr = 0;

  if (gg > g)
    gg = 0;

  if (bb > b)
    bb = 0;

  sc = HbColor(
    rr,
    gg,
    bb,
    display(),
    screen()
  );
}


void HbTexture::setDescription(const std::string &description) {
  descr.clear();
  descr.reserve(description.length());

  for (char character : description) {
    descr += static_cast<char>(
      std::tolower(static_cast<unsigned char>(character))
    );
  }

  if (descr.find("parentrelative") != std::string::npos) {
    setHbTexture(HbTexture::ParentRelativeTexture);
    return;
  }

  setHbTexture(0);

  if (descr.find("gradient") != std::string::npos) {
    addHbTexture(HbTexture::Gradient);

    if (descr.find("crossdiagonal") != std::string::npos)
      addHbTexture(HbTexture::CrossDiagonal);
    else if (descr.find("rectangle") != std::string::npos)
      addHbTexture(HbTexture::Rectangle);
    else if (descr.find("pyramid") != std::string::npos)
      addHbTexture(HbTexture::Pyramid);
    else if (descr.find("pipecross") != std::string::npos)
      addHbTexture(HbTexture::PipeCross);
    else if (descr.find("elliptic") != std::string::npos)
      addHbTexture(HbTexture::Elliptic);
    else if (descr.find("horizontal") != std::string::npos)
      addHbTexture(HbTexture::Horizontal);
    else if (descr.find("vertical") != std::string::npos)
      addHbTexture(HbTexture::Vertical);
    else
      addHbTexture(HbTexture::Diagonal);
  } else {
    addHbTexture(HbTexture::Solid);
  }

  if (descr.find("sunken") != std::string::npos)
    addHbTexture(HbTexture::Sunken);
  else if (descr.find("flat") != std::string::npos)
    addHbTexture(HbTexture::Flat);
  else
    addHbTexture(HbTexture::Raised);

  if (!(texture() & HbTexture::Flat)) {
    if (descr.find("bevel2") != std::string::npos)
      addHbTexture(HbTexture::Bevel2);
    else
      addHbTexture(HbTexture::Bevel1);
  }

  if (descr.find("interlaced") != std::string::npos)
    addHbTexture(HbTexture::Interlaced);
}


void HbTexture::setDisplay(
    const BaseDisplay *display,
    unsigned int screen) {

  if (display == this->display() &&
      screen == this->screen())
    return;

  dpy = display;
  scrn = screen;

  c.setDisplay(display, screen);
  ct.setDisplay(display, screen);
  lc.setDisplay(display, screen);
  sc.setDisplay(display, screen);
}


HbTexture &HbTexture::operator=(const HbTexture &texture) {
  if (this == &texture)
    return *this;

  c = texture.c;
  ct = texture.ct;
  lc = texture.lc;
  sc = texture.sc;

  descr = texture.descr;
  t = texture.t;

  dpy = texture.dpy;
  ctrl = texture.ctrl;
  scrn = texture.scrn;

  return *this;
}


Pixmap HbTexture::render(
    unsigned int width,
    unsigned int height,
    Pixmap old) {

  assert(display() != nullptr);

  if (texture() == (HbTexture::Flat | HbTexture::Solid))
    return None;

  if (texture() == HbTexture::ParentRelativeTexture)
    return ParentRelative;

  if (screen() == std::numeric_limits<unsigned int>::max())
    scrn = DefaultScreen(display()->getXDisplay());

  assert(ctrl != nullptr);

  Pixmap pixmap = ctrl->renderImage(
    width,
    height,
    *this
  );

  if (old)
    ctrl->removeImage(old);

  return pixmap;
}
