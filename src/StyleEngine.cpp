// StyleEngine.cpp for Hackedbox - an X Window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <locale.h>
#include <string>
#include <strings.h>
#include <unistd.h>

#include "Hackedbox.hpp"
#include "BaseMenu.hpp"
#include "ImageControl.hpp"
#include "StyleEngine.hpp"
#include "Util.hpp"


static std::string findStyleFile(const std::string &path) {
  namespace fs = std::filesystem;

  fs::path stylePath(path);

  /*
   * A style can be loaded directly from a file, or from a
   * style package directory.
   *
   * Example:
   *
   *   ~/.hackedbox/styles/USA/
   *       USA
   *       background.webp
   *
   * The style file has the same name as the style directory.
   */
  if (!fs::is_directory(stylePath))
    return path;

  fs::path styleFile =
    stylePath / stylePath.filename();

  if (fs::is_regular_file(styleFile))
    return styleFile.string();

  return std::string();
}


StyleEngine::StyleEngine(Hackedbox *hb, HbImageControl *image, unsigned int screen)
  : hackedbox(hb),
    image_control(image),
    screen_number(screen),
    stylerc(0),
    backgroundFolder(""),
    backgroundTimer(0),
    handle_width(6),
    bevel_width(3),
    frame_width(3),
    border_width(1) {

  wstyle.font = 0;

  mstyle.t_font = 0;
  mstyle.f_font = 0;
  mstyle.clock_font = 0;
  mstyle.date_font = 0;
}


StyleEngine::~StyleEngine() {
  if (stylerc)
    XrmDestroyDatabase(stylerc);

  delete wstyle.font;
  delete mstyle.f_font;
  delete mstyle.t_font;
  delete mstyle.clock_font;
  delete mstyle.date_font;
}


bool StyleEngine::load(const std::string &filename) {
  FILE *debug =
    fopen("/tmp/hackedbox-image.log", "a");

  if (debug) {
    fprintf(debug,
            "StyleEngine::load: %s\n",
            filename.c_str());
    fclose(debug);
  }

  if (stylerc)
    XrmDestroyDatabase(stylerc);

  std::string styleFile =
    findStyleFile(filename);

  if (!styleFile.empty())
    stylerc =
      XrmGetFileDatabase(styleFile.c_str());

  if (!stylerc)
    stylerc =
      XrmGetFileDatabase(DEFAULTSTYLE);

  backgroundTimer = 0;

  XrmValue value;
  char *valueType;

  delete wstyle.font;
  delete mstyle.f_font;
  delete mstyle.t_font;
  delete mstyle.clock_font;
  delete mstyle.date_font;

  wstyle.font = 0;
  mstyle.f_font = 0;
  mstyle.t_font = 0;
  mstyle.clock_font = 0;
  mstyle.date_font = 0;

  wstyle.font =
    readDatabaseFont("window.font",
                     "Window.Font");

  mstyle.t_font =
    readDatabaseFont("menu.title.font",
                     "Menu.Title.Font");

  mstyle.f_font =
    readDatabaseFont("menu.frame.font",
                     "Menu.Frame.Font");

  mstyle.clock_font =
    readDatabaseFont("menu.clock.font",
                     "Menu.Clock.Font");

  mstyle.date_font =
    readDatabaseFont("menu.date.font",
                     "Menu.Date.Font");

  wstyle.t_focus =
    readDatabaseTexture("window.title.focus",
                        "Window.Title.Focus",
                        "white");

  wstyle.t_unfocus =
    readDatabaseTexture("window.title.unfocus",
                        "Window.Title.Unfocus",
                        "black");

  wstyle.l_focus =
    readDatabaseTexture("window.label.focus",
                        "Window.Label.Focus",
                        "white");

  wstyle.l_unfocus =
    readDatabaseTexture("window.label.unfocus",
                        "Window.Label.Unfocus",
                        "black");

  wstyle.h_focus =
    readDatabaseTexture("window.handle.focus",
                        "Window.Handle.Focus",
                        "white");

  wstyle.h_unfocus =
    readDatabaseTexture("window.handle.unfocus",
                        "Window.Handle.Unfocus",
                        "black");

  wstyle.g_focus =
    readDatabaseTexture("window.grip.focus",
                        "Window.Grip.Focus",
                        "white");

  wstyle.g_unfocus =
    readDatabaseTexture("window.grip.unfocus",
                        "Window.Grip.Unfocus",
                        "black");

  wstyle.b_focus =
    readDatabaseTexture("window.button.focus",
                        "Window.Button.Focus",
                        "white");

  wstyle.b_unfocus =
    readDatabaseTexture("window.button.unfocus",
                        "Window.Button.Unfocus",
                        "black");

  wstyle.b_pressed =
    readDatabaseTexture("window.button.pressed",
                        "Window.Button.Pressed",
                        "black");

  HbColor color =
    readDatabaseColor("window.frame.focusColor",
                      "Window.Frame.FocusColor",
                      "white");

  wstyle.f_focus =
    HbTexture("solid flat",
              hackedbox,
              screen_number,
              image_control);

  wstyle.f_focus.setColor(color);

  color =
    readDatabaseColor("window.frame.unfocusColor",
                      "Window.Frame.UnfocusColor",
                      "white");

  wstyle.f_unfocus =
    HbTexture("solid flat",
              hackedbox,
              screen_number,
              image_control);

  wstyle.f_unfocus.setColor(color);

  wstyle.l_text_focus =
    readDatabaseColor("window.label.focus.textColor",
                      "Window.Label.Focus.TextColor",
                      "black");

  wstyle.l_text_unfocus =
    readDatabaseColor("window.label.unfocus.textColor",
                      "Window.Label.Unfocus.TextColor",
                      "white");

  wstyle.b_pic_focus =
    readDatabaseColor("window.button.focus.picColor",
                      "Window.Button.Focus.PicColor",
                      "black");

  wstyle.b_pic_unfocus =
    readDatabaseColor("window.button.unfocus.picColor",
                      "Window.Button.Unfocus.PicColor",
                      "white");

  wstyle.justify = LeftJustify;

  if (XrmGetResource(stylerc,
                     "window.justify",
                     "Window.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      wstyle.justify = RightJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      wstyle.justify = CenterJustify;
    }
  }

  if (wstyle.t_focus.texture() ==
      HbTexture::ParentRelativeTexture)
    wstyle.t_focus = wstyle.f_focus;

  if (wstyle.t_unfocus.texture() ==
      HbTexture::ParentRelativeTexture)
    wstyle.t_unfocus = wstyle.f_unfocus;

  if (wstyle.h_focus.texture() ==
      HbTexture::ParentRelativeTexture)
    wstyle.h_focus = wstyle.f_focus;

  if (wstyle.h_unfocus.texture() ==
      HbTexture::ParentRelativeTexture)
    wstyle.h_unfocus = wstyle.f_unfocus;

  mstyle.title =
    readDatabaseTexture("menu.title",
                        "Menu.Title",
                        "white");

  mstyle.frame =
    readDatabaseTexture("menu.frame",
                        "Menu.Frame",
                        "black");

  mstyle.hilite =
    readDatabaseTexture("menu.hilite",
                        "Menu.Hilite",
                        "white");

  mstyle.t_text =
    readDatabaseColor("menu.title.textColor",
                      "Menu.Title.TextColor",
                      "black");

  mstyle.f_text =
    readDatabaseColor("menu.frame.textColor",
                      "Menu.Frame.TextColor",
                      "white");

  mstyle.d_text =
    readDatabaseColor("menu.frame.disableColor",
                      "Menu.Frame.DisableColor",
                      "black");

  mstyle.h_text =
    readDatabaseColor("menu.hilite.textColor",
                      "Menu.Hilite.TextColor",
                      "black");

  mstyle.clock_text =
    readDatabaseColor("menu.clock.textColor",
                      "Menu.Clock.TextColor",
                      "black");

  mstyle.date_text =
    readDatabaseColor("menu.date.textColor",
                      "Menu.Date.TextColor",
                      "black");

  mstyle.clock_format =
    "%I:%M:%S %p";

  if (XrmGetResource(stylerc,
                     "menu.clock.format",
                     "Menu.Clock.Format",
                     &valueType,
                     &value)) {

    mstyle.clock_format = value.addr;
  }

  mstyle.date_format =
    "%m/%d/%Y";

  if (XrmGetResource(stylerc,
                     "menu.date.format",
                     "Menu.Date.Format",
                     &valueType,
                     &value)) {

    mstyle.date_format = value.addr;
  }

  mstyle.t_justify = LeftJustify;

  if (XrmGetResource(stylerc,
                     "menu.title.justify",
                     "Menu.Title.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
        strstr(value.addr, "Right")) {

      mstyle.t_justify = RightJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      mstyle.t_justify = CenterJustify;
    }
  }

  mstyle.f_justify = LeftJustify;

  if (XrmGetResource(stylerc,
                     "menu.frame.justify",
                     "Menu.Frame.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
               strstr(value.addr, "Right")) {

      mstyle.f_justify = RightJustify;

    } else if (strstr(value.addr, "left") ||
               strstr(value.addr, "Left")) {

      mstyle.f_justify = LeftJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      mstyle.f_justify = CenterJustify;
    }
  }

  mstyle.clock_justify = CenterJustify;

  if (XrmGetResource(stylerc,
                     "menu.clock.justify",
                     "Menu.Clock.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
               strstr(value.addr, "Right")) {

      mstyle.clock_justify = RightJustify;

    } else if (strstr(value.addr, "left") ||
               strstr(value.addr, "Left")) {

      mstyle.clock_justify = LeftJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      mstyle.clock_justify = CenterJustify;
    }
  }

  mstyle.date_justify = CenterJustify;

  if (XrmGetResource(stylerc,
                     "menu.date.justify",
                     "Menu.Date.Justify",
                     &valueType,
                     &value)) {

    if (strstr(value.addr, "right") ||
               strstr(value.addr, "Right")) {

      mstyle.date_justify = RightJustify;

    } else if (strstr(value.addr, "left") ||
               strstr(value.addr, "Left")) {

      mstyle.date_justify = LeftJustify;

    } else if (strstr(value.addr, "center") ||
               strstr(value.addr, "Center")) {

      mstyle.date_justify = CenterJustify;
    }
  }

  mstyle.bullet = HbBasemenu::Triangle;

  if (XrmGetResource(stylerc,
                     "menu.bullet",
                     "Menu.Bullet",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "empty",
                     value.size)) {

      mstyle.bullet = HbBasemenu::Empty;

    } else if (!strncasecmp(value.addr,
                            "square",
                            value.size)) {

      mstyle.bullet = HbBasemenu::Square;

    } else if (!strncasecmp(value.addr,
                            "diamond",
                            value.size)) {

      mstyle.bullet = HbBasemenu::Diamond;
    }
  }

  mstyle.bullet_pos = HbBasemenu::Left;

  if (XrmGetResource(stylerc,
                     "menu.bullet.position",
                     "Menu.Bullet.Position",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "right",
                     value.size)) {

      mstyle.bullet_pos = HbBasemenu::Right;
    }
  }

  mstyle.icon = true;

  if (XrmGetResource(stylerc,
                     "menu.icon",
                     "Menu.Icon",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "false",
                     value.size) ||
        !strncasecmp(value.addr,
                     "no",
                     value.size) ||
        !strncasecmp(value.addr,
                     "off",
                     value.size)) {

      mstyle.icon = false;
    }
  }

  mstyle.icon_pos = HbBasemenu::Left;

  if (XrmGetResource(stylerc,
                     "menu.icon.position",
                     "Menu.Icon.Position",
                     &valueType,
                     &value)) {

    if (!strncasecmp(value.addr,
                     "right",
                     value.size)) {

      mstyle.icon_pos = HbBasemenu::Right;

    } else if (!strncasecmp(value.addr,
                            "left",
                            value.size)) {

      mstyle.icon_pos = HbBasemenu::Left;
    }
  }

  if (mstyle.frame.texture() ==
      HbTexture::ParentRelativeTexture) {

    mstyle.frame =
      HbTexture("solid flat",
                hackedbox,
                screen_number,
                image_control);

    mstyle.frame.setColor(
      HbColor("black",
              hackedbox,
              screen_number));
  }

  border_color =
    readDatabaseColor("borderColor",
                      "BorderColor",
                      "black");

  unsigned int uintValue;

  handle_width = 6;

  if (XrmGetResource(stylerc,
                     "handleWidth",
                     "HandleWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1 &&
      uintValue <=
        static_cast<unsigned int>(
          DisplayWidth(hackedbox->getXDisplay(),
                       screen_number) / 2) &&
      uintValue != 0) {

    handle_width = uintValue;
  }

  border_width = 1;

  if (XrmGetResource(stylerc,
                     "borderWidth",
                     "BorderWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1) {

    border_width = uintValue;
  }

  bevel_width = 3;

  if (XrmGetResource(stylerc,
                     "bevelWidth",
                     "BevelWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1 &&
      uintValue <=
        static_cast<unsigned int>(
          DisplayWidth(hackedbox->getXDisplay(),
                       screen_number) / 2) &&
      uintValue != 0) {

    bevel_width = uintValue;
  }

  frame_width = bevel_width;

  if (XrmGetResource(stylerc,
                     "frameWidth",
                     "FrameWidth",
                     &valueType,
                     &value) &&
      sscanf(value.addr, "%u", &uintValue) == 1 &&
      uintValue <=
        static_cast<unsigned int>(
          DisplayWidth(hackedbox->getXDisplay(),
                       screen_number) / 2)) {

    frame_width = uintValue;
  }

  if (XrmGetResource(stylerc,
                     "rootCommand",
                     "RootCommand",
                     &valueType,
                     &value)) {

    hbexec(value.addr,
           DisplayString(hackedbox->getXDisplay()));
  }

  if (XrmGetResource(stylerc,
                     "root.background",
                     "Root.Background",
                     &valueType,
                     &value)) {

    std::string background =
      expandTilde(value.addr);

    std::string mode = "center";

    XrmValue modeValue{};
    char *modeType = nullptr;

    if (XrmGetResource(stylerc,
                       "root.background.mode",
                       "Root.Background.Mode",
                       &modeType,
                       &modeValue)) {

      mode = modeValue.addr;
    }

    std::string command = "hbsetbg";

    if (mode == "center")
      command += " -center";

    command += " \"" + background + "\"";

    hbexec(command.c_str(),
           DisplayString(hackedbox->getXDisplay()));
  }

  if (XrmGetResource(stylerc,
                     "root.background.folder",
                     "Root.Background.Folder",
                     &valueType,
                     &value)) {

    backgroundFolder =
      expandTilde(value.addr);
  }

  if (XrmGetResource(stylerc,
                     "root.background.timer",
                     "Root.Background.Timer",
                     &valueType,
                     &value)) {

    backgroundTimer =
      atoi(value.addr);
  }

  XrmDestroyDatabase(stylerc);
  stylerc = 0;

  return true;
}


HbTexture StyleEngine::readDatabaseTexture(
  const std::string &resourceName,
  const std::string &resourceClass,
  const std::string &defaultColor) {

  HbTexture texture;

  XrmValue value;
  char *valueType;

  if (XrmGetResource(stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    texture =
      HbTexture(value.addr);

    FILE *debug =
      fopen("/tmp/hackedbox-image.log", "a");

    if (debug) {
      fprintf(debug,
              "StyleEngine texture: key=%s value=%s texture=%lu description=%s\n",
              resourceName.c_str(),
              value.addr,
              texture.texture(),
              texture.description().c_str());
      fclose(debug);
    }

  } else {

    texture.setHbTexture(
      HbTexture::Solid |
      HbTexture::Flat);
  }

  texture.setDisplay(hackedbox,
                     screen_number);

  texture.setHbImageControl(image_control);

  texture.setColor(
    readDatabaseColor(resourceName + ".color",
                      resourceClass + ".Color",
                      defaultColor));

  texture.setColorTo(
    readDatabaseColor(resourceName + ".colorTo",
                      resourceClass + ".ColorTo",
                      defaultColor));

  return texture;
}


HbColor StyleEngine::readDatabaseColor(
  const std::string &resourceName,
  const std::string &resourceClass,
  const std::string &defaultColor) {

  HbColor color;

  XrmValue value;
  char *valueType;

  if (XrmGetResource(stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    color =
      HbColor(value.addr,
              hackedbox,
              screen_number);

  } else {

    color =
      HbColor(defaultColor,
              hackedbox,
              screen_number);
  }

  return color;
}


HbFont *StyleEngine::readDatabaseFont(
  const std::string &resourceName,
  const std::string &resourceClass) {

  const char *fontName;
  XrmValue value;
  char *valueType;

  if (XrmGetResource(stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    fontName = value.addr;

  } else {

    fprintf(stderr,
            "StyleEngine::readDatabaseFont(): "
            "no font configured for '%s'\n",
            resourceName.c_str());

    return nullptr;
  }

  FILE *log =
    fopen(expandTilde("~/.hackedbox/font-debug.log").c_str(), "a");

  if (log) {
    fprintf(log,
            "StyleEngine: font request '%s' / '%s' -> %s\n",
            resourceName.c_str(),
            resourceClass.c_str(),
            fontName);
    fclose(log);
  }

  HbFont *font = new HbFont();

  if (!font->load(hackedbox->getXDisplay(),
                  screen_number,
                  fontName)) {

    fprintf(stderr,
            "StyleEngine::readDatabaseFont(): "
            "couldn't load font '%s'\n",
            fontName);

    delete font;
    return nullptr;
  }

  return font;
}