#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <unistd.h>
#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

static const unsigned int WIDTH  = 420;
static const unsigned int HEIGHT = 54;

static void draw_dialog(
    Display* display,
    Window window,
    GC gc,
    unsigned int width,
    const std::string& password)
{
    XClearWindow(display, window);

    const char* label = "Password:";

    XDrawString(
        display,
        window,
        gc,
        16,
        34,
        label,
        std::strlen(label)
    );

    std::string masked(password.size(), '*');

    XDrawString(
        display,
        window,
        gc,
        90,
        34,
        masked.c_str(),
        static_cast<int>(masked.size())
    );

    XDrawString(
        display,
        window,
        gc,
        width - 32,
        34,
        ">",
        1
    );

    XFlush(display);
}

static bool authenticate(
    const std::string& password,
    const std::vector<std::string>& command)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
        return false;

    pid_t pid = fork();

    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        close(pipefd[1]);

        if (dup2(pipefd[0], STDIN_FILENO) == -1)
            _exit(127);

        close(pipefd[0]);

        std::vector<char*> args;

        args.push_back(const_cast<char*>("sudo"));
        args.push_back(const_cast<char*>("-S"));
        args.push_back(const_cast<char*>("-p"));
        args.push_back(const_cast<char*>(""));

        for (const auto& arg : command)
            args.push_back(const_cast<char*>(arg.c_str()));

        args.push_back(nullptr);

        execvp("sudo", args.data());
        _exit(127);
    }

    close(pipefd[0]);

    std::string input = password;
    input += '\n';

    size_t offset = 0;

    while (offset < input.size()) {
        ssize_t n = write(
            pipefd[1],
            input.data() + offset,
            input.size() - offset
        );

        if (n <= 0)
            break;

        offset += static_cast<size_t>(n);
    }

    close(pipefd[1]);

    std::fill(input.begin(), input.end(), '\0');

    int status = 0;

    if (waitpid(pid, &status, 0) == -1)
        return false;

    return WIFEXITED(status) &&
           WEXITSTATUS(status) == 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(
            stderr,
            "usage: hbsudo command [args...]\n"
        );
        return 1;
    }

    Display* display = XOpenDisplay(nullptr);

    if (!display) {
        std::fprintf(
            stderr,
            "hbsudo: cannot open display\n"
        );
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    /*
     * Find the currently active application window.
     * Hackedbox uses WM_TRANSIENT_FOR to identify transient dialogs,
     * so use the active client as the transient parent.
     */
    Atom net_active_window =
        XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    Window parent = None;

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char* data = nullptr;

    if (XGetWindowProperty(
            display,
            root,
            net_active_window,
            0,
            1,
            False,
            XA_WINDOW,
            &actual_type,
            &actual_format,
            &nitems,
            &bytes_after,
            &data) == Success) {

        if (data && nitems == 1)
            parent = *reinterpret_cast<Window*>(data);

        if (data)
            XFree(data);
    }

    int x = (DisplayWidth(display, screen) - WIDTH) / 2;
    int y = (DisplayHeight(display, screen) - HEIGHT) / 2;

    XSetWindowAttributes attributes{};
    attributes.override_redirect = False;

    Window window = XCreateWindow(
        display,
        root,
        x,
        y,
        WIDTH,
        HEIGHT,
        1,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWOverrideRedirect,
        &attributes
    );

    /*
     * Tell Hackedbox that this is a transient dialog belonging
     * to the currently active application.
     */
    if (parent != None && parent != window) {
        XSetTransientForHint(
            display,
            window,
            parent
        );
    }

    /*
     * Also identify the window as an X11 dialog.
     */
    Atom net_wm_window_type =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

    Atom net_wm_window_type_dialog =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    XChangeProperty(
        display,
        window,
        net_wm_window_type,
        XA_ATOM,
        32,
        PropModeReplace,
        reinterpret_cast<unsigned char*>(
            &net_wm_window_type_dialog
        ),
        1
    );

    XStoreName(display, window, "hbsudo");

    Atom wm_delete =
        XInternAtom(display, "WM_DELETE_WINDOW", False);

    XSetWMProtocols(
        display,
        window,
        &wm_delete,
        1
    );

    XSelectInput(
        display,
        window,
        ExposureMask |
        KeyPressMask |
        ButtonPressMask |
        StructureNotifyMask
    );

    XMapRaised(display, window);
    XFlush(display);

    bool mapped = false;

    while (!mapped) {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == MapNotify)
            mapped = true;
    }

    XGrabKeyboard(
        display,
        window,
        True,
        GrabModeAsync,
        GrabModeAsync,
        CurrentTime
    );

    GC gc = XCreateGC(
        display,
        window,
        0,
        nullptr
    );

    XFontStruct* font =
        XLoadQueryFont(display, "fixed");

    if (font)
        XSetFont(display, gc, font->fid);

    std::string password;

    bool done = false;
    bool cancelled = false;

    while (!done) {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == Expose) {
            draw_dialog(
                display,
                window,
                gc,
                WIDTH,
                password
            );
        }

        else if (event.type == KeyPress) {
            char buffer[32]{};
            KeySym keysym = NoSymbol;

            int length = XLookupString(
                &event.xkey,
                buffer,
                sizeof(buffer),
                &keysym,
                nullptr
            );

            if (keysym == XK_Escape) {
                cancelled = true;
                done = true;
            }

            else if (keysym == XK_Return ||
                     keysym == XK_KP_Enter) {
                done = true;
            }

            else if (keysym == XK_BackSpace) {
                if (!password.empty())
                    password.pop_back();
            }

            else if (length > 0) {
                for (int i = 0; i < length; ++i) {
                    unsigned char c = buffer[i];

                    if (c >= 32 && c != 127)
                        password += static_cast<char>(c);
                }
            }

            if (!done) {
                draw_dialog(
                    display,
                    window,
                    gc,
                    WIDTH,
                    password
                );
            }
        }

        else if (event.type == ButtonPress) {
            if (event.xbutton.x >= WIDTH - 50)
                done = true;
        }

        else if (event.type == ClientMessage) {
            if (static_cast<Atom>(
                    event.xclient.data.l[0]
                ) == wm_delete) {

                cancelled = true;
                done = true;
            }
        }
    }

    XUngrabKeyboard(
        display,
        CurrentTime
    );

    XFreeGC(
        display,
        gc
    );

    if (font)
        XFreeFont(
            display,
            font
        );

    XDestroyWindow(
        display,
        window
    );

    XCloseDisplay(
        display
    );

    if (cancelled || password.empty())
        return 1;

    std::vector<std::string> command;

    for (int i = 1; i < argc; ++i)
        command.emplace_back(argv[i]);

    bool result = authenticate(
        password,
        command
    );

    std::fill(
        password.begin(),
        password.end(),
        '\0'
    );

    password.clear();

    return result ? 0 : 1;
}