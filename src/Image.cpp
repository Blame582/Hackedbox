// Image.cpp for Hackedbox - an X Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
//
// See the AUTHORS file for the complete list of contributors.
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>

using std::max;
using std::min;

#include "BaseDisplay.hpp"
#include "ImageControl.hpp"
#include "Image.hpp"
#include "GCCache.hpp"
#include "Texture.hpp"


HbImage::HbImage(HbImageControl *c, int w, int h)
{
  control = c;

  width = (w > 0) ? w : 1;
  height = (h > 0) ? h : 1;

  red = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue = new unsigned char[width * height];

  xtable = ytable = nullptr;

  cpc = control->getColorsPerChannel();
  cpccpc = cpc * cpc;

  control->getColorTables(&red_table, &green_table, &blue_table,
                          &red_offset, &green_offset, &blue_offset,
                          &red_bits, &green_bits, &blue_bits);

  if (control->getVisual()->c_class != TrueColor)
    control->getXColorTable(&colors, &ncolors);
}


HbImage::~HbImage()
{
  delete [] red;
  delete [] green;
  delete [] blue;
}


Pixmap HbImage::render(const HbTexture &texture)
{
  if (texture.texture() & HbTexture::ParentRelativeTexture)
    return ParentRelative;
  else if (texture.texture() & HbTexture::Solid)
    return render_solid(texture);
  else if (texture.texture() & HbTexture::Gradient)
    return render_gradient(texture);

  return None;
}


Pixmap HbImage::render_solid(const HbTexture &texture)
{
  Pixmap pixmap =
    XCreatePixmap(control->getBaseDisplay()->getXDisplay(),
                  control->getDrawable(),
                  width,
                  height,
                  control->getDepth());

  if (pixmap == None) {
    fprintf(stderr,
            "%s",
            "HbImage::render_solid: error creating pixmap\n");
    return None;
  }

  Display *display = control->getBaseDisplay()->getXDisplay();

  HbPen pen(texture.color());
  HbPen penlight(texture.lightColor());
  HbPen penshadow(texture.shadowColor());

  XFillRectangle(display, pixmap, pen.gc(), 0, 0, width, height);

  if (texture.texture() & HbTexture::Interlaced) {
    HbPen peninterlace(texture.colorTo());

    for (unsigned int i = 0; i < height; i += 2)
      XDrawLine(display, pixmap, peninterlace.gc(), 0, i, width, i);
  }

  if (texture.texture() & HbTexture::Bevel1) {
    if (texture.texture() & HbTexture::Raised) {
      XDrawLine(display, pixmap, penshadow.gc(),
                0, height - 1, width - 1, height - 1);
      XDrawLine(display, pixmap, penshadow.gc(),
                width - 1, height - 1, width - 1, 0);

      XDrawLine(display, pixmap, penlight.gc(),
                0, 0, width - 1, 0);
      XDrawLine(display, pixmap, penlight.gc(),
                0, height - 1, 0, 0);
    }
    else if (texture.texture() & HbTexture::Sunken) {
      XDrawLine(display, pixmap, penlight.gc(),
                0, height - 1, width - 1, height - 1);
      XDrawLine(display, pixmap, penlight.gc(),
                width - 1, height - 1, width - 1, 0);

      XDrawLine(display, pixmap, penshadow.gc(),
                0, 0, width - 1, 0);
      XDrawLine(display, pixmap, penshadow.gc(),
                0, height - 1, 0, 0);
    }
  }
  else if (texture.texture() & HbTexture::Bevel2) {
    if (texture.texture() & HbTexture::Raised) {
      XDrawLine(display, pixmap, penshadow.gc(),
                1, height - 3, width - 3, height - 3);
      XDrawLine(display, pixmap, penshadow.gc(),
                width - 3, height - 3, width - 3, 1);

      XDrawLine(display, pixmap, penlight.gc(),
                1, 1, width - 3, 1);
      XDrawLine(display, pixmap, penlight.gc(),
                1, height - 3, 1, 1);
    }
    else if (texture.texture() & HbTexture::Sunken) {
      XDrawLine(display, pixmap, penlight.gc(),
                1, height - 3, width - 3, height - 3);
      XDrawLine(display, pixmap, penlight.gc(),
                width - 3, height - 3, width - 3, 1);

      XDrawLine(display, pixmap, penshadow.gc(),
                1, 1, width - 3, 1);
      XDrawLine(display, pixmap, penshadow.gc(),
                1, height - 3, 1, 1);
    }
  }

  return pixmap;
}


Pixmap HbImage::render_gradient(const HbTexture &texture)
{
  bool inverted = false;

  interlaced = texture.texture() & HbTexture::Interlaced;

  if (texture.texture() & HbTexture::Sunken) {
    from = texture.colorTo();
    to = texture.color();

    if (!(texture.texture() & HbTexture::Invert))
      inverted = true;
  }
  else {
    from = texture.color();
    to = texture.colorTo();

    if (texture.texture() & HbTexture::Invert)
      inverted = true;
  }

  control->getGradientBuffers(width, height, &xtable, &ytable);

  if (texture.texture() & HbTexture::Diagonal)
    dgradient();
  else if (texture.texture() & HbTexture::Elliptic)
    egradient();
  else if (texture.texture() & HbTexture::Horizontal)
    hgradient();
  else if (texture.texture() & HbTexture::Pyramid)
    pgradient();
  else if (texture.texture() & HbTexture::Rectangle)
    rgradient();
  else if (texture.texture() & HbTexture::Vertical)
    vgradient();
  else if (texture.texture() & HbTexture::CrossDiagonal)
    cdgradient();
  else if (texture.texture() & HbTexture::PipeCross)
    pcgradient();

  if (texture.texture() & HbTexture::Bevel1)
    bevel1();
  else if (texture.texture() & HbTexture::Bevel2)
    bevel2();

  if (inverted)
    invert();

  return renderPixmap();
}


static const unsigned char dither4[4][4] = {
  {0, 4, 1, 5},
  {6, 2, 7, 3},
  {1, 5, 0, 4},
  {7, 3, 6, 2}
};


/*
 * Helper function for TrueColorDither and renderXImage.
 *
 * This handles the proper setting of the image data based on the image depth
 * and the machine's byte ordering.
 */
static inline void assignPixelData(unsigned int bit_depth,
                                   unsigned char **data,
                                   unsigned long pixel)
{
  unsigned char *pixel_data = *data;

  switch (bit_depth) {
  case 8:
    *pixel_data++ = pixel;
    break;

  case 16:
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    break;

  case 17:
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;

  case 24:
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel >> 16;
    break;

  case 25:
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;

  case 32:
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 24;
    break;

  case 33:
    *pixel_data++ = pixel >> 24;
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;
  }

  *data = pixel_data;
}


// Algorithm: ordered dithering.
//
// Portions of this code are based on code from Imlib by rasterman.
void HbImage::TrueColorDither(unsigned int bit_depth,
                              int bytes_per_line,
                              unsigned char *pixel_data)
{
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned char *ppixel_data = pixel_data;
  unsigned long pixel;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 0x3;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 0x3;

      r = red[offset];
      g = green[offset];
      b = blue[offset];

      er = r & (red_bits - 1);
      eg = g & (green_bits - 1);
      eb = b & (blue_bits - 1);

      r = red_table[r];
      g = green_table[g];
      b = blue_table[b];

      if ((dither4[dithy][dithx] < er) &&
          (r < red_table[255]))
        r++;

      if ((dither4[dithy][dithx] < eg) &&
          (g < green_table[255]))
        g++;

      if ((dither4[dithy][dithx] < eb) &&
          (b < blue_table[255]))
        b++;

      pixel = (r << red_offset) |
              (g << green_offset) |
              (b << blue_offset);

      assignPixelData(bit_depth, &pixel_data, pixel);
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}


#ifdef ORDEREDPSEUDO

static const unsigned char dither8[8][8] = {
  { 0, 32, 8, 40, 2, 34, 10, 42},
  {48, 16, 56, 24,50, 18, 58, 26},
  {12, 44, 4, 36,14, 46, 6, 38},
  {60, 28,52, 20,62, 30,54, 22},
  { 3, 35,11, 43, 1, 33, 9, 41},
  {51, 19,59, 27,49, 17,57, 25},
  {15, 47, 7, 39,13, 45, 5, 37},
  {63, 31,55, 23,61, 29,53, 21}
};


void HbImage::OrderedPseudoColorDither(int bytes_per_line,
                                        unsigned char *pixel_data)
{
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned long pixel;
  unsigned char *ppixel_data = pixel_data;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 7;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 7;

      r = red[offset];
      g = green[offset];
      b = blue[offset];

      er = r & (red_bits - 1);
      eg = g & (green_bits - 1);
      eb = b & (blue_bits - 1);

      r = red_table[r];
      g = green_table[g];
      b = blue_table[b];

      if ((dither8[dithy][dithx] < er) &&
          (r < red_table[255]))
        r++;

      if ((dither8[dithy][dithx] < eg) &&
          (g < green_table[255]))
        g++;

      if ((dither8[dithy][dithx] < eb) &&
          (b < blue_table[255]))
        b++;

      pixel = (r * cpccpc) + (g * cpc) + b;
      *(pixel_data++) = colors[pixel].pixel;
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}

#endif


void HbImage::PseudoColorDither(int bytes_per_line,
                                unsigned char *pixel_data)
{
  short *terr;
  short *rerr = new short[width + 2];
  short *gerr = new short[width + 2];
  short *berr = new short[width + 2];
  short *nrerr = new short[width + 2];
  short *ngerr = new short[width + 2];
  short *nberr = new short[width + 2];

  int rr, gg, bb, rer, ger, ber;
  int dd = 255 / control->getColorsPerChannel();

  unsigned int x, y, r, g, b, offset;
  unsigned long pixel;

  unsigned char *ppixel_data = pixel_data;

  for (x = 0; x < width; x++) {
    rerr[x] = red[x];
    gerr[x] = green[x];
    berr[x] = blue[x];
  }

  rerr[x] = gerr[x] = berr[x] = 0;

  for (y = 0, offset = 0; y < height; y++) {
    if (y < (height - 1)) {
      int i = offset + width;

      for (x = 0; x < width; x++, i++) {
        nrerr[x] = red[i];
        ngerr[x] = green[i];
        nberr[x] = blue[i];
      }

      nrerr[x] = red[--i];
      ngerr[x] = green[i];
      nberr[x] = blue[i];
    }

    for (x = 0; x < width; x++) {
      rr = rerr[x];
      gg = gerr[x];
      bb = berr[x];

      if (rr > 255)
        rr = 255;
      else if (rr < 0)
        rr = 0;

      if (gg > 255)
        gg = 255;
      else if (gg < 0)
        gg = 0;

      if (bb > 255)
        bb = 255;
      else if (bb < 0)
        bb = 0;

      r = red_table[rr];
      g = green_table[gg];
      b = blue_table[bb];

      rer = rerr[x] - r * dd;
      ger = gerr[x] - g * dd;
      ber = berr[x] - b * dd;

      pixel = (r * cpccpc) + (g * cpc) + b;
      *pixel_data++ = colors[pixel].pixel;

      r = rer >> 1;
      g = ger >> 1;
      b = ber >> 1;

      rerr[x + 1] += r;
      gerr[x + 1] += g;
      berr[x + 1] += b;

      nrerr[x] += r;
      ngerr[x] += g;
      nberr[x] += b;
    }

    offset += width;
    pixel_data = (ppixel_data += bytes_per_line);

    terr = rerr;
    rerr = nrerr;
    nrerr = terr;

    terr = gerr;
    gerr = ngerr;
    ngerr = terr;

    terr = berr;
    berr = nberr;
    nberr = terr;
  }

  delete [] rerr;
  delete [] gerr;
  delete [] berr;
  delete [] nrerr;
  delete [] ngerr;
  delete [] nberr;
}


XImage *HbImage::renderXImage()
{
  XImage *image =
    XCreateImage(control->getBaseDisplay()->getXDisplay(),
                 control->getVisual(),
                 control->getDepth(),
                 ZPixmap,
                 0,
                 nullptr,
                 width,
                 height,
                 32,
                 0);

  if (!image) {
    fprintf(stderr,
            "%s",
            "HbImage::renderXImage: error creating XImage\n");
    return nullptr;
  }

  image->data = nullptr;

  unsigned char *d =
    new unsigned char[image->bytes_per_line * (height + 1)];

  unsigned int o =
    image->bits_per_pixel +
    ((image->byte_order == MSBFirst) ? 1 : 0);

  bool unsupported = false;

  if (control->doDither() && width > 1 && height > 1) {
    switch (control->getVisual()->c_class) {
    case TrueColor:
      TrueColorDither(o, image->bytes_per_line, d);
      break;

    case StaticColor:
    case PseudoColor:
#ifdef ORDEREDPSEUDO
      OrderedPseudoColorDither(image->bytes_per_line, d);
#else
      PseudoColorDither(image->bytes_per_line, d);
#endif
      break;

    default:
      unsupported = true;
      break;
    }
  }
  else {
    unsigned int x, y, r, g, b, offset;
    unsigned char *pixel_data = d;
    unsigned char *ppixel_data = d;
    unsigned long pixel;

    switch (control->getVisual()->c_class) {
    case StaticColor:
    case PseudoColor:
      for (y = 0, offset = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++offset) {
          r = red_table[red[offset]];
          g = green_table[green[offset]];
          b = blue_table[blue[offset]];

          pixel = (r * cpccpc) + (g * cpc) + b;
          *pixel_data++ = colors[pixel].pixel;
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }
      break;

    case TrueColor:
      for (y = 0, offset = 0; y < height; y++) {
        for (x = 0; x < width; x++, offset++) {
          r = red_table[red[offset]];
          g = green_table[green[offset]];
          b = blue_table[blue[offset]];

          pixel = (r << red_offset) |
                  (g << green_offset) |
                  (b << blue_offset);

          assignPixelData(o, &pixel_data, pixel);
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }
      break;

    case StaticGray:
    case GrayScale:
      for (y = 0, offset = 0; y < height; y++) {
        for (x = 0; x < width; x++, offset++) {
          r = red_table[red[offset]];
          g = green_table[green[offset]];
          b = blue_table[blue[offset]];

          g = ((r * 30) + (g * 59) + (b * 11)) / 100;
          *pixel_data++ = colors[g].pixel;
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }
      break;

    default:
      unsupported = true;
      break;
    }
  }

  if (unsupported) {
    fprintf(stderr,
            "%s",
            "HbImage::renderXImage: unsupported visual\n");

    delete [] d;
    XDestroyImage(image);
    return nullptr;
  }

  image->data = reinterpret_cast<char *>(d);

  return image;
}


Pixmap HbImage::renderPixmap()
{
  Pixmap pixmap =
    XCreatePixmap(control->getBaseDisplay()->getXDisplay(),
                  control->getDrawable(),
                  width,
                  height,
                  control->getDepth());

  if (pixmap == None) {
    fprintf(stderr,
            "%s",
            "HbImage::renderPixmap: error creating pixmap\n");
    return None;
  }

  XImage *image = renderXImage();

  if (!image) {
    XFreePixmap(control->getBaseDisplay()->getXDisplay(), pixmap);
    return None;
  }

  if (!image->data) {
    XDestroyImage(image);
    XFreePixmap(control->getBaseDisplay()->getXDisplay(), pixmap);
    return None;
  }

  Display *display = control->getBaseDisplay()->getXDisplay();

  XPutImage(display,
            pixmap,
            DefaultGC(display,
                      control->getScreenInfo()->getScreenNumber()),
            image,
            0,
            0,
            0,
            0,
            width,
            height);

  if (image->data) {
    delete [] image->data;
    image->data = nullptr;
  }

  XDestroyImage(image);

  return pixmap;
}


void HbImage::bevel1()
{
  if (width > 2 && height > 2) {
    unsigned char *pr = red;
    unsigned char *pg = green;
    unsigned char *pb = blue;

    unsigned char r, g, b, rr, gg, bb;
    unsigned int w = width;
    unsigned int h = height - 1;
    unsigned int wh = w * h;

    while (--w) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;

      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;

      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;

      g = *(pg + wh);
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;

      b = *(pb + wh);
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *((pr++) + wh) = rr;
      *((pg++) + wh) = gg;
      *((pb++) + wh) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;

    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;

    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    r = *(pr + wh);
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;

    g = *(pg + wh);
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;

    b = *(pb + wh);
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *(pr + wh) = rr;
    *(pg + wh) = gg;
    *(pb + wh) = bb;

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;

      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;

      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      pr += width - 1;
      pg += width - 1;
      pb += width - 1;

      r = *pr;
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;

      g = *pg;
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;

      b = *pb;
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *(pr++) = rr;
      *(pg++) = gg;
      *(pb++) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;

    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;

    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    pr += width - 1;
    pg += width - 1;
    pb += width - 1;

    r = *pr;
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;

    g = *pg;
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;

    b = *pb;
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *pr = rr;
    *pg = gg;
    *pb = bb;
  }
}


void HbImage::bevel2()
{
  if (width > 4 && height > 4) {
    unsigned char r, g, b, rr, gg, bb;

    unsigned char *pr = red + width + 1;
    unsigned char *pg = green + width + 1;
    unsigned char *pb = blue + width + 1;

    unsigned int w = width - 2;
    unsigned int h = height - 1;
    unsigned int wh = width * (height - 3);

    while (--w) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;

      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;

      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;

      g = *(pg + wh);
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;

      b = *(pb + wh);
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *((pr++) + wh) = rr;
      *((pg++) + wh) = gg;
      *((pb++) + wh) = bb;
    }

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;

      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;

      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *(++pr) = rr;
      *(++pg) = gg;
      *(++pb) = bb;

      pr += width - 3;
      pg += width - 3;
      pb += width - 3;

      r = *pr;
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;

      g = *pg;
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;

      b = *pb;
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *(pr++) = rr;
      *(pg++) = gg;
      *(pb++) = bb;

      pr++;
      pg++;
      pb++;
    }
  }
}


void HbImage::invert()
{
  unsigned int i;
  unsigned int j = (width * height) - 1;

  unsigned char tmp;

  for (i = 0; j > i; j--, i++) {
    tmp = red[j];
    red[j] = red[i];
    red[i] = tmp;

    tmp = green[j];
    green[j] = green[i];
    green[i] = tmp;

    tmp = blue[j];
    blue[j] = blue[i];
    blue[i] = tmp;
  }
}


void HbImage::dgradient()
{
  float drx, dgx, dbx, dry, dgy, dby;
  float yr = 0.0f, yg = 0.0f, yb = 0.0f;

  float xr = static_cast<float>(from.red());
  float xg = static_cast<float>(from.green());
  float xb = static_cast<float>(from.blue());

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int w = width * 2;
  unsigned int h = height * 2;

  unsigned int *xt = xtable;
  unsigned int *yt = ytable;

  unsigned int x, y;

  dry = drx = static_cast<float>(to.red() - from.red());
  dgy = dgx = static_cast<float>(to.green() - from.green());
  dby = dbx = static_cast<float>(to.blue() - from.blue());

  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = 0; x < width; x++) {
    *(xt++) = static_cast<unsigned char>(xr);
    *(xt++) = static_cast<unsigned char>(xg);
    *(xt++) = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; y++) {
    *(yt++) = static_cast<unsigned char>(yr);
    *(yt++) = static_cast<unsigned char>(yg);
    *(yt++) = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  if (!interlaced) {
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = *(xt++) + *(yt);
        *(pg++) = *(xt++) + *(yt + 1);
        *(pb++) = *(xt++) + *(yt + 2);
      }
    }
  }
  else {
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = *(xt++) + *(yt);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        }
        else {
          channel = *(xt++) + *(yt);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void HbImage::hgradient()
{
  float drx, dgx, dbx;
  float xr = static_cast<float>(from.red());
  float xg = static_cast<float>(from.green());
  float xb = static_cast<float>(from.blue());

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int x, y;

  drx = static_cast<float>(to.red() - from.red());
  dgx = static_cast<float>(to.green() - from.green());
  dbx = static_cast<float>(to.blue() - from.blue());

  drx /= width;
  dgx /= width;
  dbx /= width;

  if (interlaced && height > 2) {
    unsigned char channel, channel2;

    for (x = 0; x < width; x++, pr++, pg++, pb++) {
      channel = static_cast<unsigned char>(xr);
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pr = channel2;

      channel = static_cast<unsigned char>(xg);
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pg = channel2;

      channel = static_cast<unsigned char>(xb);
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pb = channel2;

      channel = static_cast<unsigned char>(xr);
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pr + width) = channel2;

      channel = static_cast<unsigned char>(xg);
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pg + width) = channel2;

      channel = static_cast<unsigned char>(xb);
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pb + width) = channel2;

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    pr += width;
    pg += width;
    pb += width;

    int offset;

    for (y = 2; y < height;
         y++, pr += width, pg += width, pb += width) {
      offset = (y & 1) ? width : 0;

      memcpy(pr, red + offset, width);
      memcpy(pg, green + offset, width);
      memcpy(pb, blue + offset, width);
    }
  }
  else {
    for (x = 0; x < width; x++) {
      *(pr++) = static_cast<unsigned char>(xr);
      *(pg++) = static_cast<unsigned char>(xg);
      *(pb++) = static_cast<unsigned char>(xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    for (y = 1; y < height;
         y++, pr += width, pg += width, pb += width) {
      memcpy(pr, red, width);
      memcpy(pg, green, width);
      memcpy(pb, blue, width);
    }
  }
}


void HbImage::vgradient()
{
  float dry, dgy, dby;

  float yr = static_cast<float>(from.red());
  float yg = static_cast<float>(from.green());
  float yb = static_cast<float>(from.blue());

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int y;

  dry = static_cast<float>(to.red() - from.red());
  dgy = static_cast<float>(to.green() - from.green());
  dby = static_cast<float>(to.blue() - from.blue());

  dry /= height;
  dgy /= height;
  dby /= height;

  if (interlaced) {
    unsigned char channel, channel2;

    for (y = 0; y < height;
         y++, pr += width, pg += width, pb += width) {
      if (y & 1) {
        channel = static_cast<unsigned char>(yr);
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pr, channel2, width);

        channel = static_cast<unsigned char>(yg);
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pg, channel2, width);

        channel = static_cast<unsigned char>(yb);
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pb, channel2, width);
      }
      else {
        channel = static_cast<unsigned char>(yr);
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pr, channel2, width);

        channel = static_cast<unsigned char>(yg);
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pg, channel2, width);

        channel = static_cast<unsigned char>(yb);
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pb, channel2, width);
      }

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  }
  else {
    for (y = 0; y < height;
         y++, pr += width, pg += width, pb += width) {
      memset(pr, static_cast<unsigned char>(yr), width);
      memset(pg, static_cast<unsigned char>(yg), width);
      memset(pb, static_cast<unsigned char>(yb), width);

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  }
}


void HbImage::pgradient()
{
  float yr, yg, yb;
  float drx, dgx, dbx;
  float dry, dgy, dby;
  float xr, xg, xb;

  int rsign, gsign, bsign;

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int tr = to.red();
  unsigned int tg = to.green();
  unsigned int tb = to.blue();

  unsigned int *xt = xtable;
  unsigned int *yt = ytable;

  unsigned int x, y;

  dry = drx = static_cast<float>(to.red() - from.red());
  dgy = dgx = static_cast<float>(to.green() - from.green());
  dby = dbx = static_cast<float>(to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = drx / 2;
  xg = yg = dgx / 2;
  xb = yb = dbx / 2;

  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = static_cast<unsigned char>(xr < 0 ? -xr : xr);
    *(xt++) = static_cast<unsigned char>(xg < 0 ? -xg : xg);
    *(xt++) = static_cast<unsigned char>(xb < 0 ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = static_cast<unsigned char>(yr < 0 ? -yr : yr);
    *(yt++) = static_cast<unsigned char>(yg < 0 ? -yg : yg);
    *(yt++) = static_cast<unsigned char>(yb < 0 ? -yb : yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  if (!interlaced) {
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) =
          static_cast<unsigned char>(tr - (rsign * (*(xt++) + *(yt))));

        *(pg++) =
          static_cast<unsigned char>(tg - (gsign * (*(xt++) + *(yt + 1))));

        *(pb++) =
          static_cast<unsigned char>(tb - (bsign * (*(xt++) + *(yt + 2))));
      }
    }
  }
  else {
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel =
            static_cast<unsigned char>(tr -
              (rsign * (*(xt++) + *(yt))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel =
            static_cast<unsigned char>(tg -
              (gsign * (*(xt++) + *(yt + 1))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel =
            static_cast<unsigned char>(tb -
              (bsign * (*(xt++) + *(yt + 2))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        }
        else {
          channel =
            static_cast<unsigned char>(tr -
              (rsign * (*(xt++) + *(yt))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel =
            static_cast<unsigned char>(tg -
              (gsign * (*(xt++) + *(yt + 1))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel =
            static_cast<unsigned char>(tb -
              (bsign * (*(xt++) + *(yt + 2))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void HbImage::rgradient()
{
  float drx, dgx, dbx, dry, dgy, dby;
  float xr, xg, xb, yr, yg, yb;

  int rsign, gsign, bsign;

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int tr = to.red();
  unsigned int tg = to.green();
  unsigned int tb = to.blue();

  unsigned int *xt = xtable;
  unsigned int *yt = ytable;

  unsigned int x, y;

  dry = drx = static_cast<float>(to.red() - from.red());
  dgy = dgx = static_cast<float>(to.green() - from.green());
  dby = dbx = static_cast<float>(to.blue() - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = drx / 2;
  xg = yg = dgx / 2;
  xb = yb = dby / 2;

  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = static_cast<unsigned char>(xr < 0 ? -xr : xr);
    *(xt++) = static_cast<unsigned char>(xg < 0 ? -xg : xg);
    *(xt++) = static_cast<unsigned char>(xb < 0 ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = static_cast<unsigned char>(yr < 0 ? -yr : yr);
    *(yt++) = static_cast<unsigned char>(yg < 0 ? -yg : yg);
    *(yt++) = static_cast<unsigned char>(yb < 0 ? -yb : yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  if (!interlaced) {
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = static_cast<unsigned char>(
          tr - (rsign * max(*(xt++), *(yt))));

        *(pg++) = static_cast<unsigned char>(
          tg - (gsign * max(*(xt++), *(yt + 1))));

        *(pb++) = static_cast<unsigned char>(
          tb - (bsign * max(*(xt++), *(yt + 2))));
      }
    }
  }
  else {
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = static_cast<unsigned char>(
            tr - (rsign * max(*(xt++), *(yt))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = static_cast<unsigned char>(
            tg - (gsign * max(*(xt++), *(yt + 1))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = static_cast<unsigned char>(
            tb - (bsign * max(*(xt++), *(yt + 2))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        }
        else {
          channel = static_cast<unsigned char>(
            tr - (rsign * max(*(xt++), *(yt))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = static_cast<unsigned char>(
            tg - (gsign * max(*(xt++), *(yt + 1))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = static_cast<unsigned char>(
            tb - (bsign * max(*(xt++), *(yt + 2))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void HbImage::egradient()
{
  float drx, dgx, dbx, dry, dgy, dby;
  float yr, yg, yb, xr, xg, xb;

  int rsign, gsign, bsign;

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int *xt = xtable;
  unsigned int *yt = ytable;

  unsigned int tr = static_cast<unsigned long>(to.red());
  unsigned int tg = static_cast<unsigned long>(to.green());
  unsigned int tb = static_cast<unsigned long>(to.blue());

  unsigned int x, y;

  dry = drx = static_cast<float>(to.red() - from.red());
  dgy = dgx = static_cast<float>(to.green() - from.green());
  dby = dbx = static_cast<float>(to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = drx / 2;
  xg = yg = dgx / 2;
  xb = yb = dbx / 2;

  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = static_cast<unsigned long>(xr * xr);
    *(xt++) = static_cast<unsigned long>(xg * xg);
    *(xt++) = static_cast<unsigned long>(xb * xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = static_cast<unsigned long>(yr * yr);
    *(yt++) = static_cast<unsigned long>(yg * yg);
    *(yt++) = static_cast<unsigned long>(yb * yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  if (!interlaced) {
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = static_cast<unsigned char>(
          tr - (rsign * control->getSqrt(*(xt++) + *(yt))));

        *(pg++) = static_cast<unsigned char>(
          tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));

        *(pb++) = static_cast<unsigned char>(
          tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
      }
    }
  }
  else {
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = static_cast<unsigned char>(
            tr - (rsign * control->getSqrt(*(xt++) + *(yt))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = static_cast<unsigned char>(
            tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = static_cast<unsigned char>(
            tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        }
        else {
          channel = static_cast<unsigned char>(
            tr - (rsign * control->getSqrt(*(xt++) + *(yt))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = static_cast<unsigned char>(
            tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = static_cast<unsigned char>(
            tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void HbImage::pcgradient()
{
  float drx, dgx, dbx, dry, dgy, dby;
  float xr, xg, xb, yr, yg, yb;

  int rsign, gsign, bsign;

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int *xt = xtable;
  unsigned int *yt = ytable;

  unsigned int tr = to.red();
  unsigned int tg = to.green();
  unsigned int tb = to.blue();

  unsigned int x, y;

  dry = drx = static_cast<float>(to.red() - from.red());
  dgy = dgx = static_cast<float>(to.green() - from.green());
  dby = dbx = static_cast<float>(to.blue() - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = drx / 2;
  xg = yg = dgx / 2;
  xb = yb = dbx / 2;

  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = static_cast<unsigned char>(xr < 0 ? -xr : xr);
    *(xt++) = static_cast<unsigned char>(xg < 0 ? -xg : xg);
    *(xt++) = static_cast<unsigned char>(xb < 0 ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = static_cast<unsigned char>(yr < 0 ? -yr : yr);
    *(yt++) = static_cast<unsigned char>(yg < 0 ? -yg : yg);
    *(yt++) = static_cast<unsigned char>(yb < 0 ? -yb : yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  if (!interlaced) {
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = static_cast<unsigned char>(
          tr - (rsign * min(*(xt++), *(yt))));

        *(pg++) = static_cast<unsigned char>(
          tg - (gsign * min(*(xt++), *(yt + 1))));

        *(pb++) = static_cast<unsigned char>(
          tb - (bsign * min(*(xt++), *(yt + 2))));
      }
    }
  }
  else {
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = static_cast<unsigned char>(
            tr - (rsign * min(*(xt++), *(yt))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = static_cast<unsigned char>(
            tg - (gsign * min(*(xt++), *(yt + 1))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = static_cast<unsigned char>(
            tb - (bsign * min(*(xt++), *(yt + 2))));

          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        }
        else {
          channel = static_cast<unsigned char>(
            tr - (rsign * min(*(xt++), *(yt))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = static_cast<unsigned char>(
            tg - (gsign * min(*(xt++), *(yt + 1))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = static_cast<unsigned char>(
            tb - (bsign * min(*(xt++), *(yt + 2))));

          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void HbImage::cdgradient()
{
  float drx, dgx, dbx, dry, dgy, dby;
  float yr = 0.0f, yg = 0.0f, yb = 0.0f;

  float xr = static_cast<float>(from.red());
  float xg = static_cast<float>(from.green());
  float xb = static_cast<float>(from.blue());

  unsigned char *pr = red;
  unsigned char *pg = green;
  unsigned char *pb = blue;

  unsigned int w = width * 2;
  unsigned int h = height * 2;

  unsigned int *xt;
  unsigned int *yt;

  unsigned int x, y;

  dry = drx = static_cast<float>(to.red() - from.red());
  dgy = dgx = static_cast<float>(to.green() - from.green());
  dby = dbx = static_cast<float>(to.blue() - from.blue());

  drx /= w;
  dgx /= w;
  dbx /= w;

  for (xt = xtable + (width * 3) - 1, x = 0;
       x < width;
       x++) {
    *(xt--) = static_cast<unsigned char>(xb);
    *(xt--) = static_cast<unsigned char>(xg);
    *(xt--) = static_cast<unsigned char>(xr);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  dry /= h;
  dgy /= h;
  dby /= h;

  for (yt = ytable, y = 0; y < height; y++) {
    *(yt++) = static_cast<unsigned char>(yr);
    *(yt++) = static_cast<unsigned char>(yg);
    *(yt++) = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  if (!interlaced) {
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = *(xt++) + *(yt);
        *(pg++) = *(xt++) + *(yt + 1);
        *(pb++) = *(xt++) + *(yt + 2);
      }
    }
  }
  else {
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = *(xt++) + *(yt);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        }
        else {
          channel = *(xt++) + *(yt);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}
