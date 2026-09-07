// Timer.cpp for Hackedbox - an XLibre Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// Look in the Authors file for credits and copyrights.
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
#endif // HAVE_CONFIG_H

#include "Timer.hpp"
#include "Util.hpp"


HbTimer::HbTimer(
  TimerQueueManager *manager,
  TimeoutHandler *handler
) {
  this->manager = manager;
  this->handler = handler;

  recur = false;
  timing = false;
}


HbTimer::~HbTimer() {
  if (timing)
    stop();
}


void HbTimer::setTimeout(long timeout) {
  _timeout.tv_sec = timeout / 1000;
  _timeout.tv_usec = timeout % 1000;
  _timeout.tv_usec *= 1000;
}


void HbTimer::setTimeout(const timeval &timeout) {
  _timeout.tv_sec = timeout.tv_sec;
  _timeout.tv_usec = timeout.tv_usec;
}


void HbTimer::start() {
  gettimeofday(&_start, nullptr);

  if (!timing) {
    timing = true;
    manager->addTimer(this);
  }
}


void HbTimer::stop() {
  timing = false;

  manager->removeTimer(this);
}


void HbTimer::halt() {
  timing = false;
}


void HbTimer::fireTimeout() {
  if (handler)
    handler->timeout();
}


timeval HbTimer::timeRemaining(
  const timeval &time
) const {
  timeval result = endpoint();

  result.tv_sec -= time.tv_sec;
  result.tv_usec -= time.tv_usec;

  return normalizeTimeval(result);
}


timeval HbTimer::endpoint() const {
  timeval result {};

  result.tv_sec =
    _start.tv_sec +
    _timeout.tv_sec;

  result.tv_usec =
    _start.tv_usec +
    _timeout.tv_usec;

  return normalizeTimeval(result);
}


bool HbTimer::shouldFire(
  const timeval &time
) const {
  const timeval end = endpoint();

  return !(
    (time.tv_sec < end.tv_sec) ||
    (
      time.tv_sec == end.tv_sec &&
      time.tv_usec < end.tv_usec
    )
  );
}
