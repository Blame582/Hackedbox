// KeyManager.cpp for Hackedbox - an X Window manager
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


#include "KeyManager.hpp"
#include "Util.hpp"

#include <fstream>
#include <sstream>

#include <cctype>


KeyManager::KeyManager(Display *display,
                       Window root)
    : display(display),
      root(root)
{
}


void KeyManager::load(const std::string &filename)
{
    this->filename = filename;

    clear();

    std::ifstream file(filename);

    if (!file)
        return;

    std::string line;

    while (std::getline(file, line))
        parseLine(line);

    reconfigure();
}


void KeyManager::clear()
{
    ungrabAll();
    bindings.clear();
}


void KeyManager::reconfigure()
{
    ungrabAll();

    for (const auto &binding : bindings)
        grabBinding(binding);

    XFlush(display);
}


void KeyManager::parseLine(const std::string &line)
{
    std::string text = line;

    size_t comment = text.find('#');

    if (comment != std::string::npos)
        text.erase(comment);

    size_t exec = text.find("[exec]");

    if (exec == std::string::npos)
        return;

    size_t keyBegin = text.find('(', exec + 6);
    size_t keyEnd = text.find(')', keyBegin);

    if (keyBegin == std::string::npos ||
        keyEnd == std::string::npos)
        return;

    size_t commandBegin = text.find('{', keyEnd);
    size_t commandEnd = text.rfind('}');

    if (commandBegin == std::string::npos ||
        commandEnd == std::string::npos ||
        commandEnd <= commandBegin)
        return;

    std::string keyString = text.substr(
        keyBegin + 1,
        keyEnd - keyBegin - 1
    );

    std::string command = text.substr(
        commandBegin + 1,
        commandEnd - commandBegin - 1
    );

    auto trim = [](std::string &value) {
        while (!value.empty() &&
               std::isspace(
                   static_cast<unsigned char>(value.front())
               ))
            value.erase(value.begin());

        while (!value.empty() &&
               std::isspace(
                   static_cast<unsigned char>(value.back())
               ))
            value.pop_back();
    };

    trim(keyString);
    trim(command);

    if (keyString.empty() || command.empty())
        return;

    addBinding(keyString, command);
}


void KeyManager::addBinding(const std::string &keyString,
                            const std::string &command)
{
    KeyBinding binding;

    if (!parseBinding(keyString, binding))
        return;

    binding.command = command;

    bindings.push_back(binding);
}


bool KeyManager::parseBinding(const std::string &keyString,
                              KeyBinding &binding)
{
    std::istringstream stream(keyString);

    std::string token;
    std::string keyName;
    unsigned int modifiers = 0;

    while (stream >> token) {
        if (parseModifier(token, modifiers))
            continue;

        keyName = token;
    }

    if (keyName.empty())
        return false;

    KeySym keysym = parseKey(keyName);

    if (keysym == NoSymbol)
        return false;

    KeyCode keycode =
        XKeysymToKeycode(display, keysym);

    if (keycode == 0)
        return false;

    binding.keysym = keysym;
    binding.keycode = keycode;
    binding.modifiers = modifiers;

    return true;
}


bool KeyManager::parseModifier(const std::string &name,
                               unsigned int &modifiers)
{
    if (name == "Shift") {
        modifiers |= ShiftMask;
        return true;
    }

    if (name == "Control" ||
        name == "Ctrl") {
        modifiers |= ControlMask;
        return true;
    }

    if (name == "Alt") {
        modifiers |= Mod1Mask;
        return true;
    }

    if (name == "Super" ||
        name == "Mod4") {
        modifiers |= Mod4Mask;
        return true;
    }

    return false;
}


KeySym KeyManager::parseKey(const std::string &name)
{
    if (name == "PrtScn" ||
        name == "PrintScreen" ||
        name == "Print")
        return XK_Print;

    if (name == "Escape" ||
        name == "Esc")
        return XK_Escape;

    if (name == "Return" ||
        name == "Enter")
        return XK_Return;

    if (name == "Space")
        return XK_space;

    return XStringToKeysym(name.c_str());
}


void KeyManager::grabBinding(const KeyBinding &binding)
{
    /*
     * X11 modifier state can contain NumLock and CapsLock.
     * Grab all four relevant combinations so the binding remains
     * active regardless of those states.
     */

    const unsigned int states[] = {
        binding.modifiers,
        binding.modifiers | LockMask,
        binding.modifiers | Mod2Mask,
        binding.modifiers | LockMask | Mod2Mask
    };

    for (unsigned int state : states) {
        XGrabKey(
            display,
            binding.keycode,
            state,
            root,
            True,
            GrabModeAsync,
            GrabModeAsync
        );
    }
}


void KeyManager::ungrabAll()
{
    XUngrabKey(
        display,
        AnyKey,
        AnyModifier,
        root
    );
}


bool KeyManager::handleEvent(const XKeyEvent &event)
{
    unsigned int state =
        event.state & ~(LockMask | Mod2Mask);

    for (const auto &binding : bindings) {
        if (event.keycode != binding.keycode)
            continue;

        if (state != binding.modifiers)
            continue;

        execute(binding.command);
        return true;
    }

    return false;
}


void KeyManager::execute(const std::string &command)
{
    hbexec(
        command,
        DisplayString(display)
    );
}