// KeyManager.hpp for Hackedbox - an X Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef KEYMANAGER_HPP
#define KEYMANAGER_HPP

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <string>
#include <vector>

class KeyManager
{
public:
    KeyManager(Display *display, Window root);

    void load(const std::string &filename);
    void reconfigure();
    void clear();

    bool handleEvent(const XKeyEvent &event);

private:
    struct KeyBinding {
        KeySym keysym;
        KeyCode keycode;
        unsigned int modifiers;
        std::string command;
    };

    Display *display;
    Window root;

    std::string filename;
    std::vector<KeyBinding> bindings;

    void parseLine(const std::string &line);

    void addBinding(const std::string &keyString,
                    const std::string &command);

    bool parseBinding(const std::string &keyString,
                      KeyBinding &binding);

    bool parseModifier(const std::string &name,
                       unsigned int &modifiers);

    KeySym parseKey(const std::string &name);

    void grabBinding(const KeyBinding &binding);
    void ungrabAll();

    void execute(const std::string &command);
};

#endif