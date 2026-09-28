// Runbox.hpp for Hackedbox - an X Window Manager
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

#ifndef RUNBOX_HPP
#define RUNBOX_HPP

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <cstddef>
#include <string>
#include <vector>

class HbScreen;

class RunBox {
public:
    static void show(HbScreen *screen);

    static bool handles(Window window);

    static void expose(XExposeEvent *event);
    static void keyPress(XKeyEvent *event);
    static void buttonPress(XButtonEvent *event);
    static void buttonRelease(XButtonEvent *event);
    static void motionNotify(XMotionEvent *event);
    static void clientMessage(XClientMessageEvent *event);

    static void close();

private:
    RunBox() = delete;

    static void execute();
    static void draw();

    static void drawInput();
    static void drawButtons();
    static void drawHistory();
    static void drawCursor();

    static void loadHistory();
    static void saveHistory(const std::string &command);

    static void historyPrevious();
    static void historyNext();

    static void showHistory();
    static void hideHistory();
    static void selectHistory(int index);

    static void resetCursor();

    static bool isOpen();

    static HbScreen *screen;
    static Display *display;

    static Window window;
    static Window root;
    static Window historyWindow;

    static GC gc;

    static XftDraw *xftDraw;
    static XftFont *font;

    static XftColor textColor;
    static XftColor inputTextColor;
    static XftColor accentColor;

    static Atom wmDelete;

    static std::string command;
    static std::vector<std::string> history;

    static int historyIndex;
    static int historyHover;

    static bool historyOpen;
    static bool runHover;
    static bool cancelHover;

    static bool inputFocus;
    static bool cursorVisible;

    static std::size_t cursorPosition;

    static unsigned long cursorTimer;

    static unsigned long backgroundColor;
    static unsigned long inputColor;
    static unsigned long borderColor;
    static unsigned long buttonColor;
    static unsigned long buttonFocusColor;
};

#endif // RUNBOX_HPP