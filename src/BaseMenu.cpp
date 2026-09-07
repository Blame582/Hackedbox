// Basemenu.cpp for Hackedbox - an XLibre Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// look in the Authors file for credits and Copyrights
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


#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <algorithm>
#include <assert.h>

#include "Hackedbox.hpp"
#include "BaseMenu.hpp"
#include "ImageControl.hpp"
#include "GCCache.hpp"
#include "Screen.hpp"


using std::string;
using std::min;
using std::max;


static HbBasemenu *shown = (HbBasemenu *) 0;

/*
 * Return the width of text using either Xft or the old X font system.
 *
 * Xft fonts do not have an XFontStruct, so XTextWidth() must never be
 * called on an HbFont containing an Xft font.
 */
static unsigned int hfontTextWidth(Display *display,
                                    int screen,
                                    HbFont *font,
                                    const char *text,
                                    int len)
{
  (void)screen;

  if (!font || !text || len <= 0)
    return 0;

  if (font->isXft()) {
    XGlyphInfo extents;

    XftTextExtentsUtf8(display,
                       font->xftfont(),
                       reinterpret_cast<const FcChar8 *>(text),
                       len,
                       &extents);

    return extents.xOff;
  }

  if (font->xfont())
    return XTextWidth(font->xfont(), text, len);

  return 0;
}


/*
 * Draw text using whichever font backend HbFont selected.
 *
 * Xft fonts are rendered through Xft.
 * Old X fonts continue to use the existing Xlib/Xmb paths.
 */
static void drawHbFont(Display *display,
                       int screen,
                       Visual *visual,
                       Colormap colormap,
                       Window window,
                       HbFont *font,
                       const HbColor &color,
                       int x,
                       int y,
                       const char *text,
                       int len)
{
  (void)screen;

  if (!font || !text || len <= 0)
    return;

  /*
   * Xft / TrueType font.
   */
  if (font->isXft()) {
    XftDraw *draw =
      XftDrawCreate(display,
                    window,
                    visual,
                    colormap);

    if (!draw)
      return;

    XRenderColor render_color;

    render_color.red =
      static_cast<unsigned short>(color.red() * 257);

    render_color.green =
      static_cast<unsigned short>(color.green() * 257);

    render_color.blue =
      static_cast<unsigned short>(color.blue() * 257);

    render_color.alpha = 65535;

    XftColor xft_color;

    if (XftColorAllocValue(display,
                           visual,
                           colormap,
                           &render_color,
                           &xft_color)) {

      XftDrawStringUtf8(draw,
                        &xft_color,
                        font->xftfont(),
                        x,
                        y,
                        reinterpret_cast<const FcChar8 *>(text),
                        len);

      XftColorFree(display,
                   visual,
                   colormap,
                   &xft_color);
    }

    XftDrawDestroy(draw);
    return;
  }

  /*
   * Old X core font.
   */
  if (font->xfont()) {
    HbPen pen(color, font->xfont());

    XDrawString(display,
                window,
                pen.gc(),
                x,
                y,
                text,
                len);
  }
}


HbBasemenu::HbBasemenu(HbScreen *scrn) {
  screen = scrn;
  hackedbox = screen->getHackedbox();
  image_ctrl = screen->getImageControl();
  display = hackedbox->getXDisplay();
  parent = (HbBasemenu *) 0;
  alignment = AlignDontCare;

  title_vis =
    movable =
    hide_tree = True;

  shifted =
    internal_menu =
    moving =
    torn =
    visible = False;

  menu.x =
    menu.y =
    menu.x_shift =
    menu.y_shift =
    menu.x_move =
    menu.y_move = 0;

  which_sub = -1;
  which_press = -1;
  which_sbl = -1;

  menu.sublevels =
    menu.persub =
    menu.minsub = 0;

  menu.frame_pixmap =
    menu.title_pixmap =
    menu.hilite_pixmap =
    menu.sel_pixmap = None;

  menu.bevel_w = screen->getBevelWidth();

  MenuStyle *style = screen->getMenuStyle();

  if (MB_CUR_MAX > 1 && !style->t_font->isXft()) {
    menu.width = menu.title_h = menu.item_w = menu.frame_h =
      style->t_fontset_extents->max_ink_extent.height +
      (menu.bevel_w * 2);
  } else {
    menu.width = menu.title_h = menu.item_w = menu.frame_h =
      style->t_font->height() +
      (menu.bevel_w * 2);
  }

  if (MB_CUR_MAX > 1 && !style->f_font->isXft()) {
    menu.item_h =
      style->f_fontset_extents->max_ink_extent.height +
      menu.bevel_w;
  } else {
    menu.item_h =
      style->f_font->height() +
      menu.bevel_w;
  }

  menu.height = menu.title_h + screen->getBorderWidth() + menu.frame_h;

  unsigned long attrib_mask =
    CWBackPixmap | CWBackPixel | CWBorderPixel |
    CWColormap | CWOverrideRedirect | CWEventMask;

  XSetWindowAttributes attrib;

  attrib.background_pixmap = None;
  attrib.background_pixel =
    attrib.border_pixel =
      screen->getBorderColor()->pixel();

  attrib.colormap = screen->getColormap();
  attrib.override_redirect = True;

  attrib.event_mask =
    ButtonPressMask |
    ButtonReleaseMask |
    ButtonMotionMask |
    ExposureMask;

  menu.window =
    XCreateWindow(display,
                  screen->getRootWindow(),
                  menu.x,
                  menu.y,
                  menu.width,
                  menu.height,
                  screen->getBorderWidth(),
                  screen->getDepth(),
                  InputOutput,
                  screen->getVisual(),
                  attrib_mask,
                  &attrib);

  hackedbox->saveMenuSearch(menu.window, this);

  attrib_mask =
    CWBackPixmap |
    CWBackPixel |
    CWBorderPixel |
    CWEventMask;

  attrib.background_pixel =
    screen->getBorderColor()->pixel();

  attrib.event_mask |=
    EnterWindowMask |
    LeaveWindowMask;

  menu.title =
    XCreateWindow(display,
                  menu.window,
                  0,
                  0,
                  menu.width,
                  menu.height,
                  0,
                  screen->getDepth(),
                  InputOutput,
                  screen->getVisual(),
                  attrib_mask,
                  &attrib);

  hackedbox->saveMenuSearch(menu.title, this);

  attrib.event_mask |= PointerMotionMask;

  menu.frame =
    XCreateWindow(display,
                  menu.window,
                  0,
                  menu.title_h + screen->getBorderWidth(),
                  menu.width,
                  menu.frame_h,
                  0,
                  screen->getDepth(),
                  InputOutput,
                  screen->getVisual(),
                  attrib_mask,
                  &attrib);

  hackedbox->saveMenuSearch(menu.frame, this);
}


HbBasemenu::~HbBasemenu(void) {
  XUnmapWindow(display, menu.window);

  if (shown && shown->getWindowID() == getWindowID())
    shown = (HbBasemenu *) 0;

  MenuItems::const_iterator it = menuitems.begin(),
    end = menuitems.end();

  for (; it != end; ++it) {
    if (!internal_menu) {
      HbBasemenu *tmp = (*it)->submenu();

      if (tmp) {
        if (!tmp->internal_menu)
          delete tmp;
        else
          tmp->internal_hide();
      }
    }
  }

  std::for_each(menuitems.begin(),
                menuitems.end(),
                PointerAssassin());

  if (menu.title_pixmap)
    image_ctrl->removeImage(menu.title_pixmap);

  if (menu.frame_pixmap)
    image_ctrl->removeImage(menu.frame_pixmap);

  if (menu.hilite_pixmap)
    image_ctrl->removeImage(menu.hilite_pixmap);

  if (menu.sel_pixmap)
    image_ctrl->removeImage(menu.sel_pixmap);

  hackedbox->removeMenuSearch(menu.title);
  XDestroyWindow(display, menu.title);

  hackedbox->removeMenuSearch(menu.frame);
  XDestroyWindow(display, menu.frame);

  hackedbox->removeMenuSearch(menu.window);
  XDestroyWindow(display, menu.window);
}


HbBasemenuItem *HbBasemenu::find(int index) {
  if (index < 0 ||
      index >= static_cast<signed>(menuitems.size()))
    return (HbBasemenuItem *) 0;

  return menuitems[index];
}


int HbBasemenu::insert(HbBasemenuItem *item, int pos) {
  if (pos < 0) {
    menuitems.push_back(item);
  } else {
    assert(pos < static_cast<signed>(menuitems.size()));
    menuitems.insert(menuitems.begin() + pos, item);
  }

  return menuitems.size();
}


int HbBasemenu::insert(const string& label,
                     int function,
                     const string& exec,
                     int pos) {
  HbBasemenuItem *item =
    new HbBasemenuItem(label, function, exec);

  return insert(item, pos);
}


int HbBasemenu::insert(const string& label,
                     HbBasemenu *submenu,
                     int pos) {
  HbBasemenuItem *item =
    new HbBasemenuItem(label, submenu);

  submenu->parent = this;

  return insert(item, pos);
}


int HbBasemenu::remove(int index) {
  HbBasemenuItem *item = find(index);

  if (!item)
    return -1;

  if (!internal_menu) {
    HbBasemenu *tmp = item->submenu();

    if (tmp) {
      if (!tmp->internal_menu)
        delete tmp;
      else
        tmp->internal_hide();
    }
  }

  delete item;

  if (which_sub == index)
    which_sub = -1;
  else if (which_sub > index)
    which_sub--;

  menuitems.erase(menuitems.begin() + index);

  return menuitems.size();
}


void HbBasemenu::update(void) {
  const MenuStyle *const style =
    screen->getMenuStyle();

  /*
   * Menu clock:
   *
   * The clock occupies the title area of the menu as:
   *
   *     Hackedbox Menu
   *     09/07/2026
   *     10:42:31 AM
   *
   * The Clock object itself remains responsible for
   * maintaining the current date/time strings.
   */
  const bool menu_clock =
    hackedbox->getClock() &&
    hackedbox->getClockEnabled() &&
    (hackedbox->getClockTarget() == "menu" ||
     hackedbox->getClockTarget() == "both");

    FILE *log = fopen("/home/blame/.hackedbox/hackedbox.log", "a");
    if (log) {
      fprintf(log,
              "HBTRACE: menu_clock=%d enabled=%d target='%s'\n",
              menu_clock,
              hackedbox->getClockEnabled(),
              hackedbox->getClockTarget().c_str());
      fclose(log);
    }

  if (menu_clock)
    hackedbox->getClock()->update();

  unsigned int title_font_h;

  /*
   * Xmb fontset is used only when the selected HbFont is
   * an old X core font. Xft fonts have their own metrics.
   */
  if (MB_CUR_MAX > 1 && !style->f_font->isXft()) {
    menu.item_h =
      style->f_fontset_extents->max_ink_extent.height +
      menu.bevel_w;

    title_font_h =
      style->t_fontset_extents->max_ink_extent.height;
  } else {
    menu.item_h =
      style->f_font->height() +
      menu.bevel_w;

    title_font_h =
      style->t_font->height();
  }

  /*
   * Normal title height is one line.
   *
   * With the menu clock enabled the title becomes three lines:
   *
   *     title
   *     date
   *     time
   */
  menu.title_h =
    title_font_h +
    (menu.bevel_w * 2);

  if (menu_clock)
    menu.title_h =
      (title_font_h * 3) +
      (menu.bevel_w * 2);

  /*
   * Measure title text using the same backend that will
   * actually render it.
   */
  auto titleTextWidth =
    [&](const char *text, int len) -> unsigned int {

      if (!text || len <= 0)
        return 0;

      if (style->t_font->isXft()) {

        return hfontTextWidth(
          display,
          screen->getScreenNumber(),
          style->t_font,
          text,
          len);

      } else if (MB_CUR_MAX > 1) {

        XRectangle ink, logical;

        XmbTextExtents(
          style->t_fontset,
          text,
          len,
          &ink,
          &logical);

        return logical.width;

      } else {

        return hfontTextWidth(
          display,
          screen->getScreenNumber(),
          style->t_font,
          text,
          len);
      }
    };

  if (title_vis) {

    const char *s = getLabel();
    const int l = strlen(s);

    menu.item_w =
      titleTextWidth(s, l);

    menu.item_w +=
      menu.bevel_w * 2;

    /*
     * The date/time must also fit in the title.
     */
    if (menu_clock) {

      const std::string &date =
        hackedbox->getClock()->date();

      const std::string &time =
        hackedbox->getClock()->time();

      const unsigned int date_w =
        titleTextWidth(
          date.c_str(),
          date.length());

      const unsigned int time_w =
        titleTextWidth(
          time.c_str(),
          time.length());

      const unsigned int clock_w =
        max(date_w, time_w) +
        (menu.bevel_w * 2);

      if (menu.item_w < clock_w)
        menu.item_w = clock_w;
    }

  } else {

    menu.item_w = 1;
  }

  unsigned int ii = 0;

  MenuItems::iterator it = menuitems.begin(),
    end = menuitems.end();

  for (; it != end; ++it) {

    const char *s = (*it)->label();
    const int l = strlen(s);

    if (style->f_font->isXft()) {

      ii =
        hfontTextWidth(
          display,
          screen->getScreenNumber(),
          style->f_font,
          s,
          l);

    } else if (MB_CUR_MAX > 1) {

      XRectangle ink, logical;

      XmbTextExtents(
        style->f_fontset,
        s,
        l,
        &ink,
        &logical);

      ii = logical.width;

    } else {

      ii =
        hfontTextWidth(
          display,
          screen->getScreenNumber(),
          style->f_font,
          s,
          l);
    }

    ii +=
      (menu.bevel_w * 2) +
      (menu.item_h * 2);

    menu.item_w =
      ((menu.item_w < ii) ?
       ii :
       menu.item_w);
  }

  if (!menuitems.empty()) {

    menu.sublevels = 1;

    const unsigned int menu_size =
      menuitems.size();

    while (((menu.item_h *
             (menu_size + 1) /
             menu.sublevels) +
            menu.title_h +
            screen->getBorderWidth()) >
           screen->getHeight())
      menu.sublevels++;

    if (menu.sublevels < menu.minsub)
      menu.sublevels = menu.minsub;

    menu.persub =
      menu_size / menu.sublevels;

    if (menu_size % menu.sublevels)
      menu.persub++;

  } else {

    menu.sublevels = 0;
    menu.persub = 0;
  }

  menu.width =
    menu.sublevels * menu.item_w;

  if (!menu.width)
    menu.width = menu.item_w;

  menu.frame_h =
    menu.item_h * menu.persub;

  menu.height =
    (title_vis ?
     menu.title_h +
     screen->getBorderWidth() : 0) +
    menu.frame_h;

  if (!menu.frame_h)
    menu.frame_h = 1;

  if (menu.height < 1)
    menu.height = 1;

  Pixmap tmp;
  const HbTexture *texture;

  if (title_vis) {

    tmp = menu.title_pixmap;
    texture = &(style->title);

    if (texture->texture() ==
        (HbTexture::Flat | HbTexture::Solid)) {

      menu.title_pixmap = None;

      XSetWindowBackground(
        display,
        menu.title,
        texture->color().pixel());

    } else {

      menu.title_pixmap =
        image_ctrl->renderImage(
          menu.width,
          menu.title_h,
          *texture);

      XSetWindowBackgroundPixmap(
        display,
        menu.title,
        menu.title_pixmap);
    }

    if (tmp)
      image_ctrl->removeImage(tmp);

    XClearWindow(display, menu.title);
  }

  tmp = menu.frame_pixmap;
  texture = &(style->frame);

  if (texture->texture() ==
      (HbTexture::Flat | HbTexture::Solid)) {

    menu.frame_pixmap = None;

    XSetWindowBackground(
      display,
      menu.frame,
      texture->color().pixel());

  } else {

    menu.frame_pixmap =
      image_ctrl->renderImage(
        menu.width,
        menu.frame_h,
        *texture);

    XSetWindowBackgroundPixmap(
      display,
      menu.frame,
      menu.frame_pixmap);
  }

  if (tmp)
    image_ctrl->removeImage(tmp);

  tmp = menu.hilite_pixmap;
  texture = &(style->hilite);

  if (texture->texture() ==
      (HbTexture::Flat | HbTexture::Solid)) {

    menu.hilite_pixmap = None;

  } else {

    menu.hilite_pixmap =
      image_ctrl->renderImage(
        menu.item_w,
        menu.item_h,
        *texture);
  }

  if (tmp)
    image_ctrl->removeImage(tmp);

  tmp = menu.sel_pixmap;
  texture = &(style->sel);

  if (texture->texture() ==
      (HbTexture::Flat | HbTexture::Solid)) {

    menu.sel_pixmap = None;

  } else {

    const int hw = menu.item_h / 2;

    menu.sel_pixmap =
      image_ctrl->renderImage(
        hw,
        hw,
        *texture);
  }

  if (tmp)
    image_ctrl->removeImage(tmp);

  XResizeWindow(
    display,
    menu.window,
    menu.width,
    menu.height);

  if (title_vis)
    XResizeWindow(
      display,
      menu.title,
      menu.width,
      menu.title_h);

  XMoveResizeWindow(
    display,
    menu.frame,
    0,
    ((title_vis) ?
     menu.title_h +
     screen->getBorderWidth() : 0),
    menu.width,
    menu.frame_h);

  XClearWindow(display, menu.window);
  XClearWindow(display, menu.title);
  XClearWindow(display, menu.frame);

  if (title_vis && visible)
    redrawTitle();

  const int menu_size = menuitems.size();

  for (int i = 0;
       visible && i < menu_size;
       i++) {

    if (i == which_sub) {
      drawItem(i, True, 0);
      drawSubmenu(i);
    } else {
      drawItem(i, False, 0);
    }
  }

  if (parent && visible)
    parent->drawSubmenu(parent->which_sub);

  XMapSubwindows(display, menu.window);
}


void HbBasemenu::redrawTitle(void) {
  const char *title =
    (!menu.label.empty()) ?
    getLabel() :
    "Hackedbox Menu";

  const int title_len =
    strlen(title);

  const MenuStyle *const style =
    screen->getMenuStyle();

  const bool menu_clock =
    hackedbox->getClock() &&
    hackedbox->getClockEnabled() &&
    (hackedbox->getClockTarget() == "menu" ||
     hackedbox->getClockTarget() == "both");

  /*
   * Determine the title font line height.
   */
  unsigned int line_h;

  if (MB_CUR_MAX > 1 &&
      !style->f_font->isXft()) {

    line_h =
      style->t_fontset_extents->
        max_ink_extent.height;

  } else {

    line_h =
      style->t_font->height();
  }

  /*
   * Measure text using the actual rendering backend.
   */
  auto textWidth =
    [&](const char *text, int len) -> unsigned int {

      if (!text || len <= 0)
        return 0;

      if (style->t_font->isXft()) {

        return hfontTextWidth(
          display,
          screen->getScreenNumber(),
          style->t_font,
          text,
          len);

      } else if (MB_CUR_MAX > 1) {

        XRectangle ink, logical;

        XmbTextExtents(
          style->t_fontset,
          text,
          len,
          &ink,
          &logical);

        return logical.width;

      } else {

        return hfontTextWidth(
          display,
          screen->getScreenNumber(),
          style->t_font,
          text,
          len);
      }
    };

  /*
   * Draw one centered line.
   */
  auto drawCentered =
    [&](const char *text, int len, int baseline) {

      const unsigned int width =
        textWidth(text, len);

      const int dx =
        (menu.width > width) ?
        (menu.width - width) / 2 :
        0;

      if (style->t_font->isXft()) {

        drawHbFont(
          display,
          screen->getScreenNumber(),
          screen->getVisual(),
          screen->getColormap(),
          menu.title,
          style->t_font,
          style->t_text,
          dx,
          baseline,
          text,
          len);

      } else if (MB_CUR_MAX > 1) {

        HbPen pen(
          style->t_text,
          style->t_font->xfont());

        XmbDrawString(
          display,
          menu.title,
          style->t_fontset,
          pen.gc(),
          dx,
          baseline,
          text,
          len);

      } else {

        HbPen pen(
          style->t_text,
          style->t_font->xfont());

        XDrawString(
          display,
          menu.title,
          pen.gc(),
          dx,
          baseline,
          text,
          len);
      }
    };

  /*
   * First line: normal menu title.
   */
  int baseline;

  if (style->t_font->isXft()) {

    baseline =
      style->t_font->ascent() +
      menu.bevel_w;

  } else if (MB_CUR_MAX > 1) {

    baseline =
      menu.bevel_w -
      style->t_fontset_extents->
        max_ink_extent.y;

  } else {

    baseline =
      style->t_font->ascent() +
      menu.bevel_w;
  }

  drawCentered(
    title,
    title_len,
    baseline);

  /*
   * Menu clock:
   *
   *     Hackedbox Menu
   *     09/07/2026
   *     10:42:31 AM
   */
  if (menu_clock) {

    const std::string &date =
      hackedbox->getClock()->date();

    const std::string &time =
      hackedbox->getClock()->time();

    drawCentered(
      date.c_str(),
      date.length(),
      baseline + line_h);

    drawCentered(
      time.c_str(),
      time.length(),
      baseline + (line_h * 2));
  }
}


void HbBasemenu::redrawClock(void) {
  if (!visible || !title_vis)
    return;

  if (!hackedbox->getClockEnabled())
    return;

  const std::string &target =
    hackedbox->getClockTarget();

  if (target != "menu" &&
      target != "both")
    return;

  /*
   * Clear only the title window, then redraw all
   * three title lines with the current clock values.
   */
  XClearWindow(display, menu.title);

  hackedbox->getClock()->update();

  redrawTitle();
}


void HbBasemenu::show(void) {
  visible = True;

  XMapSubwindows(display, menu.window);
  XMapWindow(display, menu.window);

  XRaiseWindow(display, menu.window);

  if (!parent) {
    if (shown && (!shown->torn))
      shown->hide();

    shown = this;
  }
}


void HbBasemenu::hide(void) {
  if (!torn &&
      hide_tree &&
      parent &&
      parent->isVisible()) {

    HbBasemenu *p = parent;

    while (p->isVisible() &&
           !p->torn &&
           p->parent)
      p = p->parent;

    p->internal_hide();
  } else {
    internal_hide();
  }
}


void HbBasemenu::internal_hide(void) {
  HbBasemenuItem *tmp = find(which_sub);

  if (tmp)
    tmp->submenu()->internal_hide();

  if (parent && !torn) {
    parent->drawItem(parent->which_sub,
                     False,
                     True);

    parent->which_sub = -1;
  } else if (shown &&
             shown->menu.window == menu.window) {
    shown = (HbBasemenu *) 0;
  }

  torn = visible = False;

  which_sub = -1;
  which_press = -1;
  which_sbl = -1;

  XUnmapWindow(display, menu.window);
}


void HbBasemenu::move(int x, int y) {
  menu.x = x;
  menu.y = y;

  XMoveWindow(display,
              menu.window,
              x,
              y);

  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void HbBasemenu::drawSubmenu(int index) {
  HbBasemenuItem *item = find(which_sub);

  if (item &&
      item->submenu() &&
      !item->submenu()->isTorn() &&
      which_sub != index)
    item->submenu()->internal_hide();

  item = find(index);

  if (!item)
    return;

  HbBasemenu *submenu = item->submenu();

  if (submenu &&
      visible &&
      !submenu->isTorn() &&
      item->isEnabled()) {

    if (submenu->parent != this)
      submenu->parent = this;

    const int sbl = index / menu.persub;
    const int i = index - (sbl * menu.persub);

    int x =
      menu.x +
      ((menu.item_w * (sbl + 1)) +
       screen->getBorderWidth());

    int y;

    if (alignment == AlignTop) {
      y =
        (((shifted) ?
          menu.y_shift :
          menu.y) +
         ((title_vis) ?
          menu.title_h +
          screen->getBorderWidth() : 0) -
         ((submenu->title_vis) ?
          submenu->menu.title_h +
          screen->getBorderWidth() : 0));
    } else {
      y =
        (((shifted) ?
          menu.y_shift :
          menu.y) +
         (menu.item_h * i) +
         ((title_vis) ?
          menu.title_h +
          screen->getBorderWidth() : 0) -
         ((submenu->title_vis) ?
          submenu->menu.title_h +
          screen->getBorderWidth() : 0));
    }

    if (alignment == AlignBottom &&
        (y + submenu->menu.height) >
        ((shifted) ?
         menu.y_shift :
         menu.y) +
        menu.height) {

      y =
        (((shifted) ?
          menu.y_shift :
          menu.y) +
         menu.height -
         submenu->menu.height);
    }

    if ((x + submenu->getWidth()) >
        screen->getWidth()) {

      x =
        ((shifted) ?
         menu.x_shift :
         menu.x) -
        submenu->getWidth() -
        screen->getBorderWidth();
    }

    if (x < 0)
      x = 0;

    if ((y + submenu->getHeight()) >
        screen->getHeight()) {

      y =
        screen->getHeight() -
        submenu->getHeight() -
        (screen->getBorderWidth() * 2);
    }

    if (y < 0)
      y = 0;

    submenu->move(x, y);

    if (!moving)
      drawItem(index, True);

    if (!submenu->isVisible())
      submenu->show();

    submenu->moving = moving;
    which_sub = index;
  } else {
    which_sub = -1;
  }
}


bool HbBasemenu::hasSubmenu(int index) {
  HbBasemenuItem *item = find(index);

  if (item && item->submenu())
    return True;

  return False;
}


void HbBasemenu::drawItem(int index,
                        bool highlight,
                        bool clear,
                        int x,
                        int y,
                        unsigned int w,
                        unsigned int h) {
  HbBasemenuItem *item = find(index);

  if (!item)
    return;

  bool dotext = True;
  bool dohilite = True;
  bool dosel = True;

  const char *text = item->label();

  const int sbl =
    index / menu.persub;

  const int i =
    index - (sbl * menu.persub);

  const int item_x =
    sbl * menu.item_w;

  const int item_y =
    i * menu.item_h;

  const unsigned int half_w =
    menu.item_h / 2;

  const unsigned int quarter_w =
    menu.item_h / 4;

  const unsigned int len =
    strlen(text);

  int text_x = 0;
  int text_y = 0;

  int sel_x = 0;
  int sel_y = 0;

  int hilite_x = item_x;
  int hilite_y = item_y;

  int hoff_x = 0;
  int hoff_y = 0;

  unsigned int hilite_w =
    menu.item_w;

  unsigned int hilite_h =
    menu.item_h;

  unsigned int text_w = 0;
  unsigned int text_h = 0;

  const MenuStyle *const style =
    screen->getMenuStyle();

  if (text) {

    /*
     * Measure text using the correct font backend.
     */
    if (style->f_font->isXft()) {

      text_w =
        hfontTextWidth(display,
                       screen->getScreenNumber(),
                       style->f_font,
                       text,
                       len);

      text_y =
        item_y +
        style->f_font->ascent() +
        (menu.bevel_w / 2);

    } else if (MB_CUR_MAX > 1) {

      XRectangle ink, logical;

      XmbTextExtents(style->f_fontset,
                     text,
                     len,
                     &ink,
                     &logical);

      text_w = logical.width;

      text_y =
        item_y +
        (menu.bevel_w / 2) -
        style->f_fontset_extents->
          max_ink_extent.y;

    } else {

      text_w =
        hfontTextWidth(display,
                       screen->getScreenNumber(),
                       style->f_font,
                       text,
                       len);

      text_y =
        item_y +
        style->f_font->ascent() +
        (menu.bevel_w / 2);
    }

    switch (style->f_justify) {

    case LeftJustify:
      text_x =
        item_x +
        menu.bevel_w +
        menu.item_h +
        1;
      break;

    case RightJustify:
      text_x =
        item_x +
        menu.item_w -
        (menu.item_h +
         menu.bevel_w +
         text_w);
      break;

    case CenterJustify:
      text_x =
        item_x +
        ((menu.item_w + 1 - text_w) / 2);
      break;
    }

    text_h =
      menu.item_h -
      menu.bevel_w;
  }

  const HbPen pen(
    (highlight || item->isSelected()) ?
      style->h_text :
      style->f_text);

  /*
   * HbPen is only given an XFontStruct when using
   * the old X core font backend.
   *
   * An Xft font gets no XFontStruct.
   */
  const HbPen textpen(
    (highlight) ?
      style->h_text :
      item->isEnabled() ?
        style->f_text :
        style->d_text,
    style->f_font->isXft() ?
      0 :
      style->f_font->xfont());

  const HbPen hipen(
    style->hilite.color());

  sel_x = item_x;

  if (style->bullet_pos == Right)
    sel_x +=
      menu.item_w -
      menu.item_h -
      menu.bevel_w;

  sel_x += quarter_w;
  sel_y = item_y + quarter_w;

  if (clear) {

    XClearArea(display,
               menu.frame,
               item_x,
               item_y,
               menu.item_w,
               menu.item_h,
               False);

  } else if (!(x == y &&
               y == -1 &&
               w == h &&
               h == 0)) {

    if (!(
      max(item_x, x) <=
      min<signed>(
        item_x + menu.item_w,
        x + w) &&
      max(item_y, y) <=
      min<signed>(
        item_y + menu.item_h,
        y + h))) {

      dohilite = False;

    } else {

      hilite_x =
        max(item_x, x);

      hilite_y =
        max(item_y, y);

      hilite_w =
        min(item_x + menu.item_w,
            x + w) -
        hilite_x;

      hilite_h =
        min(item_y + menu.item_h,
            y + h) -
        hilite_y;

      hoff_x =
        hilite_x % menu.item_w;

      hoff_y =
        hilite_y % menu.item_h;
    }

    const int text_ry =
      item_y +
      (menu.bevel_w / 2);

    if (!(
      max(text_x, x) <=
      min<signed>(
        text_x + text_w,
        x + w) &&
      max(text_ry, y) <=
      min<signed>(
        text_ry + text_h,
        y + h))) {

      dotext = False;
    }

    if (!(
      max(sel_x, x) <=
      min<signed>(
        sel_x + half_w,
        x + w) &&
      max(sel_y, y) <=
      min<signed>(
        sel_y + half_w,
        y + h))) {

      dosel = False;
    }
  }

  if (dohilite &&
      highlight &&
      (menu.hilite_pixmap != ParentRelative)) {

    if (menu.hilite_pixmap) {

      XCopyArea(display,
                menu.hilite_pixmap,
                menu.frame,
                hipen.gc(),
                hoff_x,
                hoff_y,
                hilite_w,
                hilite_h,
                hilite_x,
                hilite_y);

    } else {

      XFillRectangle(display,
                     menu.frame,
                     hipen.gc(),
                     hilite_x,
                     hilite_y,
                     hilite_w,
                     hilite_h);
    }

  } else if (dosel &&
             item->isSelected() &&
             (menu.sel_pixmap != ParentRelative)) {

    if (menu.sel_pixmap) {

      XCopyArea(display,
                menu.sel_pixmap,
                menu.frame,
                hipen.gc(),
                0,
                0,
                half_w,
                half_w,
                sel_x,
                sel_y);

    } else {

      XFillRectangle(display,
                     menu.frame,
                     hipen.gc(),
                     sel_x,
                     sel_y,
                     half_w,
                     half_w);
    }
  }

  /*
   * TEXT RENDERING
   *
   * This is the critical fix.
   *
   * Never call XDrawString/XmbDrawString with an Xft-only
   * HbFont.
   */
  if (dotext && text) {

    const HbColor &text_color =
      (highlight) ?
        style->h_text :
        item->isEnabled() ?
          style->f_text :
          style->d_text;

    if (style->f_font->isXft()) {

      drawHbFont(display,
                screen->getScreenNumber(),
                screen->getVisual(),
                screen->getColormap(),
                menu.frame,
                style->f_font,
                text_color,
                text_x,
                text_y,
                text,
                len);

    } else if (MB_CUR_MAX > 1) {

      XmbDrawString(display,
                    menu.frame,
                    style->f_fontset,
                    textpen.gc(),
                    text_x,
                    text_y,
                    text,
                    len);

    } else {

      XDrawString(display,
                  menu.frame,
                  textpen.gc(),
                  text_x,
                  text_y,
                  text,
                  len);
    }
  }

  if (dosel && item->submenu()) {

    switch (style->bullet) {

    case Square:

      XDrawRectangle(display,
                     menu.frame,
                     pen.gc(),
                     sel_x,
                     sel_y,
                     half_w,
                     half_w);
      break;

    case Triangle: {
      XPoint tri[3];

      if (style->bullet_pos == Right) {

        tri[0].x =
          sel_x + quarter_w - 2;

        tri[0].y =
          sel_y + quarter_w - 2;

        tri[1].x = 4;
        tri[1].y = 2;

        tri[2].x = -4;
        tri[2].y = 2;

      } else {

        tri[0].x =
          sel_x + quarter_w - 2;

        tri[0].y =
          item_y + half_w;

        tri[1].x = 4;
        tri[1].y = 2;

        tri[2].x = 0;
        tri[2].y = -4;
      }

      XFillPolygon(display,
                   menu.frame,
                   pen.gc(),
                   tri,
                   3,
                   Convex,
                   CoordModePrevious);
      break;
    }

    case Diamond: {
      XPoint dia[4];

      dia[0].x =
        sel_x + quarter_w - 3;

      dia[0].y =
        item_y + half_w;

      dia[1].x = 3;
      dia[1].y = -3;

      dia[2].x = 3;
      dia[2].y = 3;

      dia[3].x = -3;
      dia[3].y = 3;

      XFillPolygon(display,
                   menu.frame,
                   pen.gc(),
                   dia,
                   4,
                   Convex,
                   CoordModePrevious);
      break;
    }
    }
  }
}


void HbBasemenu::setLabel(const string& label) {
  menu.label = label;
}


void HbBasemenu::setItemSelected(int index, bool sel) {
  assert(index >= 0);

  HbBasemenuItem *item = find(index);

  if (!item)
    return;

  item->setSelected(sel);

  if (visible)
    drawItem(index,
             (index == which_sub),
             True);
}


bool HbBasemenu::isItemSelected(int index) {
  assert(index >= 0);

  HbBasemenuItem *item = find(index);

  if (!item)
    return False;

  return item->isSelected();
}


void HbBasemenu::setItemEnabled(int index, bool enable) {
  assert(index >= 0);

  HbBasemenuItem *item = find(index);

  if (!item)
    return;

  item->setEnabled(enable);

  if (visible)
    drawItem(index,
             (index == which_sub),
             True);
}


bool HbBasemenu::isItemEnabled(int index) {
  assert(index >= 0);

  HbBasemenuItem *item = find(index);

  if (!item)
    return False;

  return item->isEnabled();
}


void HbBasemenu::buttonPressEvent(XButtonEvent *be) {
  if (be->window == menu.frame) {

    const int sbl =
      be->x / menu.item_w;

    const int i =
      be->y / menu.item_h;

    const int w =
      (sbl * menu.persub) + i;

    HbBasemenuItem *item = find(w);

    if (item) {

      which_press = i;
      which_sbl = sbl;

      if (item->submenu())
        drawSubmenu(w);
      else
        drawItem(w,
                 item->isEnabled(),
                 True);
    }

  } else {

    menu.x_move =
      be->x_root - menu.x;

    menu.y_move =
      be->y_root - menu.y;
  }
}


void HbBasemenu::buttonReleaseEvent(XButtonEvent *re) {
  if (re->window == menu.title) {

    if (moving) {
      moving = False;

      if (which_sub != -1)
        drawSubmenu(which_sub);
    }

    if (re->x >= 0 &&
        re->x <= static_cast<signed>(menu.width) &&
        re->y >= 0 &&
        re->y <= static_cast<signed>(menu.title_h))
      if (re->button == 3)
        hide();

  } else if (re->window == menu.frame &&
             re->x >= 0 &&
             re->x < static_cast<signed>(menu.width) &&
             re->y >= 0 &&
             re->y < static_cast<signed>(menu.frame_h)) {

    if (re->button == 3) {

      hide();

    } else {

      const int sbl =
        re->x / menu.item_w;

      const int i =
        re->y / menu.item_h;

      const int ix =
        sbl * menu.item_w;

      const int iy =
        i * menu.item_h;

      const int w =
        (sbl * menu.persub) + i;

      const int p =
        (which_sbl * menu.persub) +
        which_press;

      if (w >= 0 &&
          w < static_cast<signed>(menuitems.size())) {

        drawItem(p,
                 (p == which_sub),
                 True);

        if (p == w &&
            isItemEnabled(w)) {

          if (re->x > ix &&
              re->x < static_cast<signed>(
                ix + menu.item_w) &&
              re->y > iy &&
              re->y < static_cast<signed>(
                iy + menu.item_h)) {

            itemSelected(re->button, w);
          }
        }

      } else {

        drawItem(p,
                 False,
                 True);
      }
    }
  }
}


void HbBasemenu::motionNotifyEvent(XMotionEvent *me) {
  if (me->window == menu.title &&
      (me->state & Button1Mask)) {

    if (movable) {

      if (!moving) {

        if (parent && !torn) {

          parent->drawItem(parent->which_sub,
                           False,
                           True);

          parent->which_sub = -1;
        }

        moving = torn = True;

        if (which_sub != -1)
          drawSubmenu(which_sub);

      } else {

        menu.x =
          me->x_root - menu.x_move;

        menu.y =
          me->y_root - menu.y_move;

        XMoveWindow(display,
                     menu.window,
                     menu.x,
                     menu.y);

        if (which_sub != -1)
          drawSubmenu(which_sub);
      }
    }

  } else if (me->window == menu.frame &&
             me->x >= 0 &&
             me->x < static_cast<signed>(menu.width) &&
             me->y >= 0 &&
             me->y < static_cast<signed>(menu.frame_h)) {

    const int sbl =
      me->x / menu.item_w;

    const int i =
      me->y / menu.item_h;

    const int w =
      (sbl * menu.persub) + i;

    if ((i != which_press ||
         sbl != which_sbl) &&
        (w >= 0 &&
         w < static_cast<signed>(menuitems.size()))) {

      if (which_press != -1 &&
          which_sbl != -1) {

        const int p =
          (which_sbl * menu.persub) +
          which_press;

        HbBasemenuItem *item = find(p);

        if (!item)
          return;

        drawItem(p,
                 False,
                 True);

        if (item->submenu() &&
            item->submenu()->isVisible() &&
            !item->submenu()->isTorn()) {

          item->submenu()->internal_hide();
          which_sub = -1;
        }
      }

      which_press = i;
      which_sbl = sbl;

      HbBasemenuItem *itmp = find(w);

      if (!itmp)
        return;

      if (itmp->submenu())
        drawSubmenu(w);
      else
        drawItem(w,
                 itmp->isEnabled(),
                 True);
    }
  }
}


void HbBasemenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {

    redrawTitle();

  } else if (ee->window == menu.frame) {

    const int sbl =
      ee->x / menu.item_w;

    const int id =
      ee->y / menu.item_h;

    const int sbl_d =
      (ee->x + ee->width) /
      menu.item_w;

    int id_d =
      (ee->y + ee->height) /
      menu.item_h;

    if (id_d > menu.persub)
      id_d = menu.persub;

    MenuItems::iterator it,
      end = menuitems.end();

    for (int i = sbl;
         i <= sbl_d;
         i++) {

      it =
        menuitems.begin() +
        (id + (i * menu.persub));

      for (int ii = id;
           ii <= id_d &&
           it != end;
           ++it, ii++) {

        const int index =
          ii + (i * menu.persub);

        drawItem(index,
                 (which_sub == index),
                 False,
                 ee->x,
                 ee->y,
                 ee->width,
                 ee->height);
      }
    }
  }
}


void HbBasemenu::enterNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.frame) {

    menu.x_shift = menu.x;
    menu.y_shift = menu.y;

    if (menu.x + menu.width >
        screen->getWidth()) {

      menu.x_shift =
        screen->getWidth() -
        menu.width -
        screen->getBorderWidth();

      shifted = True;

    } else if (menu.x < 0) {

      menu.x_shift =
        -screen->getBorderWidth();

      shifted = True;
    }

    if (menu.y + menu.height >
        screen->getHeight()) {

      menu.y_shift =
        screen->getHeight() -
        menu.height -
        screen->getBorderWidth();

      shifted = True;

    } else if (menu.y +
               static_cast<signed>(
                 menu.title_h) < 0) {

      menu.y_shift =
        -screen->getBorderWidth();

      shifted = True;
    }

    if (shifted)
      XMoveWindow(display,
                  menu.window,
                  menu.x_shift,
                  menu.y_shift);

    if (which_sub != -1) {

      HbBasemenuItem *tmp =
        find(which_sub);

      if (tmp->submenu()->isVisible()) {

        const int sbl =
          ce->x / menu.item_w;

        const int i =
          ce->y / menu.item_h;

        const int w =
          (sbl * menu.persub) + i;

        if (w != which_sub &&
            !tmp->submenu()->isTorn()) {

          tmp->submenu()->internal_hide();

          drawItem(which_sub,
                   False,
                   True);

          which_sub = -1;
        }
      }
    }
  }
}


void HbBasemenu::leaveNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.frame) {

    if (which_press != -1 &&
        which_sbl != -1 &&
        menuitems.size() > 0) {

      const int p =
        (which_sbl * menu.persub) +
        which_press;

      drawItem(p,
               (p == which_sub),
               True);

      which_sbl =
        which_press = -1;
    }

    if (shifted) {

      XMoveWindow(display,
                  menu.window,
                  menu.x,
                  menu.y);

      shifted = False;

      if (which_sub != -1)
        drawSubmenu(which_sub);
    }
  }
}


void HbBasemenu::reconfigure(void) {
  XSetWindowBackground(
    display,
    menu.window,
    screen->getBorderColor()->pixel());

  XSetWindowBorder(
    display,
    menu.window,
    screen->getBorderColor()->pixel());

  XSetWindowBorderWidth(
    display,
    menu.window,
    screen->getBorderWidth());

  menu.bevel_w =
    screen->getBevelWidth();

  update();
}


void HbBasemenu::changeItemLabel(unsigned int index,
                               const string& label) {
  HbBasemenuItem *item = find(index);

  assert(item);

  item->newLabel(label);
}
