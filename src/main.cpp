// main.cpp for Hackedbox - an X Window manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// See the AUTHORS file for original Blackbox contributors.
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

#include "../version.h"

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Hackedbox.hpp"

using std::string;


static void showHelp(int exitval)
{
  printf(
    "Hackedbox %s : (c) 2026 Kevin Day\n"
    "\t\t\t See the AUTHORS file for original Blackbox contributors.\n\n"
    "  -display <string>\t\tuse display connection.\n"
    "  -rc <string>\t\t\tuse alternate resource file.\n"
    "  -version\t\t\tdisplay version and exit.\n"
    "  -help\t\t\tdisplay this help text and exit.\n\n",
    __hackedbox_version);

  const char *debug_status =
#ifdef DEBUG
    "yes";
#else
    "no";
#endif

  const char *shape_status =
#ifdef SHAPE
    "yes";
#else
    "no";
#endif

  printf(
    "Compile time options:\n"
    " Debugging:\t\t\t%s\n"
    " Shape:\t\t\t%s\n",
    debug_status,
    shape_status);

  ::exit(exitval);
}

int main(int argc, char **argv)
{
  char *session_display = nullptr;
  char *rc_file = nullptr;

  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-rc")) {
      if (++i >= argc) {
        fprintf(stderr, "error: '-rc' requires an argument\n");
        ::exit(1);
      }

      rc_file = argv[i];

    } else if (!strcmp(argv[i], "-display")) {
      if (++i >= argc) {
        fprintf(stderr, "error: '-display' requires an argument\n");
        ::exit(1);
      }

      session_display = argv[i];

      string dtmp = "DISPLAY=";
      dtmp += session_display;

      if (putenv(const_cast<char *>(dtmp.c_str()))) {
        fprintf(stderr,
                "warning: couldn't set environment variable 'DISPLAY'\n");
        perror("putenv()");
      }

    } else if (!strcmp(argv[i], "-version")) {
      printf(
        "Hackedbox %s : (c) 2026 Kevin Day\n"
        "\t\t\t See the AUTHORS file for original Blackbox contributors.\n",
        __hackedbox_version);

      ::exit(0);

    } else if (!strcmp(argv[i], "-help")) {
      showHelp(0);

    } else {
      showHelp(-1);
    }
  }

#ifdef __EMX__
  _chdir2(getenv("X11ROOT"));
#endif // __EMX__

  Hackedbox hackedbox(argv, session_display, rc_file);

  hackedbox.eventLoop();

  return 0;
}
