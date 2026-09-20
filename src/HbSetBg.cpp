// HbSetBg.cpp for Hackedbox - an X window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <Imlib2.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "BaseDisplay.hpp"
#include "HbSetBg.hpp"


HbSetBg::HbSetBg(int argc, char **argv, char *display_name)
  : BaseDisplay(argv[0], display_name),
    mode(Mode::Center),
    timer(nullptr),
    timer_interval(0)
{
  for (int i = 1; i < argc; ++i) {

    if (!std::strcmp(argv[i], "-help") ||
        !std::strcmp(argv[i], "--help")) {

      usage(0);

    } else if (!std::strcmp(argv[i], "-center") ||
               !std::strcmp(argv[i], "--center")) {

      mode = Mode::Center;

      if (++i >= argc)
        usage(1);

      image_file = argv[i];

    } else if (!std::strcmp(argv[i], "-tile") ||
               !std::strcmp(argv[i], "--tile")) {

      mode = Mode::Tile;

      if (++i >= argc)
        usage(1);

      image_file = argv[i];

    } else if (!std::strcmp(argv[i], "-s2c") ||
               !std::strcmp(argv[i], "--stretch2center")) {

      mode = Mode::StretchToCenter;

      if (++i >= argc)
        usage(1);

      image_file = argv[i];

    } else if (!std::strcmp(argv[i], "-s2e") ||
               !std::strcmp(argv[i], "--stretch2edge")) {

      mode = Mode::StretchToEdge;

      if (++i >= argc)
        usage(1);

      image_file = argv[i];

    } else if (!std::strcmp(argv[i], "-solid") ||
               !std::strcmp(argv[i], "--solid")) {

      mode = Mode::Solid;

      if (++i >= argc)
        usage(1);

      color = argv[i];

    } else if (!std::strcmp(argv[i], "-display") ||
               !std::strcmp(argv[i], "--display")) {

      /*
       * BaseDisplay receives the display name through its constructor.
       * Keep the option accepted for command-line compatibility.
       */

      if (++i >= argc)
        usage(1);

      display_name = argv[i];

    } else {

      /*
       * A bare filename means center mode.
       */

      if (image_file.empty() &&
          color.empty()) {

        image_file = argv[i];

      } else {

        usage(1);
      }
    }
  }

  if (image_file.empty() &&
      color.empty())
    usage(1);

  setBackground();
}


HbSetBg::~HbSetBg()
{
  if (timer)
    {
      timer->stop();
      delete timer;
      timer = nullptr;
    }

  XSetCloseDownMode(
    getXDisplay(),
    RetainPermanent
  );
}


void HbSetBg::setTimer(long milliseconds)
{
  timer_interval = milliseconds;

  if (timer)
  {
    timer->stop();

    delete timer;
    timer = nullptr;
  }

  if (milliseconds <= 0)
    return;

  timer = new HbTimer(
    this,
    this
  );

  timer->setTimeout(milliseconds);
  timer->start();
}


void HbSetBg::timeout()
{
  if (timer_interval <= 0)
    return;

  setBackground();

  timer->setTimeout(timer_interval);
  timer->start();
}


void HbSetBg::process_event(XEvent *event)
{
  (void)event;
}


bool HbSetBg::handleSignal(int signal)
{
  (void)signal;
  return true;
}


void HbSetBg::setBackground()
{
  const unsigned int screens = getNumberOfScreens();

  for (unsigned int screen = 0;
       screen < screens;
       ++screen) {

    const int width =
      getScreenInfo(screen)->getWidth();

    const int height =
      getScreenInfo(screen)->getHeight();

    Pixmap pixmap = None;

    if (mode == Mode::Solid) {

      pixmap =
        createSolidPixmap(
          static_cast<int>(screen),
          width,
          height
        );

    } else {

      pixmap =
        loadImage(
          static_cast<int>(screen),
          image_file,
          width,
          height
        );
    }

    if (!pixmap) {

      if (mode == Mode::Solid) {

        std::fprintf(
          stderr,
          "hbsetbg: unable to create solid color '%s'\n",
          color.c_str()
        );

      } else {

        std::fprintf(
          stderr,
          "hbsetbg: unable to load '%s'\n",
          image_file.c_str()
        );
      }

      continue;
    }

    XSetWindowBackgroundPixmap(
      getXDisplay(),
      getScreenInfo(screen)->getRootWindow(),
      pixmap
    );

    XClearWindow(
      getXDisplay(),
      getScreenInfo(screen)->getRootWindow()
    );

    setPixmapProperty(
      static_cast<int>(screen),
      pixmap
    );
  }

  XFlush(getXDisplay());
}


Pixmap HbSetBg::createSolidPixmap(int screen,
                                  int width,
                                  int height)
{
  Display *display =
    getXDisplay();

  const ScreenInfo *screen_info =
    getScreenInfo(screen);

  const int depth =
    screen_info->getDepth();

  Pixmap pixmap =
    XCreatePixmap(
      display,
      screen_info->getRootWindow(),
      static_cast<unsigned int>(width),
      static_cast<unsigned int>(height),
      static_cast<unsigned int>(depth)
    );

  if (!pixmap) {

    std::fprintf(
      stderr,
      "hbsetbg: XCreatePixmap failed\n"
    );

    return None;
  }

  Colormap colormap =
    DefaultColormap(display, screen);

  XColor xcolor;

  xcolor.red = 0;
  xcolor.green = 0;
  xcolor.blue = 0;
  xcolor.pixel = 0;

  if (!XParseColor(
        display,
        colormap,
        color.c_str(),
        &xcolor)) {

    std::fprintf(
      stderr,
      "hbsetbg: invalid color '%s'\n",
      color.c_str()
    );

    XFreePixmap(
      display,
      pixmap
    );

    return None;
  }

  if (!XAllocColor(
        display,
        colormap,
        &xcolor)) {

    std::fprintf(
      stderr,
      "hbsetbg: unable to allocate color '%s'\n",
      color.c_str()
    );

    XFreePixmap(
      display,
      pixmap
    );

    return None;
  }

  GC gc =
    XCreateGC(
      display,
      pixmap,
      0,
      nullptr
    );

  if (!gc) {

    XFreePixmap(
      display,
      pixmap
    );

    return None;
  }

  XSetForeground(
    display,
    gc,
    xcolor.pixel
  );

  XFillRectangle(
    display,
    pixmap,
    gc,
    0,
    0,
    static_cast<unsigned int>(width),
    static_cast<unsigned int>(height)
  );

  XFreeGC(
    display,
    gc
  );

  return pixmap;
}


Pixmap HbSetBg::loadImage(int screen,
                          const std::string& file,
                          int width,
                          int height)
{
  Display *display =
    getXDisplay();

  const ScreenInfo *screen_info =
    getScreenInfo(screen);

  const int depth =
    screen_info->getDepth();

  /*
   * Configure the Imlib2 X11 context.
   */
  imlib_set_cache_size(0);

  imlib_context_set_display(display);

  imlib_context_set_visual(
    DefaultVisual(display, screen)
  );

  imlib_context_set_colormap(
    DefaultColormap(display, screen)
  );

  Imlib_Image image =
    imlib_load_image(file.c_str());

  if (!image) {

    std::fprintf(
      stderr,
      "hbsetbg: Imlib2 failed to load image '%s'\n",
      file.c_str()
    );

    return None;
  }

  imlib_context_set_image(image);

  const int image_width =
    imlib_image_get_width();

  const int image_height =
    imlib_image_get_height();

  if (image_width <= 0 ||
      image_height <= 0) {

    imlib_free_image();

    return None;
  }

  int target_width =
    image_width;

  int target_height =
    image_height;

  int offset_x =
    (width - target_width) / 2;

  int offset_y =
    (height - target_height) / 2;

  switch (mode) {

    case Mode::Center:
      break;

    case Mode::Tile:
      break;

    case Mode::StretchToCenter:
    {
      const double scale_x =
        static_cast<double>(width) /
        static_cast<double>(image_width);

      const double scale_y =
        static_cast<double>(height) /
        static_cast<double>(image_height);

      const double scale =
        std::min(scale_x, scale_y);

      target_width =
        static_cast<int>(
          image_width * scale
        );

      target_height =
        static_cast<int>(
          image_height * scale
        );

      offset_x =
        (width - target_width) / 2;

      offset_y =
        (height - target_height) / 2;

      break;
    }

    case Mode::StretchToEdge:
      target_width = width;
      target_height = height;
      offset_x = 0;
      offset_y = 0;
      break;

    case Mode::Solid:
      break;
  }

  Pixmap pixmap =
    XCreatePixmap(
      display,
      screen_info->getRootWindow(),
      static_cast<unsigned int>(width),
      static_cast<unsigned int>(height),
      static_cast<unsigned int>(depth)
    );

  if (!pixmap) {

    std::fprintf(
      stderr,
      "hbsetbg: XCreatePixmap failed\n"
    );

    imlib_free_image();

    return None;
  }

  /*
   * Paint the pixmap black first.
   */
  GC gc =
    XCreateGC(
      display,
      pixmap,
      0,
      nullptr
    );

  if (!gc) {

    XFreePixmap(
      display,
      pixmap
    );

    imlib_free_image();

    return None;
  }

  XSetForeground(
    display,
    gc,
    BlackPixel(display, screen)
  );

  XFillRectangle(
    display,
    pixmap,
    gc,
    0,
    0,
    static_cast<unsigned int>(width),
    static_cast<unsigned int>(height)
  );

  XFreeGC(
    display,
    gc
  );

  /*
   * Render the image into the root-sized pixmap.
   */
  imlib_context_set_drawable(pixmap);

  if (mode == Mode::Tile) {

    for (int y = 0;
         y < height;
         y += image_height) {

      for (int x = 0;
           x < width;
           x += image_width) {

        const int tile_width =
          std::min(
            image_width,
            width - x
          );

        const int tile_height =
          std::min(
            image_height,
            height - y
          );

        imlib_context_set_cliprect(
          x,
          y,
          tile_width,
          tile_height
        );

        imlib_render_image_on_drawable(
          x,
          y
        );
      }
    }

    imlib_context_set_cliprect(
      0,
      0,
      0,
      0
    );

  } else if (target_width == image_width &&
             target_height == image_height) {

    imlib_render_image_on_drawable(
      offset_x,
      offset_y
    );

  } else {

    imlib_render_image_on_drawable_at_size(
      offset_x,
      offset_y,
      target_width,
      target_height
    );
  }

  XSync(
    display,
    False
  );

  imlib_free_image();

  return pixmap;
}


Pixmap HbSetBg::createPixmap(int screen,
                             int width,
                             int height)
{
  const ScreenInfo *screen_info =
    getScreenInfo(screen);

  return XCreatePixmap(
    getXDisplay(),
    screen_info->getRootWindow(),
    static_cast<unsigned int>(width),
    static_cast<unsigned int>(height),
    static_cast<unsigned int>(screen_info->getDepth())
  );
}


void HbSetBg::setPixmapProperty(int screen,
                                Pixmap pixmap)
{
  Display *display =
    getXDisplay();

  Window root =
    getScreenInfo(screen)->getRootWindow();

  Atom pixmap_atom =
    XInternAtom(
      display,
      "_XROOTPMAP_ID",
      False
    );

  Atom pixmap_type =
    XInternAtom(
      display,
      "PIXMAP",
      False
    );

  XChangeProperty(
    display,
    root,
    pixmap_atom,
    pixmap_type,
    32,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(&pixmap),
    1
  );

  Atom esetroot_atom =
    XInternAtom(
      display,
      "ESETROOT_PMAP_ID",
      False
    );

  XChangeProperty(
    display,
    root,
    esetroot_atom,
    pixmap_type,
    32,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(&pixmap),
    1
  );
}


void HbSetBg::usage(int exit_code)
{
  std::fprintf(
    exit_code == 0 ? stdout : stderr,
    "hbsetbg 2.0\n"
    "\n"
    "Copyright (c) 2026 Kevin Day\n"
    "\n"
    "Usage:\n"
    "  hbsetbg [options] image\n"
    "\n"
    "Options:\n"
    "  -center, --center <image>\n"
    "                         center image\n"
    "  -tile, --tile <image>\n"
    "                         tile image\n"
    "  -s2c, --stretch2center <image>\n"
    "                         stretch image proportionally and center\n"
    "  -s2e, --stretch2edge <image>\n"
    "                         stretch image to the screen\n"
    "  -solid, --solid <color>\n"
    "                         fill the screen with a solid color. Uses #HEX code.\n"
    "  -display, --display <display>\n"
    "                         X display\n"
    "  -help, --help          show this help\n"
    "\n"
  );

  std::exit(exit_code);
}


int main(int argc,
         char **argv)
{
  if (argc < 2) {

    std::fprintf(
      stderr,
      "hbsetbg: no image or color specified\n"
    );

    return 1;
  }

  HbSetBg app(
    argc,
    argv,
    nullptr
  );

  return 0;
}
