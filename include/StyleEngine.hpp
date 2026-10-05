// StyleEngine.hpp for Hackedbox - an X Window manager
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

#ifndef __StyleEngine_hpp
#define __StyleEngine_hpp

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <string>

#include "Color.hpp"
#include "Font.hpp"
#include "Texture.hpp"

class Hackedbox;
class HbImageControl;

enum TextJustify {
  LeftJustify = 1,
  RightJustify,
  CenterJustify
};

struct WindowStyle {
  HbColor l_text_focus, l_text_unfocus, b_pic_focus,
    b_pic_unfocus;

  HbTexture f_focus, f_unfocus, t_focus, t_unfocus, l_focus, l_unfocus,
    h_focus, h_unfocus, b_focus, b_unfocus, b_pressed, g_focus, g_unfocus;

  HbFont *font;

  TextJustify justify;

  int doJustify(
    Display *display,
    const char *text,
    int &start_pos,
    unsigned int max_length,
    unsigned int modifier
  ) const;
};

struct MenuStyle {
  HbColor t_text, f_text, h_text, d_text;
  HbColor clock_text, date_text;

  HbTexture title, frame, hilite, sel;

  HbFont *t_font, *f_font;
  HbFont *clock_font, *date_font;

  TextJustify t_justify, f_justify;
  TextJustify clock_justify, date_justify;

  std::string clock_format, date_format;

  int bullet, bullet_pos;

  bool icon;
  int icon_pos;
};

class StyleEngine {
private:
  Hackedbox *hackedbox;
  HbImageControl *image_control;
  unsigned int screen_number;

  XrmDatabase stylerc;

  WindowStyle wstyle;
  MenuStyle mstyle;

  HbColor border_color;

  std::string backgroundFolder;
  int backgroundTimer;

  unsigned int handle_width;
  unsigned int bevel_width;
  unsigned int frame_width;
  unsigned int border_width;

  HbTexture readDatabaseTexture(const std::string &rname,
                                const std::string &rclass,
                                const std::string &default_color);

  HbColor readDatabaseColor(const std::string &rname,
                           const std::string &rclass,
                           const std::string &default_color);

  HbFont *readDatabaseFont(const std::string &rname,
                           const std::string &rclass);

public:
  StyleEngine(Hackedbox *hb,
              HbImageControl *image,
              unsigned int screen);

  ~StyleEngine();

  /*
   * Load a style from either a style file or a style package
   * directory.
   *
   * A style package uses the style directory name as the
   * style file name.
   *
   * Example:
   *
   *   styles/USA/USA
   */
  bool load(const std::string &filename);

  WindowStyle *getWindowStyle()
  { return &wstyle; }

  MenuStyle *getMenuStyle()
  { return &mstyle; }

  HbColor *getBorderColor()
  { return &border_color; }

  unsigned int getHandleWidth() const
  { return handle_width; }

  unsigned int getBevelWidth() const
  { return bevel_width; }

  unsigned int getFrameWidth() const
  { return frame_width; }

  unsigned int getBorderWidth() const
  { return border_width; }

  const std::string &getBackgroundFolder() const
  { return backgroundFolder; }

  int getBackgroundTimer() const
  { return backgroundTimer; }

  XrmDatabase getDatabase() const
  { return stylerc; }
};

#endif // __StyleEngine_hpp