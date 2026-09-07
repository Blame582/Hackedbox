// MenuManager.cpp for Hackedbox - an XLibre Window manager
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

#include "MenuManager.hpp"

#include "Hackedbox.hpp"
#include "Screen.hpp"
#include "RootMenu.hpp"
#include "ConfigMenu.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>


static size_t stringWithin(char begin,
                           char end,
                           const char *input,
                           size_t startAt,
                           size_t length,
                           char *output) {
  bool parsing = false;

  size_t index = 0;
  size_t i = startAt;

  for (; i < length; ++i) {

    if (input[i] == begin) {
      parsing = true;

    } else if (input[i] == end) {
      break;

    } else if (parsing) {

      if (input[i] == '\\' &&
          i < length - 1)
        ++i;

      output[index++] = input[i];
    }
  }

  if (parsing)
    output[index] = '\0';
  else
    output[0] = '\0';

  return i;
}


MenuManager::MenuManager(HbScreen *screen)
  : m_screen(screen) {
}


bool MenuManager::parseFile(FILE *file,
                            Rootmenu *menu) {
  if (!file || !menu || !m_screen)
    return false;

  char line[1024];
  char keyword[1024];
  char label[1024];
  char command[1024];
  char icon[1024];

  bool done = false;

  while (!done && !feof(file)) {

    memset(line, 0, sizeof(line));
    memset(keyword, 0, sizeof(keyword));
    memset(label, 0, sizeof(label));
    memset(command, 0, sizeof(command));
    memset(icon, 0, sizeof(icon));

    if (!fgets(line, sizeof(line), file))
      continue;

    if (line[0] == '#')
      continue;

    size_t lineLength = strlen(line);

    unsigned int key = 0;

    size_t position =
      stringWithin('[',
                   ']',
                   line,
                   0,
                   lineLength,
                   keyword);

    if (keyword[0] == '\0')
      continue;

    size_t keywordLength = strlen(keyword);

    for (size_t i = 0;
         i < keywordLength;
         ++i) {

      if (keyword[i] != ' ')
        key += tolower(
          static_cast<unsigned char>(keyword[i]));
    }

    position =
      stringWithin('(',
                   ')',
                   line,
                   position,
                   lineLength,
                   label);

    position =
      stringWithin('{',
                   '}',
                   line,
                   position,
                   lineLength,
                   command);

    position =
      stringWithin('<',
                   '>',
                   line,
                   position,
                   lineLength,
                   icon);

    switch (key) {

    /*
     * [end]
     */
    case 311:
      done = true;
      break;


    /*
     * [nop]
     */
    case 333:
      menu->insert(label);
      break;


    /*
     * [exec]
     */
    case 421:

      if (!(*label && *command)) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[exec] error, "
                "no menu label and/or command defined\n");

        continue;
      }

      /*
       * Run is handled internally by Hackedbox.
       */
      if (strcmp(label, "Run") == 0) {

        menu->insert(label,
                     HbScreen::Runbox);

      } else {

        /*
         * Icon support is parsed here but is not yet passed to
         * Rootmenu::insert().
         */
        menu->insert(label,
                     HbScreen::Execute,
                     command);
      }

      break;


    /*
     * [exit]
     */
    case 442:

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[exit] error, "
                "no menu label defined\n");

        continue;
      }

      menu->insert(label,
                   HbScreen::Exit);

      break;

    /*
     * [begin]
     */
    case 517:

        if (*label)
         menu->setLabel(label);

        break;


    /*
     * [style]
     */
    case 561: {

      if (!(*label && *command)) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[style] error, "
                "no menu label and/or filename defined\n");

        continue;
      }

      std::string style =
        expandTilde(command);

      menu->insert(label,
                   HbScreen::SetStyle,
                   style.c_str());

      break;
    }


    /*
     * [config]
     *
     * The Configmenu is owned by HbScreen.  MenuManager does not
     * directly access Screen's private member.
     *
     * This entry is handled by the root menu initialization code
     * rather than by directly accessing configmenu here.
     */
    case 630:

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[config] error, "
                "no label defined\n");

        continue;
      }

      /*
       * Config menu insertion is handled by HbScreen::InitMenu().
       *
       * Do not attempt to access HbScreen::configmenu here because
       * it is private.
       */
      break;


    /*
     * [include]
     */
    case 740: {

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[include] error, "
                "no filename defined\n");

        continue;
      }

      std::string newFile =
        expandTilde(label);

      FILE *submenuFile =
        fopen(newFile.c_str(), "r");

      if (!submenuFile) {
        perror(newFile.c_str());
        continue;
      }

      struct stat buffer;

      if (fstat(fileno(submenuFile), &buffer) ||
          !S_ISREG(buffer.st_mode)) {

        fprintf(stderr,
                "MenuManager::parseFile: "
                "[include] error: "
                "'%s' is not a regular file\n",
                newFile.c_str());

        fclose(submenuFile);
        continue;
      }

      parseFile(submenuFile, menu);

      fclose(submenuFile);

      break;
    }


    /*
     * [submenu]
     */
    case 767: {

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[submenu] error, "
                "no menu label defined\n");

        continue;
      }

      Rootmenu *submenu =
        new Rootmenu(m_screen);

      if (*command)
        submenu->setLabel(command);
      else
        submenu->setLabel(label);

      parseFile(file, submenu);

      submenu->update();

      menu->insert(label,
                   submenu);

      /*
       * Rootmenu ownership is handled by HbScreen.
       * The current Screen interface does not expose addRootmenu().
       */
      break;
    }


    /*
     * [restart]
     */
    case 773:

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[restart] error, "
                "no menu label defined\n");

        continue;
      }

      if (*command) {

        menu->insert(label,
                     HbScreen::RestartOther,
                     command);

      } else {

        menu->insert(label,
                     HbScreen::Restart);
      }

      break;


    /*
     * [reconfig]
     */
    case 845:

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[reconfig] error, "
                "no menu label defined\n");

        continue;
      }

      menu->insert(label,
                   HbScreen::Reconfigure);

      break;


    /*
     * [stylesdir]
     *
     * [stylesmenu]
     */
    case 995:
    case 1113: {

      bool newMenu =
        (key == 1113);

      if (!*label ||
          (!*command && newMenu)) {

        fprintf(stderr,
                "MenuManager::parseFile: "
                "[stylesdir/stylesmenu] error, "
                "no directory defined\n");

        continue;
      }

      const char *directory =
        newMenu ? command : label;

      std::string stylesDirectory =
        expandTilde(directory);

      struct stat statBuffer;

      if (stat(stylesDirectory.c_str(),
               &statBuffer) == -1) {

        fprintf(stderr,
                "MenuManager::parseFile: "
                "[stylesdir/stylesmenu] error, "
                "%s does not exist\n",
                stylesDirectory.c_str());

        continue;
      }

      if (!S_ISDIR(statBuffer.st_mode)) {

        fprintf(stderr,
                "MenuManager::parseFile: "
                "[stylesdir/stylesmenu] error, "
                "%s is not a directory\n",
                stylesDirectory.c_str());

        continue;
      }

      Rootmenu *stylesMenu;

      if (newMenu)
        stylesMenu =
          new Rootmenu(m_screen);
      else
        stylesMenu = menu;

      DIR *directoryHandle =
        opendir(stylesDirectory.c_str());

      if (!directoryHandle)
        continue;

      struct dirent *entry;

      std::vector<std::string> entries;

      while ((entry = readdir(directoryHandle)))
        entries.push_back(entry->d_name);

      closedir(directoryHandle);

      std::sort(entries.begin(),
                entries.end());

      for (std::vector<std::string>::iterator iterator =
             entries.begin();
           iterator != entries.end();
           ++iterator) {

        const std::string &fileName =
          *iterator;

        if (fileName.empty())
          continue;

        if (fileName == "." ||
            fileName == "..")
          continue;

        if (fileName[fileName.size() - 1] == '~')
          continue;

        std::string style =
          stylesDirectory + "/" + fileName;

        if (!stat(style.c_str(),
                  &statBuffer) &&
            S_ISREG(statBuffer.st_mode)) {

          stylesMenu->insert(fileName,
                             HbScreen::SetStyle,
                             style);
        }
      }

      stylesMenu->update();

      if (newMenu) {

        stylesMenu->setLabel(label);

        menu->insert(label,
                     stylesMenu);
      }

      /*
       * Keep the menu filename behavior from the original parser.
       */
      m_screen->getHackedbox()->saveMenuFilename(
        stylesDirectory);

      break;
    }


    /*
     * [workspaces]
     */
    case 1090:

      if (!*label) {
        fprintf(stderr,
                "MenuManager::parseFile: "
                "[workspaces] error, "
                "no menu label defined\n");

        continue;
      }

      menu->insert(label,
                   m_screen->getWorkspacemenu());

      break;


    default:
      break;
    }
  }

  return menu->getCount() == 0;
}
