// Runbox.cpp for Hackedbox - an X Window Manager
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

#include "Runbox.hpp"

#include "Hackedbox.hpp"
#include "Screen.hpp"
#include "Util.hpp"

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace {

constexpr int WIDTH = 520;
constexpr int HEIGHT = 110;

constexpr int INPUT_X = 16;
constexpr int INPUT_Y = 14;
constexpr int INPUT_W = 488;
constexpr int INPUT_H = 28;

constexpr int RUN_X = 16;
constexpr int RUN_Y = 54;

constexpr int BUTTON_W = 90;
constexpr int BUTTON_H = 30;

constexpr int CANCEL_X = 116;

constexpr int HISTORY_HEIGHT = 180;


struct DialogStyle {
    std::string font;

    unsigned long background;
    unsigned long input;
    unsigned long inputText;
    unsigned long text;
    unsigned long accent;
    unsigned long border;
    unsigned long focusBorder;
    unsigned long button;
    unsigned long buttonFocus;

    int borderWidth;

    DialogStyle()
        : font("Hack-13"),
          background(0x14181C),
          input(0x0A141E),
          inputText(0xFFFFFF),
          text(0xFFFFFF),
          accent(0x00FF00),
          border(0x505458),
          focusBorder(0x00FF00),
          button(0x32363A),
          buttonFocus(0x00FF00),
          borderWidth(1) {
    }
};


DialogStyle style;


std::string trim(const std::string &value) {
    std::size_t first = 0;

    while (first < value.size() &&
           std::isspace(
               static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();

    while (last > first &&
           std::isspace(
               static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }

    return value.substr(first, last - first);
}


unsigned long parseColor(
    const std::string &value,
    unsigned long fallback) {

    if (value.empty())
        return fallback;

    std::string text = value;

    if (text[0] == '#')
        text.erase(0, 1);

    char *end = nullptr;

    const unsigned long color =
        std::strtoul(
            text.c_str(),
            &end,
            16
        );

    if (!end || *end != '\0')
        return fallback;

    return color;
}


std::string getStyleFile() {
    const char *home =
        std::getenv("HOME");

    if (!home)
        return {};

    const std::string rcFile =
        std::string(home) +
        "/.hackedbox/hackedbox.rc";

    std::ifstream input(rcFile);

    if (!input)
        return {};

    std::string line;

    while (std::getline(input, line)) {
        line = trim(line);

        if (line.empty() ||
            line[0] == '#') {
            continue;
        }

        constexpr const char *key =
            "session.styleFile:";

        if (line.rfind(key, 0) != 0)
            continue;

        return trim(
            line.substr(
                std::strlen(key)
            )
        );
    }

    return {};
}


void loadStyle() {
    style = DialogStyle();

    const std::string styleFile =
        getStyleFile();

    if (styleFile.empty())
        return;

    std::ifstream input(styleFile);

    if (!input)
        return;

    std::string line;

    while (std::getline(input, line)) {
        line = trim(line);

        if (line.empty() ||
            line[0] == '#') {
            continue;
        }

        const std::size_t colon =
            line.find(':');

        if (colon == std::string::npos)
            continue;

        const std::string key =
            trim(line.substr(0, colon));

        const std::string value =
            trim(line.substr(colon + 1));

        if (key == "window.dialog.font") {
            style.font = value;

        } else if (key == "window.dialog.bgcolor") {
            style.background =
                parseColor(
                    value,
                    style.background
                );

        } else if (key == "window.dialog.inputColor") {
            style.input =
                parseColor(
                    value,
                    style.input
                );

        } else if (key == "window.dialog.inputTextColor") {
            style.inputText =
                parseColor(
                    value,
                    style.inputText
                );

        } else if (key == "window.dialog.textColor") {
            style.text =
                parseColor(
                    value,
                    style.text
                );

        } else if (key == "window.dialog.accentColor") {
            style.accent =
                parseColor(
                    value,
                    style.accent
                );

        } else if (key == "window.dialog.borderColor") {
            style.border =
                parseColor(
                    value,
                    style.border
                );

        } else if (key == "window.dialog.borderColor.focus") {
            style.focusBorder =
                parseColor(
                    value,
                    style.focusBorder
                );

        } else if (key == "window.dialog.buttonColor") {
            style.button =
                parseColor(
                    value,
                    style.button
                );

        } else if (key == "window.dialog.buttonFocusColor") {
            style.buttonFocus =
                parseColor(
                    value,
                    style.buttonFocus
                );

        } else if (key == "window.dialog.borderWidth") {
            style.borderWidth =
                std::atoi(value.c_str());
        }
    }
}


std::string historyFile() {
    const char *home =
        std::getenv("HOME");

    if (!home)
        return {};

    return std::string(home) +
           "/.hackedbox/runboxHistory.txt";
}


void allocateColor(
    Display *display,
    Colormap colormap,
    unsigned long pixel,
    XftColor *color) {

    XRenderColor renderColor{};

    renderColor.red =
        static_cast<unsigned short>(
            ((pixel >> 16) & 0xff) * 257
        );

    renderColor.green =
        static_cast<unsigned short>(
            ((pixel >> 8) & 0xff) * 257
        );

    renderColor.blue =
        static_cast<unsigned short>(
            (pixel & 0xff) * 257
        );

    renderColor.alpha = 0xffff;

    XftColorAllocValue(
        display,
        DefaultVisual(
            display,
            DefaultScreen(display)
        ),
        colormap,
        &renderColor,
        color
    );
}

} // namespace


HbScreen *RunBox::screen = nullptr;
Display *RunBox::display = nullptr;

Window RunBox::window = None;
Window RunBox::root = None;
Window RunBox::historyWindow = None;

GC RunBox::gc = nullptr;

XftDraw *RunBox::xftDraw = nullptr;
XftFont *RunBox::font = nullptr;

XftColor RunBox::textColor{};
XftColor RunBox::inputTextColor{};
XftColor RunBox::accentColor{};

Atom RunBox::wmDelete = None;

std::string RunBox::command;
std::vector<std::string> RunBox::history;

int RunBox::historyIndex = -1;
int RunBox::historyHover = -1;

bool RunBox::historyOpen = false;
bool RunBox::runHover = false;
bool RunBox::cancelHover = false;

bool RunBox::inputFocus = false;
bool RunBox::cursorVisible = true;

std::size_t RunBox::cursorPosition = 0;

unsigned long RunBox::cursorTimer = 0;

unsigned long RunBox::backgroundColor = 0;
unsigned long RunBox::inputColor = 0;
unsigned long RunBox::borderColor = 0;
unsigned long RunBox::buttonColor = 0;
unsigned long RunBox::buttonFocusColor = 0;


bool RunBox::isOpen() {
    return window != None;
}


bool RunBox::handles(Window candidate) {
    return isOpen() &&
           candidate == window;
}


void RunBox::show(HbScreen *newScreen) {
    if (!newScreen)
        return;

    if (isOpen()) {
        XMapRaised(
            display,
            window
        );

        XSetInputFocus(
            display,
            window,
            RevertToPointerRoot,
            CurrentTime
        );
        
        XGrabKeyboard(
    		display,
    		window,
    		False,
    		GrabModeAsync,
    		GrabModeAsync,
    		CurrentTime
		);

        inputFocus = true;
        cursorVisible = true;
        cursorPosition = command.size();

        draw();

        XFlush(display);
        return;
    }

    screen = newScreen;

    display =
        screen->getHackedbox()->getXDisplay();

    root =
        screen->getRootWindow();

    loadStyle();

    backgroundColor =
        style.background;

    inputColor =
        style.input;

    borderColor =
        style.border;

    buttonColor =
        style.button;

    buttonFocusColor =
        style.buttonFocus;

    XSetWindowAttributes attributes{};

    attributes.background_pixel =
        backgroundColor;

    attributes.border_pixel =
        borderColor;

    attributes.event_mask =
        ExposureMask |
        KeyPressMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask;

    window =
        XCreateWindow(
            display,
            root,
            0,
            0,
            WIDTH,
            HEIGHT,
            style.borderWidth,
            CopyFromParent,
            InputOutput,
            CopyFromParent,
            CWBackPixel |
            CWBorderPixel |
            CWEventMask,
            &attributes
        );

    if (!window) {
        screen = nullptr;
        display = nullptr;
        return;
    }

    XStoreName(
        display,
        window,
        "Runbox"
    );

    XClassHint classHint{};

    classHint.res_name =
        const_cast<char *>("runbox");

    classHint.res_class =
        const_cast<char *>("Runbox");

    XSetClassHint(
        display,
        window,
        &classHint
    );

    wmDelete =
        XInternAtom(
            display,
            "WM_DELETE_WINDOW",
            False
        );

    XSetWMProtocols(
        display,
        window,
        &wmDelete,
        1
    );

    XSizeHints sizeHints{};

    sizeHints.flags =
        PMinSize |
        PMaxSize;

    sizeHints.min_width = WIDTH;
    sizeHints.max_width = WIDTH;
    sizeHints.min_height = HEIGHT;
    sizeHints.max_height = HEIGHT;

    XSetWMNormalHints(
        display,
        window,
        &sizeHints
    );

    gc =
        XCreateGC(
            display,
            window,
            0,
            nullptr
        );

    if (!gc) {
        close();
        return;
    }

    XSetForeground(
        display,
        gc,
        borderColor
    );

    const int screenNumber =
        DefaultScreen(display);

    const Colormap colormap =
        DefaultColormap(
            display,
            screenNumber
        );

    font =
        XftFontOpenName(
            display,
            screenNumber,
            style.font.c_str()
        );

    if (!font) {
        font =
            XftFontOpenName(
                display,
                screenNumber,
                "Hack-13"
            );
    }

    if (!font) {
        close();
        return;
    }

    xftDraw =
        XftDrawCreate(
            display,
            window,
            DefaultVisual(
                display,
                screenNumber
            ),
            colormap
        );

    if (!xftDraw) {
        close();
        return;
    }

    allocateColor(
        display,
        colormap,
        style.text,
        &textColor
    );

    allocateColor(
        display,
        colormap,
        style.inputText,
        &inputTextColor
    );

    allocateColor(
        display,
        colormap,
        style.accent,
        &accentColor
    );

    loadHistory();

    command.clear();
    historyIndex = -1;
    historyHover = -1;
    historyOpen = false;

    inputFocus = true;
    cursorVisible = true;
    cursorPosition = 0;
    cursorTimer = 0;

    /*
     * Hackedbox must manage the window first so it becomes a
     * normal framed client window.
     */
    screen->manageWindow(window);

    /*
     * manageWindow() changes the window's X11 state and event
     * handling. Re-select the events needed by RunBox afterward.
     */
    XSelectInput(
        display,
        window,
        ExposureMask |
        KeyPressMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask
    );

    /*
     * RunBox is an actual managed client, so explicitly give the
     * client input focus after Hackedbox has finished managing it.
     */
    XSetInputFocus(
        display,
        window,
        RevertToPointerRoot,
        CurrentTime
    );

    inputFocus = true;
    cursorVisible = true;
    cursorPosition = command.size();

    draw();

    XFlush(display);
}


void RunBox::draw() {
    if (!isOpen() || !gc)
        return;

    XSetForeground(
        display,
        gc,
        backgroundColor
    );

    XFillRectangle(
        display,
        window,
        gc,
        0,
        0,
        WIDTH,
        HEIGHT
    );

    drawInput();
    drawButtons();

    if (historyOpen)
        drawHistory();

    XFlush(display);
}


void RunBox::drawInput() {
    XSetForeground(
        display,
        gc,
        inputColor
    );

    XFillRectangle(
        display,
        window,
        gc,
        INPUT_X,
        INPUT_Y,
        INPUT_W,
        INPUT_H
    );

    XSetForeground(
        display,
        gc,
        inputFocus
            ? style.focusBorder
            : borderColor
    );

    XDrawRectangle(
        display,
        window,
        gc,
        INPUT_X,
        INPUT_Y,
        INPUT_W - 1,
        INPUT_H - 1
    );

    if (!xftDraw || !font)
        return;

    XftDrawStringUtf8(
        xftDraw,
        &inputTextColor,
        font,
        INPUT_X + 7,
        INPUT_Y + 19,
        reinterpret_cast<const FcChar8 *>(
            command.c_str()
        ),
        static_cast<int>(
            command.size()
        )
    );

    drawCursor();
}


void RunBox::drawCursor() {
    if (!isOpen() ||
        !inputFocus ||
        !cursorVisible ||
        !gc ||
        !font) {
        return;
    }

    const std::string before =
        command.substr(
            0,
            cursorPosition
        );

    XGlyphInfo extents{};

    XftTextExtentsUtf8(
        display,
        font,
        reinterpret_cast<const FcChar8 *>(
            before.c_str()
        ),
        static_cast<int>(
            before.size()
        ),
        &extents
    );

    int cursorX =
        INPUT_X +
        7 +
        extents.xOff;

    const int minimumX =
        INPUT_X + 5;

    const int maximumX =
        INPUT_X +
        INPUT_W -
        5;

    cursorX =
        std::clamp(
            cursorX,
            minimumX,
            maximumX
        );

    XSetForeground(
        display,
        gc,
        style.accent
    );

    XFillRectangle(
        display,
        window,
        gc,
        cursorX,
        INPUT_Y + 5,
        1,
        INPUT_H - 10
    );
}


void RunBox::drawButtons() {
    XSetForeground(
        display,
        gc,
        runHover
            ? buttonFocusColor
            : buttonColor
    );

    XFillRectangle(
        display,
        window,
        gc,
        RUN_X,
        RUN_Y,
        BUTTON_W,
        BUTTON_H
    );

    XSetForeground(
        display,
        gc,
        cancelHover
            ? buttonFocusColor
            : buttonColor
    );

    XFillRectangle(
        display,
        window,
        gc,
        CANCEL_X,
        RUN_Y,
        BUTTON_W,
        BUTTON_H
    );

    if (!xftDraw || !font)
        return;

    XftDrawStringUtf8(
        xftDraw,
        &textColor,
        font,
        RUN_X + 25,
        RUN_Y + 20,
        reinterpret_cast<const FcChar8 *>("Run"),
        3
    );

    XftDrawStringUtf8(
        xftDraw,
        &textColor,
        font,
        CANCEL_X + 17,
        RUN_Y + 20,
        reinterpret_cast<const FcChar8 *>("Cancel"),
        6
    );
}


void RunBox::drawHistory() {
    if (!historyOpen ||
        !xftDraw ||
        !font ||
        history.empty()) {
        return;
    }

    const int visible =
        std::min(
            static_cast<int>(history.size()),
            HISTORY_HEIGHT / 22
        );

    const int start =
        std::max(
            0,
            static_cast<int>(history.size()) -
            visible
        );

    XSetForeground(
        display,
        gc,
        backgroundColor
    );

    XFillRectangle(
        display,
        window,
        gc,
        INPUT_X,
        INPUT_Y + INPUT_H,
        INPUT_W,
        visible * 22
    );

    XSetForeground(
        display,
        gc,
        borderColor
    );

    XDrawRectangle(
        display,
        window,
        gc,
        INPUT_X,
        INPUT_Y + INPUT_H,
        INPUT_W - 1,
        visible * 22 - 1
    );

    for (int i = 0; i < visible; ++i) {
        const int index =
            start + i;

        const int y =
            INPUT_Y +
            INPUT_H +
            i * 22;

        if (index == historyHover) {
            XSetForeground(
                display,
                gc,
                buttonColor
            );

            XFillRectangle(
                display,
                window,
                gc,
                INPUT_X + 1,
                y + 1,
                INPUT_W - 2,
                21
            );
        }

        XftDrawStringUtf8(
            xftDraw,
            &textColor,
            font,
            INPUT_X + 7,
            y + 16,
            reinterpret_cast<const FcChar8 *>(
                history[index].c_str()
            ),
            static_cast<int>(
                history[index].size()
            )
        );
    }
}


void RunBox::expose(XExposeEvent *event) {
    if (!event ||
        !handles(event->window)) {
        return;
    }

    draw();
}


void RunBox::keyPress(XKeyEvent *event) {
    if (!event ||
        !isOpen()) {
        return;
    }

    inputFocus = true;
    cursorVisible = true;

    KeySym key = NoSymbol;
    char buffer[64]{};

    const int length =
        XLookupString(
            event,
            buffer,
            sizeof(buffer) - 1,
            &key,
            nullptr
        );

    if (key == XK_Escape) {
        close();
        return;
    }

    if (key == XK_Return ||
        key == XK_KP_Enter) {

        execute();
        return;
    }

    if (key == XK_Up) {
        historyPrevious();
        return;
    }

    if (key == XK_Down) {
        historyNext();
        return;
    }

    if (key == XK_Tab) {
        if (historyOpen)
            hideHistory();
        else
            showHistory();

        resetCursor();
        return;
    }

    if (key == XK_Left) {
        if (cursorPosition > 0)
            --cursorPosition;

        resetCursor();
        return;
    }

    if (key == XK_Right) {
        if (cursorPosition < command.size())
            ++cursorPosition;

        resetCursor();
        return;
    }

    if (key == XK_Home) {
        cursorPosition = 0;
        resetCursor();
        return;
    }

    if (key == XK_End) {
        cursorPosition = command.size();
        resetCursor();
        return;
    }

    if (key == XK_BackSpace) {
        if (cursorPosition > 0) {
            command.erase(
                cursorPosition - 1,
                1
            );

            --cursorPosition;
        }

        historyIndex = -1;
        resetCursor();
        return;
    }

    if (key == XK_Delete) {
        if (cursorPosition < command.size()) {
            command.erase(
                cursorPosition,
                1
            );
        }

        historyIndex = -1;
        resetCursor();
        return;
    }

    if (length <= 0)
        return;

    for (int i = 0; i < length; ++i) {
        const unsigned char character =
            static_cast<unsigned char>(
                buffer[i]
            );

        if (character >= 0x20 &&
            character != 0x7f) {

            command.insert(
                cursorPosition,
                1,
                static_cast<char>(character)
            );

            ++cursorPosition;
        }
    }

    historyIndex = -1;

    if (historyOpen)
        hideHistory();

    resetCursor();
}


void RunBox::buttonPress(XButtonEvent *event) {
    if (!event ||
        !isOpen()) {
        return;
    }

    const int x = event->x;
    const int y = event->y;

    if (historyOpen &&
        x >= INPUT_X &&
        x < INPUT_X + INPUT_W &&
        y >= INPUT_Y + INPUT_H) {

        const int row =
            (y - INPUT_Y - INPUT_H) / 22;

        const int visible =
            std::min(
                static_cast<int>(history.size()),
                HISTORY_HEIGHT / 22
            );

        const int start =
            std::max(
                0,
                static_cast<int>(history.size()) -
                visible
            );

        const int index =
            start + row;

        if (row >= 0 &&
            row < visible &&
            index >= 0 &&
            index < static_cast<int>(
                history.size()
            )) {

            selectHistory(index);
        }

        return;
    }

    if (x >= RUN_X &&
        x < RUN_X + BUTTON_W &&
        y >= RUN_Y &&
        y < RUN_Y + BUTTON_H) {

        execute();
        return;
    }

    if (x >= CANCEL_X &&
        x < CANCEL_X + BUTTON_W &&
        y >= RUN_Y &&
        y < RUN_Y + BUTTON_H) {

        close();
        return;
    }

    if (x >= INPUT_X &&
        x < INPUT_X + INPUT_W &&
        y >= INPUT_Y &&
        y < INPUT_Y + INPUT_H) {

        inputFocus = true;
        cursorVisible = true;

        XSetInputFocus(
            display,
            window,
            RevertToPointerRoot,
            CurrentTime
        );

        resetCursor();
        return;
    }

    inputFocus = false;
    cursorVisible = false;

    draw();
}


void RunBox::buttonRelease(XButtonEvent *event) {
    (void)event;
}


void RunBox::motionNotify(XMotionEvent *event) {
    if (!event ||
        !isOpen()) {
        return;
    }

    const int x = event->x;
    const int y = event->y;

    const bool newRunHover =
        x >= RUN_X &&
        x < RUN_X + BUTTON_W &&
        y >= RUN_Y &&
        y < RUN_Y + BUTTON_H;

    const bool newCancelHover =
        x >= CANCEL_X &&
        x < CANCEL_X + BUTTON_W &&
        y >= RUN_Y &&
        y < RUN_Y + BUTTON_H;

    int newHistoryHover = -1;

    if (historyOpen &&
        x >= INPUT_X &&
        x < INPUT_X + INPUT_W &&
        y >= INPUT_Y + INPUT_H) {

        const int row =
            (y - INPUT_Y - INPUT_H) / 22;

        const int visible =
            std::min(
                static_cast<int>(history.size()),
                HISTORY_HEIGHT / 22
            );

        const int start =
            std::max(
                0,
                static_cast<int>(history.size()) -
                visible
            );

        if (row >= 0 &&
            row < visible) {

            newHistoryHover =
                start + row;
        }
    }

    if (newRunHover != runHover ||
        newCancelHover != cancelHover ||
        newHistoryHover != historyHover) {

        runHover = newRunHover;
        cancelHover = newCancelHover;
        historyHover = newHistoryHover;

        draw();
    }
}


void RunBox::clientMessage(
    XClientMessageEvent *event) {

    if (!event ||
        !isOpen()) {
        return;
    }

    if (event->message_type ==
            XInternAtom(
                display,
                "WM_PROTOCOLS",
                False
            ) &&
        static_cast<Atom>(
            event->data.l[0]
        ) == wmDelete) {

        close();
    }
}


void RunBox::execute() {
    const std::string value =
        trim(command);

    if (value.empty())
        return;

    saveHistory(value);

    const bool success =
        hbexec(
            value,
            screen->displayString()
        );

    if (success) {
        close();
        return;
    }

    command = value;
    cursorPosition = command.size();
    historyIndex = -1;
    historyOpen = false;

    inputFocus = true;
    cursorVisible = true;

    XSetInputFocus(
        display,
        window,
        RevertToPointerRoot,
        CurrentTime
    );

    draw();
}


void RunBox::loadHistory() {
    history.clear();

    const std::string filename =
        historyFile();

    if (filename.empty())
        return;

    std::ifstream input(filename);

    if (!input)
        return;

    std::string line;

    while (std::getline(input, line)) {
        line = trim(line);

        if (!line.empty())
            history.push_back(line);
    }

    historyIndex = -1;
}


void RunBox::saveHistory(
    const std::string &value) {

    if (value.empty())
        return;

    if (history.empty() ||
        history.back() != value) {

        history.push_back(value);
    }

    const std::string filename =
        historyFile();

    if (filename.empty())
        return;

    std::ofstream output(
        filename,
        std::ios::app
    );

    if (output)
        output << value << '\n';
}


void RunBox::historyPrevious() {
    if (history.empty())
        return;

    if (historyIndex < 0) {
        historyIndex =
            static_cast<int>(
                history.size()
            ) - 1;

    } else if (historyIndex > 0) {
        --historyIndex;
    }

    command =
        history[historyIndex];

    cursorPosition = command.size();
    historyOpen = false;

    resetCursor();
}


void RunBox::historyNext() {
    if (history.empty())
        return;

    if (historyIndex < 0)
        return;

    ++historyIndex;

    if (historyIndex >=
        static_cast<int>(history.size())) {

        historyIndex = -1;
        command.clear();

    } else {
        command =
            history[historyIndex];
    }

    cursorPosition = command.size();
    historyOpen = false;

    resetCursor();
}


void RunBox::showHistory() {
    if (history.empty())
        return;

    historyOpen = true;
    historyHover = -1;

    draw();
}


void RunBox::hideHistory() {
    historyOpen = false;
    historyHover = -1;
}


void RunBox::selectHistory(int index) {
    if (index < 0 ||
        index >= static_cast<int>(
            history.size()
        )) {
        return;
    }

    command =
        history[index];

    historyIndex = index;
    cursorPosition = command.size();

    hideHistory();

    inputFocus = true;
    cursorVisible = true;

    XSetInputFocus(
        display,
        window,
        RevertToPointerRoot,
        CurrentTime
    );

    resetCursor();
}


void RunBox::resetCursor() {
    if (!isOpen())
        return;

    inputFocus = true;
    cursorVisible = true;

    XSetInputFocus(
        display,
        window,
        RevertToPointerRoot,
        CurrentTime
    );

    draw();
}


void RunBox::close() {
    if (!display)
        return;

    XUngrabKeyboard(
        display,
        CurrentTime
    );

    if (xftDraw) {
        XftDrawDestroy(xftDraw);
        xftDraw = nullptr;
    }

    if (font) {
        XftFontClose(
            display,
            font
        );

        font = nullptr;
    }

    if (gc) {
        XFreeGC(
            display,
            gc
        );

        gc = nullptr;
    }

    if (window != None) {
        XDestroyWindow(
            display,
            window
        );

        window = None;
    }

    command.clear();

    historyIndex = -1;
    historyHover = -1;

    historyOpen = false;
    runHover = false;
    cancelHover = false;

    inputFocus = false;
    cursorVisible = false;
    cursorPosition = 0;
    cursorTimer = 0;

    screen = nullptr;
    display = nullptr;
    root = None;
    historyWindow = None;
    wmDelete = None;
}
