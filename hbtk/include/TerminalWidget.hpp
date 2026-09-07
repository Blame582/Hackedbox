// include/TerminalWidget.hpp

#pragma once

#include <string>
#include <vector>

#include <sys/types.h>

#include <Widget.hpp>

namespace HbTK {

class Painter;

class TerminalWidget : public Widget
{
public:
    TerminalWidget();
    ~TerminalWidget() override;

    TerminalWidget(const TerminalWidget&) = delete;
    TerminalWidget& operator=(const TerminalWidget&) = delete;

    bool start(const std::string& shell = "");

    void stop();

    bool running() const;

    void write(const std::string& text);

    void resize(int width,
                int height) override;

    void paint(Painter& painter) override;

    void keyPress(unsigned int keycode) override;

    void processOutput();

    pid_t processId() const;

private:
    bool createPty();

    void updatePtySize();

    void readPty();

    std::string shellPath() const;

private:
    int m_masterFd;
    pid_t m_pid;

    int m_columns;
    int m_rows;

    std::string m_shell;

    std::vector<std::string> m_lines;

    std::string m_currentLine;

    bool m_running;
};

} // namespace HbTK
