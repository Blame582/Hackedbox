// HbSetBg.cpp is the included back ground setter for Hackedbox - an X Window manager 
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

#include "config.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "BaseDisplay.hpp"
#include "HbSetBg.hpp"
#include "ImageLoader.hpp"

HbSetBg::HbSetBg(int argc, char **argv, char *display_name)
  : BaseDisplay(display_name),
    image_file(),
    color(),
    mode(Center)
{
  for (int i = 1; i < argc; ++i)
  {
    const std::string argument(argv[i]);

    if (argument == "-center" ||
        argument == "--center")
    {
      mode = Center;
      continue;
    }

    if (argument == "-solid" ||
        argument == "--solid")
    {
      if (++i >= argc)
      {
        usage();
        throw std::runtime_error("hbsetbg: -solid requires a color");
      }
      mode = Solid;
      color = argv[i];
      continue;
    }

    if (argument == "-display" ||
        argument == "--display")
    {
      if (++i >= argc)
      {
        throw std::runtime_error("hbsetbg: -display requires a display");
      }

      continue;
    }

    if (!argument.empty() && argument[0] == '-')
    {
      usage();
      throw std::runtime_error("hbsetbg: unknown option: " + argument);
    }

    image_file = argument;
  }

  if (mode == Center && image_file.empty())
  {
    usage();
    throw std::runtime_error("hbsetbg: no image specified");
  }
  setBackground();
}

HbSetBg::~HbSetBg()
{
}

void HbSetBg::process_event(XEvent *event)
{
  (void)event;
}

bool HbSetBg::handleSignal(int sig)
{
  (void)sig;
  return false;
}

void HbSetBg::setBackground()
{
  const int screens = getNumberOfScreens();

  for (int screen = 0; screen < screens; ++screen)
  {
    const int width = getScreenInfo(screen)->getWidth();
    const int height = getScreenInfo(screen)->getHeight();

    Pixmap pixmap = None;

    if (mode == Solid)
    {
      pixmap = createSolidPixmap(screen, width, height, color);
    }
    else
    {
      if (!loadImage(screen, image_file, width, height))
        continue;

      pixmap = createPixmap(screen, width, height);
    }

    if (pixmap == None)
      continue;

    XSetWindowBackgroundPixmap(
      getXDisplay(),
      getScreenInfo(screen)->getRootWindow(),
      pixmap
    );

    XClearWindow(
      getXDisplay(),
      getScreenInfo(screen)->getRootWindow()
    );

    setPixmapProperty(screen, pixmap);
    XFlush(getXDisplay());
  }
}

bool HbSetBg::loadImage(int screen, const std::string &filename,
                        int width, int height)
{
  HbImageData image;
  std::string error;

  if (!HbImageLoader::load(filename, image, error))
  {
    std::cerr << "hbsetbg: " << filename << ": "
              << error << std::endl;
    return false;
  }

  if (static_cast<int>(image.width) != width ||
      static_cast<int>(image.height) != height)
  {
    std::cerr << "hbsetbg: " << filename << ": image is "
              << image.width << "x" << image.height
              << ", but screen " << screen << " requires "
              << width << "x" << height << std::endl;
    return false;
  }

  return true;
}

Pixmap HbSetBg::createPixmap(int screen, int width, int height)
{
  HbImageData image;
  std::string error;

  if (!HbImageLoader::load(image_file, image, error))
  {
    fprintf(stderr, "hbsetbg: %s\n", error.c_str());
    return None;
  }

  if (!image.valid() ||
      image.width != static_cast<unsigned int>(width) ||
      image.height != static_cast<unsigned int>(height))
  {
    fprintf(stderr,
            "hbsetbg: image size does not match screen: "
            "%ux%u required, %ux%u supplied\n",
            width, height, image.width, image.height);
    return None;
  }

  Display *display = getXDisplay();
  const ScreenInfo *screen_info = getScreenInfo(screen);
  Visual *visual = screen_info->getVisual();
  const int depth = screen_info->getDepth();
  const Window root = screen_info->getRootWindow();

  if (visual->c_class != TrueColor &&
      visual->c_class != DirectColor)
  {
    fprintf(stderr,
            "hbsetbg: unsupported visual class for image background\n");
    return None;
  }

  Pixmap pixmap = XCreatePixmap(
    display, root, width, height, depth
  );

  if (pixmap == None)
  {
    fprintf(stderr,
            "hbsetbg: unable to create background pixmap\n");
    return None;
  }

  XImage *ximage = XCreateImage(
    display, visual, depth, ZPixmap, 0, nullptr,
    width, height, 32, 0
  );

  if (!ximage)
  {
    fprintf(stderr, "hbsetbg: unable to create XImage\n");
    XFreePixmap(display, pixmap);
    return None;
  }

  const size_t data_size =
    static_cast<size_t>(ximage->bytes_per_line) *
    static_cast<size_t>(height);

  ximage->data = static_cast<char *>(calloc(1, data_size));

  if (!ximage->data)
  {
    fprintf(stderr,
            "hbsetbg: unable to allocate XImage data\n");
    XDestroyImage(ximage);
    XFreePixmap(display, pixmap);
    return None;
  }

  auto maskValue =
    [](unsigned char channel, unsigned long mask) -> unsigned long
  {
    if (mask == 0)
      return 0;

    unsigned int shift = 0;
    unsigned long maximum = mask;

    while ((maximum & 1UL) == 0)
    {
      maximum >>= 1;
      ++shift;
    }

    const unsigned long value =
      (static_cast<unsigned long>(channel) * maximum + 127UL) / 255UL;

    return value << shift;
  };

  for (unsigned int y = 0; y < image.height; ++y)
  {
    const unsigned char *row =
      image.pixels.data() +
      static_cast<size_t>(y) * image.stride;

    for (unsigned int x = 0; x < image.width; ++x)
    {
      const unsigned char *pixel =
        row + static_cast<size_t>(x) * 4;

      const unsigned long value =
        maskValue(pixel[0], ximage->red_mask) |
        maskValue(pixel[1], ximage->green_mask) |
        maskValue(pixel[2], ximage->blue_mask);

      XPutPixel(ximage, static_cast<int>(x),
                static_cast<int>(y), value);
    }
  }

  XPutImage(
    display, pixmap, DefaultGC(display, screen),
    ximage, 0, 0, 0, 0, width, height
  );

  XDestroyImage(ximage);
  return pixmap;
}

Pixmap HbSetBg::createSolidPixmap(int screen, int width, int height,
                                  const std::string &value)
{
  Display *display = getXDisplay();
  const Window root = getScreenInfo(screen)->getRootWindow();
  const int depth = getScreenInfo(screen)->getDepth();

  Pixmap pixmap = XCreatePixmap(
    display, root, width, height, depth
  );

  if (pixmap == None)
    return None;

  XColor xcolor;

  if (!XParseColor(display,
                   getScreenInfo(screen)->getColormap(),
                   value.c_str(), &xcolor))
  {
    std::cerr << "hbsetbg: invalid color: "
              << value << std::endl;
    XFreePixmap(display, pixmap);
    return None;
  }

  if (!XAllocColor(display,
                   getScreenInfo(screen)->getColormap(),
                   &xcolor))
  {
    std::cerr << "hbsetbg: unable to allocate color: "
              << value << std::endl;
    XFreePixmap(display, pixmap);
    return None;
  }

  GC gc = XCreateGC(display, pixmap, 0, nullptr);

  if (!gc)
  {
    XFreePixmap(display, pixmap);
    return None;
  }

  XSetForeground(display, gc, xcolor.pixel);
  XFillRectangle(display, pixmap, gc, 0, 0, width, height);
  XFreeGC(display, gc);

  return pixmap;
}

void HbSetBg::setPixmapProperty(int screen, Pixmap pixmap)
{
  Display *display = getXDisplay();
  const Window root = getScreenInfo(screen)->getRootWindow();

  Atom root_pixmap =
    XInternAtom(display, "_XROOTPMAP_ID", False);

  Atom esetroot_pixmap =
    XInternAtom(display, "ESETROOT_PMAP_ID", False);

  XChangeProperty(
    display, root, root_pixmap, XA_PIXMAP, 32,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(&pixmap), 1
  );

  XChangeProperty(
    display, root, esetroot_pixmap, XA_PIXMAP, 32,
    PropModeReplace,
    reinterpret_cast<unsigned char *>(&pixmap), 1
  );
}

void HbSetBg::usage()
{
  std::cerr
    << "Usage: hbsetbg [options] image\n"
    << "\n"
    << "Options:\n"
    << "  -center, --center       Set an image background\n"
    << "  -solid,  --solid COLOR  Set a solid color background\n"
    << "  -display, --display DISPLAY\n"
    << "                          Use the specified display\n"
    << std::endl;
}

int main(int argc, char **argv)
{
  char *display_name = nullptr;

  try
  {
    for (int i = 1; i < argc; ++i)
    {
      if (std::strcmp(argv[i], "-display") == 0 ||
          std::strcmp(argv[i], "--display") == 0)
      {
        if (++i >= argc)
        {
          throw std::runtime_error("hbsetbg: -display requires a display");
        }

        display_name = argv[i];
        break;
      }
    }

    HbSetBg app(argc, argv, display_name);
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}