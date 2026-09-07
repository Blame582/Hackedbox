// src/TerminalWidget.cpp

#include <TerminalWidget.hpp>

#include <Painter.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <pty.h>
#include <termios.h>
#include <sys/ioctl.h>

namespace HbTK {

TerminalWidget::TerminalWidget()
    : m_masterFd(-1),
      m_pid(-1),
      m_columns(80),
      m_rows(24),
      m_shell(),
      m_running(false)
{
}

TerminalWidget::~TerminalWidget()
{
    stop();
}

std::string TerminalWidget::shellPath() const
{
    if (!m_shell.empty())
        return m_shell;

    const char* shell = std::getenv("SHELL");

    if (shell && *shell)
        return shell;

    return "/bin/sh";
}

bool TerminalWidget::createPty()
{
    struct winsize size;

    std::memset(&size, 0, sizeof(size));

    size.ws_col =
        static_cast<unsigned short>(m_columns);

    size.ws_row =
        static_cast<unsigned short>(m_rows);

    size.ws_xpixel = 0;
    size.ws_ypixel = 0;

    int master = -1;
    int slave = -1;

    if (openpty(&master,
                &slave,
                nullptr,
                nullptr,
                &size) == -1)
    {
        return false;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        close(master);
        close(slave);
        return false;
    }

    if (pid == 0)
    {
        close(master);

        if (setsid() == -1)
            _exit(127);

        ioctl(slave, TIOCSCTTY, 0);

        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);

        if (slave > STDERR_FILENO)
            close(slave);

        std::string shell = shellPath();

        execl(shell.c_str(),
              shell.c_str(),
              static_cast<char*>(nullptr));

        _exit(127);
    }

    close(slave);

    m_masterFd = master;
    m_pid = pid;

    fcntl(m_masterFd,
          F_SETFL,
          fcntl(m_masterFd, F_GETFL) | O_NONBLOCK);

    return true;
}

bool TerminalWidget::start(const std::string& shell)
{
    if (m_running)
        return true;

    m_shell = shell;

    if (!createPty())
        return false;

    m_running = true;

    return true;
}

void TerminalWidget::stop()
{
    if (m_masterFd != -1)
    {
        close(m_masterFd);
        m_masterFd = -1;
    }

    if (m_pid > 0)
    {
        kill(m_pid, SIGHUP);
        m_pid = -1;
    }

    m_running = false;
}

bool TerminalWidget::running() const
{
    return m_running;
}

pid_t TerminalWidget::processId() const
{
    return m_pid;
}

void TerminalWidget::write(const std::string& text)
{
    if (!m_running || m_masterFd == -1)
        return;

    const char* data = text.data();
    std::size_t remaining = text.size();

    while (remaining)
    {
        ssize_t written =
            ::write(m_masterFd,
                    data,
                    remaining);

        if (written <= 0)
            break;

        data += written;
        remaining -=
            static_cast<std::size_t>(written);
    }
}

void TerminalWidget::resize(int width,
                            int height)
{
    Widget::resize(width, height);

    /*
     * These will eventually be calculated from
     * the terminal font metrics.
     *
     * For now, keep the terminal grid simple.
     */

    m_columns = width / 8;

    m_rows = height / 16;

    if (m_columns < 1)
        m_columns = 1;

    if (m_rows < 1)
        m_rows = 1;

    updatePtySize();
}

void TerminalWidget::updatePtySize()
{
    if (m_masterFd == -1)
        return;

    struct winsize size;

    std::memset(&size, 0, sizeof(size));

    size.ws_col =
        static_cast<unsigned short>(m_columns);

    size.ws_row =
        static_cast<unsigned short>(m_rows);

    size.ws_xpixel =
        static_cast<unsigned short>(width());

    size.ws_ypixel =
        static_cast<unsigned short>(height());

    ioctl(m_masterFd,
          TIOCSWINSZ,
          &size);
}

void TerminalWidget::readPty()
{
    if (!m_running || m_masterFd == -1)
        return;

    char buffer[4096];

    for (;;)
    {
        ssize_t count =
            ::read(m_masterFd,
                   buffer,
                   sizeof(buffer));

        if (count <= 0)
        {
            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                break;
            }

            m_running = false;
            break;
        }

        for (ssize_t i = 0; i < count; ++i)
        {
            char character = buffer[i];

            if (character == '\n')
            {
                m_lines.push_back(
                    m_currentLine);

                m_currentLine.clear();
            }
            else if (character == '\r')
            {
                m_currentLine.clear();
            }
            else if (character == '\b')
            {
                if (!m_currentLine.empty())
                    m_currentLine.pop_back();
            }
            else if (character >= 32)
            {
                m_currentLine += character;
            }
        }
    }
}

void TerminalWidget::processOutput()
{
    readPty();
}

void TerminalWidget::paint(Painter& painter)
{
    /*
     * Temporary renderer.
     *
     * The next terminal layer will replace this
     * with a proper terminal screen/parser handling
     * ANSI escape sequences, cursor state, colors,
     * scrolling and selection.
     */

    painter.fillRect(
        0,
        0,
        static_cast<unsigned int>(width()),
        static_cast<unsigned int>(height()),
        Colors(0, 0, 0));

    int y = 16;

    for (const auto& line : m_lines)
    {
        painter.drawText(
            4,
            y,
            line.c_str(),
            Colors(220, 220, 220));

        y += 16;

        if (y >= height())
            break;
    }

    if (y < height())
    {
        painter.drawText(
            4,
            y,
            m_currentLine.c_str(),
            Colors(220, 220, 220));
    }
}

void TerminalWidget::keyPress(unsigned int keycode)
{
    if (!m_running)
        return;

    /*
     * This will be connected to the X11 KeySym
     * translation layer in HbTK::Application.
     *
     * For now this accepts ASCII keycodes.
     */

    if (keycode >= 32 &&
        keycode <= 126)
    {
        char character =
            static_cast<char>(keycode);

        write(std::string(1, character));
    }
    else if (keycode == 13)
    {
        write("\n");
    }
    else if (keycode == 8)
    {
        write("\b");
    }
}

} // namespace HbTK
