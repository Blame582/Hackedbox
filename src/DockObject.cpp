// DockObject.cpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>

#include "DockObject.hpp"

DockObject::DockObject(void)
  : x(0), y(0), width(0), height(0) {
}

DockObject::~DockObject(void) {
}

bool DockObject::buttonPress(Display *display, Window window,
                             unsigned int button, int objectX, int objectY) {
    return false;
}

void DockObject::setGeometry(int newX, int newY,
                             unsigned int newWidth,
                             unsigned int newHeight) {
    x = newX;
    y = newY;
    width = newWidth;
    height = newHeight;
}

bool DockObject::contains(int objectX, int objectY) const {
    return objectX >= x && objectX < x + static_cast<int>(width) &&
           objectY >= y && objectY < y + static_cast<int>(height);
}

int DockObject::getX(void) const {
    return x;
}

int DockObject::getY(void) const {
    return y;
}

unsigned int DockObject::getWidth(void) const {
    return width;
}

unsigned int DockObject::getHeight(void) const {
    return height;
}