// Basemenu.cpp for Hackedbox - an X Window manager
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
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#ifdef HAVE_XRENDER
#  include <X11/extensions/Xrender.h>
#endif

#include <Imlib2.h>

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
 * Select a 32-bit TrueColor visual with an alpha channel for menus.
 *
 * The normal ScreenInfo visual remains unchanged. This visual is used
 * only by menu windows that need ARGB rendering.
 */
static Visual *findARGBVisual(Display *display,
                              int screen,
                              int *depth)
{
#ifndef HAVE_XRENDER
  (void)display;
  (void)screen;
  (void)depth;

  return (Visual *) 0;
#else
  if (!display ||
      !depth)
    return (Visual *) 0;

  XVisualInfo visual_template;
  visual_template.screen = screen;
  visual_template.depth = 32;
  visual_template.c_class = TrueColor;

  int count = 0;

  XVisualInfo *visuals =
    XGetVisualInfo(display,
                   VisualScreenMask |
                   VisualDepthMask |
                   VisualClassMask,
                   &visual_template,
                   &count);

  if (!visuals)
    return (Visual *) 0;

  Visual *result = (Visual *) 0;

  for (int i = 0; i < count; i++) {
    XRenderPictFormat *format =
      XRenderFindVisualFormat(
        display,
        visuals[i].visual);

    if (!format)
      continue;

    if (format->type != PictTypeDirect)
      continue;

    if (!format->direct.alphaMask)
      continue;

    result = visuals[i].visual;
    *depth = visuals[i].depth;
    break;
  }

  XFree(visuals);

  return result;
#endif
}


/*
 * Allocate an RGB border color in the menu colormap.
 *
 * HbColor::pixel() belongs to the normal screen colormap and therefore
 * cannot be reused when the menu uses a different visual/colormap.
 */
static unsigned long allocateMenuColor(Display *display,
                                        Colormap colormap,
                                        const HbColor &color)
{
  if (!display ||
      colormap == None)
    return 0;

  XColor xcolor;

  xcolor.red =
    static_cast<unsigned short>(
      color.red() * 257U);

  xcolor.green =
    static_cast<unsigned short>(
      color.green() * 257U);

  xcolor.blue =
    static_cast<unsigned short>(
      color.blue() * 257U);

  xcolor.flags =
    DoRed |
    DoGreen |
    DoBlue;

  if (XAllocColor(display,
                  colormap,
                  &xcolor))
    return xcolor.pixel;

  return 0;
}


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

  if (!display ||
      !font ||
      !text ||
      len <= 0)
    return 0;

  if (font->isXft()) {
    XftFont *xft_font = font->xftfont();

    if (!xft_font)
      return 0;

    XGlyphInfo extents;

    XftTextExtentsUtf8(
      display,
      xft_font,
      reinterpret_cast<const FcChar8 *>(text),
      len,
      &extents);

    return static_cast<unsigned int>(extents.xOff);
  }

  XFontStruct *xfont = font->xfont();

  if (xfont)
    return XTextWidth(xfont, text, len);

  return 0;
}


/*
 * Return the height of a menu font using either Xft or the old X font
 * system.
 */
static unsigned int hfontHeight(HbFont *font,
                                XFontSet fontset,
                                XFontSetExtents *fontset_extents)
{
  if (!font)
    return 0;

  if (font->isXft())
    return font->height();

  if (MB_CUR_MAX > 1 &&
      fontset &&
      fontset_extents)
    return fontset_extents->max_ink_extent.height;

  return font->height();
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

  if (!display ||
      !visual ||
      !font ||
      !text ||
      len <= 0 ||
      window == None)
    return;

  /*
   * Xft / TrueType font.
   */
  if (font->isXft()) {
    XftFont *xft_font = font->xftfont();

    if (!xft_font)
      return;

    XftDraw *draw =
      XftDrawCreate(display,
                    window,
                    visual,
                    colormap);

    if (!draw)
      return;

    XRenderColor render_color;

    render_color.red =
      static_cast<unsigned short>(
        color.red() * 257U);

    render_color.green =
      static_cast<unsigned short>(
        color.green() * 257U);

    render_color.blue =
      static_cast<unsigned short>(
        color.blue() * 257U);

    render_color.alpha =
      static_cast<unsigned short>(
        color.alpha() * 257U);

    XftColor xft_color;

    if (XftColorAllocValue(display,
                           visual,
                           colormap,
                           &render_color,
                           &xft_color)) {

      XftDrawStringUtf8(
        draw,
        &xft_color,
        xft_font,
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
  XFontStruct *xfont = font->xfont();

  if (xfont) {
    HbPen pen(color, xfont);

    XDrawString(display,
                window,
                pen.gc(),
                x,
                y,
                text,
                len);
  }
}


/*
 * Draw a menu item icon from a direct image filename.
 *
 * The image is loaded with Imlib2 and rendered into the menu frame.
 */
static void drawMenuIcon(Display *display,
                         Visual *visual,
                         Colormap colormap,
                         Window window,
                         const char *filename,
                         int x,
                         int y,
                         unsigned int size)
{
  if (!display ||
      !visual ||
      window == None ||
      !filename ||
      !*filename ||
      size == 0)
    return;

  /*
   * Imlib2 uses process-global context state. Set every context
   * value required by this operation explicitly.
   */
  imlib_context_set_display(display);
  imlib_context_set_visual(visual);
  imlib_context_set_colormap(colormap);
  imlib_context_set_drawable(window);

  Imlib_Image image =
    imlib_load_image(filename);

  if (!image)
    return;

  imlib_context_set_image(image);

  const int image_width =
    imlib_image_get_width();

  const int image_height =
    imlib_image_get_height();

  if (image_width <= 0 ||
      image_height <= 0) {

    imlib_free_image();
    imlib_context_set_image(NULL);

    return;
  }

  unsigned int draw_width = size;
  unsigned int draw_height = size;

  /*
   * Use 64-bit intermediate arithmetic so a large image dimension
   * cannot overflow the scaling calculation.
   */
  if (image_width > image_height) {

    const unsigned long long scaled_height =
      (static_cast<unsigned long long>(size) *
       static_cast<unsigned long long>(image_height)) /
      static_cast<unsigned long long>(image_width);

    draw_height =
      static_cast<unsigned int>(
        scaled_height > size ?
        size :
        scaled_height);

    if (!draw_height)
      draw_height = 1;

  } else if (image_height > image_width) {

    const unsigned long long scaled_width =
      (static_cast<unsigned long long>(size) *
       static_cast<unsigned long long>(image_width)) /
      static_cast<unsigned long long>(image_height);

    draw_width =
      static_cast<unsigned int>(
        scaled_width > size ?
        size :
        scaled_width);

    if (!draw_width)
      draw_width = 1;
  }

  const int draw_x =
    x + static_cast<int>(
      (size - draw_width) / 2);

  const int draw_y =
    y + static_cast<int>(
      (size - draw_height) / 2);

  imlib_render_image_on_drawable_at_size(
    draw_x,
    draw_y,
    static_cast<int>(draw_width),
    static_cast<int>(draw_height));

  imlib_free_image();
  imlib_context_set_image(NULL);
}


HbBasemenu::HbBasemenu(HbScreen *scrn) {
  screen = scrn;
  hackedbox = screen->getHackedbox();
  image_ctrl = screen->getImageControl();
  display = hackedbox->getXDisplay();
  parent = (HbBasemenu *) 0;
  alignment = AlignDontCare;

  /*
   * Normal menu state starts with the screen visual. If an ARGB visual
   * is available, it is selected below without changing ScreenInfo.
   */
  menu_visual = screen->getVisual();
  menu_colormap = screen->getColormap();
  menu_depth = screen->getDepth();
  menu_argb = False;

#ifdef HAVE_XRENDER
  {
    int argb_depth = 0;

    Visual *argb_visual =
      findARGBVisual(
        display,
        screen->getScreenNumber(),
        &argb_depth);

    if (argb_visual) {
      Colormap argb_colormap =
        XCreateColormap(
          display,
          screen->getRootWindow(),
          argb_visual,
          AllocNone);

      if (argb_colormap != None) {
        menu_visual = argb_visual;
        menu_colormap = argb_colormap;
        menu_depth = argb_depth;
        menu_argb = True;
      }
    }
  }
#endif

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

  clock_item = -1;
  date_item = -1;

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

  menu.item_h =
    hfontHeight(style->f_font,
                style->f_fontset,
                style->f_fontset_extents) +
    menu.bevel_w;

  const unsigned int clock_h =
    hfontHeight(style->clock_font,
                style->clock_fontset,
                style->clock_fontset_extents) +
    menu.bevel_w;

  const unsigned int date_h =
    hfontHeight(style->date_font,
                style->date_fontset,
                style->date_fontset_extents) +
    menu.bevel_w;

  menu.item_h =
    max(menu.item_h,
        max(clock_h, date_h));

  menu.height = menu.title_h + screen->getBorderWidth() + menu.frame_h;

  unsigned long attrib_mask =
    CWBackPixmap | CWBackPixel | CWBorderPixel |
    CWColormap | CWOverrideRedirect | CWEventMask;

  XSetWindowAttributes attrib;

  attrib.background_pixmap = None;

  /*
   * An ARGB menu starts transparent. The actual frame/title textures
   * are installed as 32-bit ARGB pixmaps during update().
   */
  if (menu_argb) {
    attrib.background_pixel = 0;
    attrib.border_pixel =
      allocateMenuColor(
        display,
        menu_colormap,
        *screen->getBorderColor());
  } else {
    attrib.background_pixel =
      attrib.border_pixel =
        screen->getBorderColor()->pixel();
  }

  attrib.colormap = menu_colormap;
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
                  menu_depth,
                  InputOutput,
                  menu_visual,
                  attrib_mask,
                  &attrib);

  hackedbox->saveMenuSearch(menu.window, this);

  attrib_mask =
    CWBackPixmap |
    CWBackPixel |
    CWBorderPixel |
    CWEventMask;

  attrib.background_pixmap = None;

  if (menu_argb) {
    attrib.background_pixel = 0;
    attrib.border_pixel =
      allocateMenuColor(
        display,
        menu_colormap,
        *screen->getBorderColor());
  } else {
    attrib.background_pixel =
      screen->getBorderColor()->pixel();

    attrib.border_pixel =
      screen->getBorderColor()->pixel();
  }

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
                  menu_depth,
                  InputOutput,
                  menu_visual,
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
                  menu_depth,
                  InputOutput,
                  menu_visual,
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

  /*
   * The ARGB colormap is owned by this menu object.
   * The normal screen colormap belongs to ScreenInfo and must never
   * be destroyed here.
   */
  if (menu_argb &&
      menu_colormap != None) {

    XFreeColormap(
      display,
      menu_colormap);

    menu_colormap = None;
  }
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
    new HbBasemenuItem(label,
                       function,
                       exec);

  return insert(item, pos);
}


int HbBasemenu::insert(const string& label,
                       int function,
                       const string& exec,
                       const string& icon,
                       int pos) {
  HbBasemenuItem *item =
    new HbBasemenuItem(label,
                       function,
                       exec,
                       icon);

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

  if (clock_item == index)
    clock_item = -1;
  else if (clock_item > index)
    clock_item--;

  if (date_item == index)
    date_item = -1;
  else if (date_item > index)
    date_item--;

  menuitems.erase(menuitems.begin() + index);

  return menuitems.size();
}


void HbBasemenu::update(void) {
  const MenuStyle *const style =
    screen->getMenuStyle();

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
   * Clock and date rows can use fonts different from the normal
   * menu frame font. Make the common row height large enough for
   * either of them.
   */
  const unsigned int clock_h =
    hfontHeight(style->clock_font,
                style->clock_fontset,
                style->clock_fontset_extents) +
    menu.bevel_w;

  const unsigned int date_h =
    hfontHeight(style->date_font,
                style->date_fontset,
                style->date_fontset_extents) +
    menu.bevel_w;

  menu.item_h =
    max(menu.item_h,
        max(clock_h, date_h));

  /*
   * The menu title is always one line.
   */
  menu.title_h =
    title_font_h +
    (menu.bevel_w * 2);

  /*
   * Measure text using the supplied font and fontset.
   */
  auto textWidth =
    [&](HbFont *font,
        XFontSet fontset,
        const char *text,
        int len) -> unsigned int {

      if (!font || !text || len <= 0)
        return 0;

      if (font->isXft()) {

        return hfontTextWidth(
          display,
          screen->getScreenNumber(),
          font,
          text,
          len);

      } else if (MB_CUR_MAX > 1) {

        XRectangle ink, logical;

        XmbTextExtents(
          fontset,
          text,
          len,
          &ink,
          &logical);

        return logical.width;

      } else {

        return hfontTextWidth(
          display,
          screen->getScreenNumber(),
          font,
          text,
          len);
      }
    };

  if (title_vis) {

    const char *s = getLabel();
    const int l = strlen(s);

    menu.item_w =
      textWidth(style->t_font,
                style->t_fontset,
                s,
                l);

    menu.item_w +=
      menu.bevel_w * 2;

  } else {

    menu.item_w = 1;
  }

  unsigned int ii = 0;

  MenuItems::iterator it = menuitems.begin(),
    end = menuitems.end();

  for (; it != end; ++it) {

    const int index =
      it - menuitems.begin();

    const char *s = (*it)->label();
    const int l = strlen(s);

    HbFont *font = style->f_font;
    XFontSet fontset = style->f_fontset;

    if (index == clock_item) {
      font = style->clock_font;
      fontset = style->clock_fontset;
    } else if (index == date_item) {
      font = style->date_font;
      fontset = style->date_fontset;
    }

    ii =
      textWidth(font,
                fontset,
                s,
                l);

    /*
     * Keep the existing two item-height columns of horizontal
     * room when icons are enabled. One is the icon area and the
     * other is the bullet/spacing area.
     */
    if (style->icon) {
      ii += menu.item_h * 2;
    } else {
      ii += menu.item_h;
    }

    ii +=
      menu.bevel_w * 2;

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

      if (menu_argb) {

        if (texture->color().alpha() < 255) {
          menu.title_pixmap =
            image_ctrl->renderImageARGB(
              menu.width,
              menu.title_h,
              *texture);

          XSetWindowBackgroundPixmap(
            display,
            menu.title,
            menu.title_pixmap);
        } else {
          XSetWindowBackground(
            display,
            menu.title,
            allocateMenuColor(
              display,
              menu_colormap,
              texture->color()));
        }

      } else {

        XSetWindowBackground(
          display,
          menu.title,
          texture->color().pixel());
      }

    } else {

      menu.title_pixmap =
        menu_argb ?
        image_ctrl->renderImageARGB(
          menu.width,
          menu.title_h,
          *texture) :
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

    if (menu_argb) {

      if (texture->color().alpha() < 255) {

        menu.frame_pixmap =
          image_ctrl->renderImageARGB(
            menu.width,
            menu.frame_h,
            *texture);

        XSetWindowBackgroundPixmap(
          display,
          menu.frame,
          menu.frame_pixmap);

      } else {

        XSetWindowBackground(
          display,
          menu.frame,
          allocateMenuColor(
            display,
            menu_colormap,
            texture->color()));
      }

    } else {

      XSetWindowBackground(
        display,
        menu.frame,
        texture->color().pixel());
    }

  } else {

    menu.frame_pixmap =
      menu_argb ?
      image_ctrl->renderImageARGB(
        menu.width,
        menu.frame_h,
        *texture) :
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
      menu_argb ?
      image_ctrl->renderImageARGB(
        menu.item_w,
        menu.item_h,
        *texture) :
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
      menu_argb ?
      image_ctrl->renderImageARGB(
        hw,
        hw,
        *texture) :
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
          menu_visual,
          menu_colormap,
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
   * One normal menu title line.
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
}


void HbBasemenu::redrawClock(void) {
  if (!visible)
    return;

  if (!hackedbox->getClockEnabled())
    return;

  const std::string &target =
    hackedbox->getClockTarget();

  if (target != "menu" &&
      target != "both")
    return;

  const MenuStyle *const style =
    screen->getMenuStyle();

  /*
   * Keep the global Clock object's formatting independent from
   * the menu clock/date formatting.
   */
  time_t now = time((time_t *) 0);

  struct tm local_time;

  if (!localtime_r(&now, &local_time))
    return;

  if (clock_item >= 0 &&
      !style->clock_format.empty()) {

    char buffer[256];

    const size_t len =
      strftime(buffer,
               sizeof(buffer),
               style->clock_format.c_str(),
               &local_time);

    if (len)
      changeItemLabel(clock_item,
                      buffer);
  }

  if (date_item >= 0 &&
      !style->date_format.empty()) {

    char buffer[256];

    const size_t len =
      strftime(buffer,
               sizeof(buffer),
               style->date_format.c_str(),
               &local_time);

    if (len)
      changeItemLabel(date_item,
                      buffer);
  }
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

  const MenuStyle *const style =
    screen->getMenuStyle();

  /*
   * Select the font, fontset, color and justification for this
   * particular row.
   */
  HbFont *item_font =
    style->f_font;

  XFontSet item_fontset =
    style->f_fontset;

  XFontSetExtents *item_fontset_extents =
    style->f_fontset_extents;

  TextJustify item_justify =
    style->f_justify;

  const HbColor *item_text_color =
    &style->f_text;

  if (index == clock_item) {

    item_font =
      style->clock_font;

    item_fontset =
      style->clock_fontset;

    item_fontset_extents =
      style->clock_fontset_extents;

    item_justify =
      style->clock_justify;

    item_text_color =
      &style->clock_text;

  } else if (index == date_item) {

    item_font =
      style->date_font;

    item_fontset =
      style->date_fontset;

    item_fontset_extents =
      style->date_fontset_extents;

    item_justify =
      style->date_justify;

    item_text_color =
      &style->date_text;
  }

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

  /*
   * Measure text using the selected font backend.
   */
  if (text) {

    if (item_font->isXft()) {

      text_w =
        hfontTextWidth(display,
                       screen->getScreenNumber(),
                       item_font,
                       text,
                       len);

      text_y =
        item_y +
        item_font->ascent() +
        (menu.bevel_w / 2);

    } else if (MB_CUR_MAX > 1) {

      XRectangle ink, logical;

      XmbTextExtents(item_fontset,
                     text,
                     len,
                     &ink,
                     &logical);

      text_w = logical.width;

      text_y =
        item_y +
        (menu.bevel_w / 2) -
          (item_fontset_extents ?
           item_fontset_extents->max_ink_extent.y :
           0);

    } else {

      text_w =
        hfontTextWidth(display,
                       screen->getScreenNumber(),
                       item_font,
                       text,
                       len);

      text_y =
        item_y +
        item_font->ascent() +
        (menu.bevel_w / 2);
    }

    /*
     * Icon space is only reserved when icons are enabled.
     *
     * The bullet occupies the opposite edge when possible.
     */
    const unsigned int icon_space =
      style->icon ?
      menu.item_h :
      0;

    const unsigned int bullet_space =
      menu.item_h;

    switch (item_justify) {

    case LeftJustify:

      text_x =
        item_x +
        menu.bevel_w +
        ((style->icon &&
          style->icon_pos == Left) ?
         icon_space :
         0) +
        1;
      break;

    case RightJustify:

      text_x =
        item_x +
        menu.item_w -
        menu.bevel_w -
        text_w -
        ((style->icon &&
          style->icon_pos == Right) ?
         icon_space :
         ((style->bullet_pos == Right) ?
          bullet_space :
          0));
      break;

    case CenterJustify: {

      const unsigned int left_space =
        ((style->icon &&
          style->icon_pos == Left) ?
         icon_space :
         ((style->bullet_pos == Left) ?
          bullet_space :
          0));

      const unsigned int right_space =
        ((style->icon &&
          style->icon_pos == Right) ?
         icon_space :
         ((style->bullet_pos == Right) ?
          bullet_space :
          0));

      const unsigned int available =
        (menu.item_w >
         (left_space +
          right_space +
          text_w)) ?
        menu.item_w -
        left_space -
        right_space -
        text_w :
        0;

      text_x =
        item_x +
        left_space +
        (available / 2);

      break;
    }
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
   */
  const HbPen textpen(
    (index == clock_item ||
     index == date_item) ?
      *item_text_color :
      (highlight) ?
        style->h_text :
        item->isEnabled() ?
          style->f_text :
          style->d_text,
    item_font->isXft() ?
      0 :
      item_font->xfont());

  const HbPen hipen(
    style->hilite.color());

  /*
   * Submenu bullet position.
   */
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
   * MENU ICON
   *
   * Icons are controlled entirely by the menu style.
   *
   *     menu.icon: true
   *     menu.icon.position: left
   *
   * or:
   *
   *     menu.icon: false
   *
   * The menu still uses the direct filename supplied by the
   * menu entry. No desktop-file or icon-theme lookup is done.
   */
  if (style->icon &&
      item->icon() &&
      *item->icon()) {

    const unsigned int icon_size =
      menu.item_h;

    int icon_x =
      item_x +
      menu.bevel_w;

    if (style->icon_pos == Right)
      icon_x =
        item_x +
        menu.item_w -
        menu.bevel_w -
        icon_size;

    drawMenuIcon(
      display,
      menu_visual,
      menu_colormap,
      menu.frame,
      item->icon(),
      icon_x,
      item_y,
      icon_size);
  }

  /*
   * TEXT RENDERING
   *
   * Never call XDrawString/XmbDrawString with an Xft-only
   * HbFont.
   */
  if (dotext && text) {

    const HbColor &text_color =
      (index == clock_item ||
       index == date_item) ?
        *item_text_color :
        (highlight) ?
          style->h_text :
          item->isEnabled() ?
            style->f_text :
            style->d_text;

    if (item_font->isXft()) {

      drawHbFont(display,
                 screen->getScreenNumber(),
                 menu_visual,
                 menu_colormap,
                 menu.frame,
                 item_font,
                 text_color,
                 text_x,
                 text_y,
                 text,
                 len);

    } else if (MB_CUR_MAX > 1) {

      XmbDrawString(display,
                    menu.frame,
                    item_fontset,
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
  unsigned long border_pixel =
    screen->getBorderColor()->pixel();

  if (menu_argb) {
    border_pixel =
      allocateMenuColor(
        display,
        menu_colormap,
        *screen->getBorderColor());
  }

  XSetWindowBackground(
    display,
    menu.window,
    menu_argb ?
      0 :
      screen->getBorderColor()->pixel());

  XSetWindowBorder(
    display,
    menu.window,
    border_pixel);

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

  if (visible)
    drawItem(index, false, true);
}


void HbBasemenu::setClockItem(int index) {
  clock_item = index;
}


void HbBasemenu::setDateItem(int index) {
  date_item = index;
}