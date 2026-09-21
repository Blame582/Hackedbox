// Image.hpp for Hackedbox - an X Window manager
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

#ifndef HACKEDBOX_IMAGE_HPP
#define HACKEDBOX_IMAGE_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "BaseDisplay.hpp"
#include "Color.hpp"
#include "Timer.hpp"

class HbImageControl;
class HbTexture;


class HbImage {
private:
  HbImageControl *control;

  bool interlaced;

  XColor *colors;

  HbColor from;
  HbColor to;

  int red_offset;
  int green_offset;
  int blue_offset;

  int red_bits;
  int green_bits;
  int blue_bits;

  int ncolors;
  int cpc;
  int cpccpc;

  unsigned char *red;
  unsigned char *green;
  unsigned char *blue;

  unsigned char *red_table;
  unsigned char *green_table;
  unsigned char *blue_table;

  unsigned int width;
  unsigned int height;

  unsigned int *xtable;
  unsigned int *ytable;

  void TrueColorDither(unsigned int bit_depth,
                        int bytes_per_line,
                        unsigned char *pixel_data);

  void PseudoColorDither(int bytes_per_line,
                         unsigned char *pixel_data);

  Pixmap renderPixmap(void);
  

  Pixmap render_solid(const HbTexture &texture);

  Pixmap render_gradient(const HbTexture &texture);

  XImage *renderXImage(void);

  void invert(void);

  void bevel1(void);
  void bevel2(void);

  void dgradient(void);
  void egradient(void);
  void hgradient(void);
  void pgradient(void);
  void rgradient(void);
  void vgradient(void);

  void cdgradient(void);
  void pcgradient(void);

public:
  // Signed dimensions allow invalid sizes to be detected and normalized.
  HbImage(HbImageControl *control, int width, int height);

  ~HbImage(void);

  Pixmap render(const HbTexture &texture);
  
  Pixmap renderARGB(const HbTexture &texture);
};


#endif // HACKEDBOX_IMAGE_HPP
