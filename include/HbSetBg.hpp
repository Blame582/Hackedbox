// HbSetBg.hpp for Hackedbox - an X window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef HBSETBG_HPP
#define HBSETBG_HPP

#include <string>

#include <X11/Xlib.h>

#include "../include/BaseDisplay.hpp"
#include "../include/Timer.hpp"

class HbSetBg :
    public BaseDisplay,
    public TimeoutHandler
{
public:
  HbSetBg(int argc,
          char **argv,
          char *display_name);

  ~HbSetBg() override;

private:
  enum class Mode {
    Center,
    Tile,
    StretchToCenter,
    StretchToEdge,
    Solid
  };

  void process_event(XEvent *event) override;

  bool handleSignal(int signal) override;

  void timeout() override;

  void setBackground();

  void setTimer(long milliseconds);

  Pixmap loadImage(int screen,
                   const std::string &filename,
                   int width,
                   int height);

  Pixmap createSolidPixmap(int screen,
                            int width,
                            int height);

  Pixmap createPixmap(int screen,
                      int width,
                      int height);

  void setPixmapProperty(int screen,
                         Pixmap pixmap);

  void usage(int exit_code = 0);

  std::string image_file;
  std::string color;

  Mode mode;

  HbTimer *timer;
  long timer_interval;
};

#endif // HBSETBG_HPP
