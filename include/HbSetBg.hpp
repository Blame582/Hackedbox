#ifndef HBSETBG_HPP
#define HBSETBG_HPP

#include <string>

#include <X11/Xlib.h>

#include "../include/BaseDisplay.hpp"
#include "../include/Timer.hpp"

class HbSetBg :
    public BaseDisplay,
    public TimeoutHandler
{
public:
  HbSetBg(int argc,
          char **argv,
          char *display_name);

  ~HbSetBg() override;

private:
  enum class Mode {
    Center,
    Tile,
    StretchToCenter,
    StretchToEdge
  };

  void process_event(XEvent *event) override;

  bool handleSignal(int signal) override;

  void timeout() override;

  void setBackground();

  void setTimer(long milliseconds);

  Pixmap loadImage(int screen,
                   const std::string &filename,
                   int width,
                   int height);

  Pixmap createPixmap(int screen,
                      int width,
                      int height);

  void setPixmapProperty(int screen,
                         Pixmap pixmap);

  void usage(int exit_code = 0);

  std::string image_file;

  Mode mode;

  HbTimer *timer;
  long timer_interval;
};

#endif // HBSETBG_HPP
