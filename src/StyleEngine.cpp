// StyleEngine.cpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif


#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale.h>
#include <string>
#include <strings.h>
#include <unistd.h>

#include "Hackedbox.hpp"
#include "BaseMenu.hpp"
#include "ImageControl.hpp"
#include "StyleEngine.hpp"
#include "Util.hpp"

#ifndef FONT_ELEMENT_SIZE
#define FONT_ELEMENT_SIZE 50
#endif

StyleEngine::StyleEngine(Hackedbox *hb, HbImageControl *image, unsigned int screen)
  : hackedbox(hb),
    image_control(image),
    screen_number(screen),
    stylerc(0),
    backgroundFolder("~/.hackedbox/backgrounds"),
    backgroundTimer(0),
    handle_width(6),
    bevel_width(3),
    frame_width(3),
    border_width(1) {

  wstyle.fontset = 0;
  wstyle.fontset_extents = 0;
  wstyle.font = 0;

  mstyle.t_fontset = 0;
  mstyle.t_fontset_extents = 0;
  mstyle.t_font = 0;
  mstyle.f_fontset = 0;
  mstyle.f_fontset_extents = 0;
  mstyle.f_font = 0;
  mstyle.clock_fontset = 0;
  mstyle.clock_fontset_extents = 0;
  mstyle.clock_font = 0;
  mstyle.date_fontset = 0;
  mstyle.date_fontset_extents = 0;
  mstyle.date_font = 0;
}


StyleEngine::~StyleEngine() {
  if (stylerc)
    XrmDestroyDatabase(stylerc);

  if (wstyle.fontset)
    XFreeFontSet(hackedbox->getXDisplay(), wstyle.fontset);

  if (mstyle.f_fontset)
    XFreeFontSet(hackedbox->getXDisplay(), mstyle.f_fontset);

  if (mstyle.t_fontset)
    XFreeFontSet(hackedbox->getXDisplay(), mstyle.t_fontset);

  if (mstyle.clock_fontset)
    XFreeFontSet(hackedbox->getXDisplay(), mstyle.clock_fontset);

  if (mstyle.date_fontset)
    XFreeFontSet(hackedbox->getXDisplay(), mstyle.date_fontset);

  delete wstyle.font;
  delete mstyle.f_font;
  delete mstyle.t_font;
  delete mstyle.clock_font;
  delete mstyle.date_font;
}


bool StyleEngine::load(const std::string &filename) {
  if (stylerc)
    XrmDestroyDatabase(stylerc);

  stylerc =
    XrmGetFileDatabase(filename.c_str());

  if (!stylerc)
    stylerc =
      XrmGetFileDatabase(DEFAULTSTYLE);

  backgroundFolder =
    expandTilde("~/.hackedbox/backgrounds");

  backgroundTimer = 0;

  XrmValue value;
  char *valueType;

  if (wstyle.fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 wstyle.fontset);

  if (mstyle.f_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 mstyle.f_fontset);

  if (mstyle.t_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 mstyle.t_fontset);

  if (mstyle.clock_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 mstyle.clock_fontset);

  if (mstyle.date_fontset)
    XFreeFontSet(hackedbox->getXDisplay(),
                 mstyle.date_fontset);

  wstyle.fontset = 0;
  wstyle.fontset_extents = 0;

  mstyle.f_fontset = 0;
  mstyle.f_fontset_extents = 0;

  mstyle.t_fontset = 0;
  mstyle.t_fontset_extents = 0;

  mstyle.clock_fontset = 0;
  mstyle.clock_fontset_extents = 0;

  mstyle.date_fontset = 0;
  mstyle.date_fontset_extents = 0;

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

  if (MB_CUR_MAX > 1) {
    wstyle.fontset =
      readDatabaseFontSet("window.font",
                          "Window.Font");

    mstyle.t_fontset =
      readDatabaseFontSet("menu.title.font",
                          "Menu.Title.Font");

    mstyle.f_fontset =
      readDatabaseFontSet("menu.frame.font",
                          "Menu.Frame.Font");

    mstyle.clock_fontset =
      readDatabaseFontSet("menu.clock.font",
                          "Menu.Clock.Font");

    mstyle.date_fontset =
      readDatabaseFontSet("menu.date.font",
                          "Menu.Date.Font");

    mstyle.t_fontset_extents =
      XExtentsOfFontSet(mstyle.t_fontset);

    mstyle.f_fontset_extents =
      XExtentsOfFontSet(mstyle.f_fontset);

    mstyle.clock_fontset_extents =
      XExtentsOfFontSet(mstyle.clock_fontset);

    mstyle.date_fontset_extents =
      XExtentsOfFontSet(mstyle.date_fontset);

    wstyle.fontset_extents =
      XExtentsOfFontSet(wstyle.fontset);
  }

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


XFontSet StyleEngine::readDatabaseFontSet(
  const std::string &resourceName,
  const std::string &resourceClass) {

  const char *defaultFont = "fixed";

  bool loadDefault = True;

  XrmValue value;
  char *valueType;

  XFontSet fontSet = 0;

  if (XrmGetResource(stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value) &&
      (fontSet = createFontSet(value.addr))) {

    loadDefault = False;
  }

  if (loadDefault) {
    fontSet = createFontSet(defaultFont);

    if (!fontSet) {
      fprintf(stderr,
              "StyleEngine::readDatabaseFontSet(): "
              "couldn't load default font.\n");

      exit(2);
    }
  }

  return fontSet;
}


HbFont *StyleEngine::readDatabaseFont(
  const std::string &resourceName,
  const std::string &resourceClass) {

  const char *defaultFont = "fixed";

  XrmValue value;
  char *valueType;

  std::string fontName;

  if (XrmGetResource(stylerc,
                     resourceName.c_str(),
                     resourceClass.c_str(),
                     &valueType,
                     &value)) {

    fontName = value.addr;

  } else {

    fontName = defaultFont;
  }

  HbFont *font = new HbFont();

  if (!font->load(hackedbox->getXDisplay(),
                  screen_number,
                  fontName)) {

    fprintf(stderr,
            "StyleEngine::readDatabaseFont(): "
            "couldn't load font '%s'\n",
            fontName.c_str());

    delete font;

    font = new HbFont();

    if (!font->load(hackedbox->getXDisplay(),
                    screen_number,
                    defaultFont)) {

      fprintf(stderr,
              "StyleEngine::readDatabaseFont(): "
              "couldn't load default font.\n");

      delete font;
      exit(2);
    }
  }

  return font;
}


const char *StyleEngine::getFontElement(
  const char *pattern,
  char *buffer,
  int bufferSize,
  ...) {

  const char *position;
  const char *value;

  char *bufferPosition;

  va_list arguments;

  va_start(arguments, bufferSize);

  buffer[bufferSize - 1] = 0;
  buffer[bufferSize - 2] = '*';

  while ((value = va_arg(arguments, char *)) != NULL) {

    position = strcasestr(pattern, value);

    if (position) {
      strncpy(buffer,
              position + 1,
              bufferSize - 2);

      buffer[bufferSize - 1] = 0;

      bufferPosition = strchr(buffer, '-');

      if (bufferPosition)
        *bufferPosition = 0;

      va_end(arguments);

      return position;
    }
  }

  va_end(arguments);

  strncpy(buffer, "*", bufferSize - 1);
  buffer[bufferSize - 1] = 0;

  return NULL;
}


const char *StyleEngine::getFontSize(
  const char *pattern,
  int *size) {

  const char *position;
  const char *previousPosition = nullptr;

  int number = 0;

  for (position = pattern; ; ++position) {

    if (!*position) {

      if (previousPosition &&
          number > 1 &&
          number < 72) {

        *size = number;
        return previousPosition + 1;

      } else {

        *size = 16;
        return NULL;
      }

    } else if (*position == '-') {

      if (number > 1 &&
          number < 72 &&
          previousPosition) {

        *size = number;
        return previousPosition + 1;
      }

      previousPosition = position;
      number = 0;

    } else if (*position >= '0' &&
               *position <= '9' &&
               previousPosition) {

      number *= 10;
      number += *position - '0';

    } else {

      previousPosition = NULL;
      number = 0;
    }
  }
}


XFontSet StyleEngine::createFontSet(
  const std::string &fontName) {

  XFontSet fontSet;

  char **missing = 0;
  char *defaultString = const_cast<char *>("-");

  int missingCount = 0;
  int pixelSize = 0;
  int bufferSize = 0;

  char weight[FONT_ELEMENT_SIZE];
  char slant[FONT_ELEMENT_SIZE];

  fontSet =
    XCreateFontSet(hackedbox->getXDisplay(),
                   fontName.c_str(),
                   &missing,
                   &missingCount,
                   &defaultString);

  if (fontSet && !missingCount)
    return fontSet;

#ifdef HAVE_SETLOCALE
  if (!fontSet) {
    if (missingCount)
      XFreeStringList(missing);

    missing = 0;
    missingCount = 0;

    setlocale(LC_CTYPE, "C");

    fontSet =
      XCreateFontSet(hackedbox->getXDisplay(),
                     fontName.c_str(),
                     &missing,
                     &missingCount,
                     &defaultString);

    setlocale(LC_CTYPE, "");

    if (fontSet && !missingCount)
      return fontSet;
  }
#endif

  const char *nativeFontName =
    fontName.c_str();

  if (fontSet) {
    XFontStruct **fontStructs = 0;
    char **fontNames = 0;

    XFontsOfFontSet(fontSet,
                    &fontStructs,
                    &fontNames);

    if (fontNames && fontNames[0])
      nativeFontName = fontNames[0];
  }

  getFontElement(
    nativeFontName,
    weight,
    FONT_ELEMENT_SIZE,
    "-medium-",
    "-bold-",
    "-demibold-",
    "-regular-",
    NULL);

  getFontElement(
    nativeFontName,
    slant,
    FONT_ELEMENT_SIZE,
    "-r-",
    "-i-",
    "-o-",
    "-ri-",
    "-ro-",
    NULL);

  getFontSize(nativeFontName,
              &pixelSize);

  if (!strcmp(weight, "*"))
    strncpy(weight,
            "medium",
            FONT_ELEMENT_SIZE - 1);

  weight[FONT_ELEMENT_SIZE - 1] = 0;

  if (!strcmp(slant, "*"))
    strncpy(slant,
            "r",
            FONT_ELEMENT_SIZE - 1);

  slant[FONT_ELEMENT_SIZE - 1] = 0;

  if (pixelSize < 3)
    pixelSize = 3;
  else if (pixelSize > 97)
    pixelSize = 97;

  bufferSize =
    strlen(nativeFontName) +
    (FONT_ELEMENT_SIZE * 2) +
    64;

  char *pattern =
    new char[bufferSize];

  snprintf(
    pattern,
    bufferSize,
    "%s,"
    "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
    "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
    nativeFontName,
    weight,
    slant,
    pixelSize,
    pixelSize);

  if (missingCount) {
    XFreeStringList(missing);
    missing = 0;
    missingCount = 0;
  }

  if (fontSet) {
    XFreeFontSet(hackedbox->getXDisplay(),
                 fontSet);
    fontSet = 0;
  }

  fontSet =
    XCreateFontSet(hackedbox->getXDisplay(),
                   pattern,
                   &missing,
                   &missingCount,
                   &defaultString);

  if (missingCount)
    XFreeStringList(missing);

  delete [] pattern;

  return fontSet;
}