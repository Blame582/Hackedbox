// Timer.hpp for Hackedbox - an X Window Manager
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

#ifndef HACKEDBOX_TIMER_HPP
#define HACKEDBOX_TIMER_HPP

#include <algorithm>
#include <cstddef>
#include <queue>
#include <sys/time.h>
#include <vector>


class TimerQueueManager;


class TimeoutHandler {
public:
  virtual ~TimeoutHandler() = default;

  virtual void timeout() = 0;
};


class HbTimer {
private:
  TimerQueueManager *manager;
  TimeoutHandler *handler;

  bool timing;
  bool recur;

  timeval _start {};
  timeval _timeout {};

  HbTimer(const HbTimer &) = delete;
  HbTimer &operator=(const HbTimer &) = delete;

public:
  HbTimer(
    TimerQueueManager *manager,
    TimeoutHandler *handler
  );

  virtual ~HbTimer();

  void fireTimeout();

  bool isTiming() const {
    return timing;
  }

  bool isRecurring() const {
    return recur;
  }

  const timeval &getTimeout() const {
    return _timeout;
  }

  const timeval &getStartTime() const {
    return _start;
  }

  timeval timeRemaining(
    const timeval &time
  ) const;

  bool shouldFire(
    const timeval &time
  ) const;

  timeval endpoint() const;

  void recurring(bool recurring) {
    recur = recurring;
  }

  void setTimeout(long timeout);
  void setTimeout(const timeval &timeout);

  // TimerQueueManager acquires the timer.
  void start();

  // TimerQueueManager releases the timer.
  void stop();

  // Halt the timer without removing it from the queue.
  void halt();

  bool operator<(const HbTimer &other) const {
    return shouldFire(other.endpoint());
  }
};


template <
  class Type,
  class Sequence,
  class Compare
>
class TimerQueue :
  protected std::priority_queue<Type, Sequence, Compare> {

public:
  using Base =
    std::priority_queue<Type, Sequence, Compare>;

  TimerQueue() = default;
  ~TimerQueue() = default;

  void release(const Type &value) {
    Base::c.erase(
      std::remove(
        Base::c.begin(),
        Base::c.end(),
        value
      ),
      Base::c.end()
    );

    // Restore the heap after removing the item.
    std::make_heap(
      Base::c.begin(),
      Base::c.end(),
      Base::comp
    );
  }

  bool empty() const {
    return Base::empty();
  }

  std::size_t size() const {
    return Base::size();
  }

  void push(const Type &value) {
    Base::push(value);
  }

  void pop() {
    Base::pop();
  }

  const Type &top() const {
    return Base::top();
  }
};


struct TimerLessThan {
  bool operator()(
    const HbTimer *left,
    const HbTimer *right
  ) const {
    return *right < *left;
  }
};


using HbTimerQueue =
  TimerQueue<
    HbTimer *,
    std::vector<HbTimer *>,
    TimerLessThan
  >;


class TimerQueueManager {
public:
  virtual ~TimerQueueManager() = default;

  virtual void addTimer(HbTimer *timer) = 0;
  virtual void removeTimer(HbTimer *timer) = 0;
};


#endif // HACKEDBOX_TIMER_HPP
