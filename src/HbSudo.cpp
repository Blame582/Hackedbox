// HbSudo.cpp
//
// Graphical sudo password input for Hackedbox.
//
// The dialog appearance is read from the currently selected Hackedbox
// style.  The style is discovered through session.styleFile in
// ~/.hackedbox/hackedbox.rc.
//
// Every visual property has a compiled-in fallback so an older style,
// or a style without window.dialog.* settings, continues to work.
//
// The style file is treated strictly as data.  It is never sourced,
// executed, or passed to a shell.

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>

#include <unistd.h>
#include <sys/wait.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>

static const unsigned int WIDTH  = 420;
static const unsigned int HEIGHT = 54;

static const char* SUDO = "/usr/bin/sudo";
static const char* SH   = "/bin/sh";

struct DialogStyle {
std::string font;


unsigned long background;
unsigned long input;
unsigned long input_text;
unsigned long text;
unsigned long accent;

unsigned long border;
unsigned long focus_border;

unsigned long button;
unsigned long button_to;
unsigned long button_text;
unsigned long button_focus;

unsigned int border_width;
unsigned int alpha;

DialogStyle()
    : font("Hack-13"),
      background(0x14181C),
      input(0x0A141E),
      input_text(0xFFFFFF),
      text(0xFFFFFF),
      accent(0x00FF00),
      border(0x505458),
      focus_border(0x00FF00),
      button(0x32363A),
      button_to(0x14181C),
      button_text(0xFFFFFF),
      button_focus(0x00FF00),
      border_width(1),
      alpha(255)
{
}


};

static std::string trim(const std::string& value)
{
size_t begin = 0;
size_t end = value.size();


while (begin < end &&
       (value[begin] == ' ' ||
        value[begin] == '\t' ||
        value[begin] == '\r' ||
        value[begin] == '\n')) {
    ++begin;
}

while (end > begin &&
       (value[end - 1] == ' ' ||
        value[end - 1] == '\t' ||
        value[end - 1] == '\r' ||
        value[end - 1] == '\n')) {
    --end;
}

return value.substr(begin, end - begin);


}

static bool parse_unsigned(
const std::string& value,
unsigned int& result)
{
char* end = nullptr;


errno = 0;

unsigned long number =
    std::strtoul(
        value.c_str(),
        &end,
        10
    );

if (errno != 0 ||
    end == value.c_str() ||
    *end != '\0') {
    return false;
}

if (number > 255)
    return false;

result = static_cast<unsigned int>(number);
return true;


}

static bool parse_color(
const std::string& value,
unsigned long& result)
{
std::string color = trim(value);


if (color.empty())
    return false;

if (color[0] == '#') {
    if (color.size() != 7)
        return false;

    char* end = nullptr;

    errno = 0;

    unsigned long number =
        std::strtoul(
            color.c_str() + 1,
            &end,
            16
        );

    if (errno != 0 ||
        end == color.c_str() + 1 ||
        *end != '\0' ||
        number > 0xFFFFFFUL) {
        return false;
    }

    result = number;
    return true;
}

return false;


}

static std::map<std::string, std::string> read_style_file(
const std::string& path)
{
std::map<std::string, std::string> values;


std::ifstream file(path);

if (!file)
    return values;

std::string line;

while (std::getline(file, line)) {
    line = trim(line);

    if (line.empty() || line[0] == '#')
        continue;

    size_t colon = line.find(':');

    if (colon == std::string::npos)
        continue;

    std::string key =
        trim(line.substr(0, colon));

    std::string value =
        trim(line.substr(colon + 1));

    if (!key.empty())
        values[key] = value;
}

return values;


}

static std::string get_hackedbox_rc()
{
const char* home = std::getenv("HOME");


if (!home || !*home)
    return std::string();

return std::string(home) +
       "/.hackedbox/hackedbox.rc";


}

static std::string get_style_file()
{
std::string rc_path = get_hackedbox_rc();


if (rc_path.empty())
    return std::string();

std::ifstream file(rc_path);

if (!file)
    return std::string();

std::string line;

while (std::getline(file, line)) {
    line = trim(line);

    if (line.empty() || line[0] == '#')
        continue;

    size_t colon = line.find(':');

    if (colon == std::string::npos)
        continue;

    std::string key =
        trim(line.substr(0, colon));

    if (key != "session.styleFile")
        continue;

    return trim(line.substr(colon + 1));
}

return std::string();


}

static void load_dialog_style(DialogStyle& style)
{
std::string style_path = get_style_file();


if (style_path.empty())
    return;

std::map<std::string, std::string> values =
    read_style_file(style_path);

auto find_value =
    [&values](const char* key) -> const std::string* {
        auto it = values.find(key);

        if (it == values.end())
            return nullptr;

        return &it->second;
    };

if (const std::string* value =
        find_value("window.dialog.font")) {
    if (!value->empty())
        style.font = *value;
}

if (const std::string* value =
        find_value("window.dialog.bgcolor")) {
    parse_color(*value, style.background);
}

if (const std::string* value =
        find_value("window.dialog.inputColor")) {
    parse_color(*value, style.input);
}

if (const std::string* value =
        find_value("window.dialog.inputTextColor")) {
    parse_color(*value, style.input_text);
}

if (const std::string* value =
        find_value("window.dialog.textColor")) {
    parse_color(*value, style.text);
}

if (const std::string* value =
        find_value("window.dialog.accentColor")) {
    parse_color(*value, style.accent);
}

if (const std::string* value =
        find_value("window.dialog.borderColor")) {
    parse_color(*value, style.border);
}

if (const std::string* value =
        find_value("window.dialog.borderColor.focus")) {
    parse_color(*value, style.focus_border);
}

if (const std::string* value =
        find_value("window.dialog.buttonColor")) {
    parse_color(*value, style.button);
}

if (const std::string* value =
        find_value("window.dialog.buttonColorTo")) {
    parse_color(*value, style.button_to);
}

if (const std::string* value =
        find_value("window.dialog.buttonTextColor")) {
    parse_color(*value, style.button_text);
}

if (const std::string* value =
        find_value("window.dialog.buttonFocusColor")) {
    parse_color(*value, style.button_focus);
}

if (const std::string* value =
        find_value("window.dialog.borderWidth")) {
    unsigned int number = 0;

    if (parse_unsigned(*value, number))
        style.border_width = number;
}

if (const std::string* value =
        find_value("window.dialog.alpha")) {
    unsigned int number = 0;

    if (parse_unsigned(*value, number))
        style.alpha = number;
}


}

static unsigned long alloc_color(
Display* display,
int screen,
unsigned long rgb)
{
XColor color{};


color.red =
    static_cast<unsigned short>(
        ((rgb >> 16) & 0xFF) * 257
    );

color.green =
    static_cast<unsigned short>(
        ((rgb >> 8) & 0xFF) * 257
    );

color.blue =
    static_cast<unsigned short>(
        (rgb & 0xFF) * 257
    );

color.flags = DoRed | DoGreen | DoBlue;

Colormap colormap =
    DefaultColormap(display, screen);

if (XAllocColor(
        display,
        colormap,
        &color
    )) {
    return color.pixel;
}

return BlackPixel(display, screen);


}

static void draw_gradient(
Display* display,
Window window,
GC gc,
int x,
int y,
unsigned int width,
unsigned int height,
unsigned long from,
unsigned long to)
{
if (height == 0)
return;


unsigned int r1 =
    static_cast<unsigned int>((from >> 16) & 0xFF);

unsigned int g1 =
    static_cast<unsigned int>((from >> 8) & 0xFF);

unsigned int b1 =
    static_cast<unsigned int>(from & 0xFF);

unsigned int r2 =
    static_cast<unsigned int>((to >> 16) & 0xFF);

unsigned int g2 =
    static_cast<unsigned int>((to >> 8) & 0xFF);

unsigned int b2 =
    static_cast<unsigned int>(to & 0xFF);

for (unsigned int row = 0; row < height; ++row) {
    double ratio =
        static_cast<double>(row) /
        static_cast<double>(height - 1);

    unsigned int r =
        static_cast<unsigned int>(
            r1 + (r2 - r1) * ratio
        );

    unsigned int g =
        static_cast<unsigned int>(
            g1 + (g2 - g1) * ratio
        );

    unsigned int b =
        static_cast<unsigned int>(
            b1 + (b2 - b1) * ratio
        );

    unsigned long rgb =
        (static_cast<unsigned long>(r) << 16) |
        (static_cast<unsigned long>(g) << 8) |
        static_cast<unsigned long>(b);

    XSetForeground(
        display,
        gc,
        alloc_color(
            display,
            DefaultScreen(display),
            rgb
        )
    );

    XDrawLine(
        display,
        window,
        gc,
        x,
        y + static_cast<int>(row),
        x + static_cast<int>(width) - 1,
        y + static_cast<int>(row)
    );
}


}

static void draw_dialog(
Display* display,
Window window,
GC gc,
XftDraw* xft_draw,
XftFont* font,
const DialogStyle& style,
unsigned int width,
unsigned int height,
const std::string& password)
{
int screen = DefaultScreen(display);


unsigned long bg =
    alloc_color(
        display,
        screen,
        style.background
    );

unsigned long input =
    alloc_color(
        display,
        screen,
        style.input
    );

unsigned long border =
    alloc_color(
        display,
        screen,
        style.border
    );

unsigned long accent =
    alloc_color(
        display,
        screen,
        style.accent
    );

unsigned long text =
    alloc_color(
        display,
        screen,
        style.text
    );

unsigned long input_text =
    alloc_color(
        display,
        screen,
        style.input_text
    );

unsigned long button =
    alloc_color(
        display,
        screen,
        style.button
    );

unsigned long button_text =
    alloc_color(
        display,
        screen,
        style.button_text
    );

XSetForeground(
    display,
    gc,
    bg
);

XFillRectangle(
    display,
    window,
    gc,
    0,
    0,
    width,
    height
);

/*
 * Outer border.
 */
XSetForeground(
    display,
    gc,
    border
);

XSetLineAttributes(
    display,
    gc,
    style.border_width,
    LineSolid,
    CapButt,
    JoinMiter
);

XDrawRectangle(
    display,
    window,
    gc,
    0,
    0,
    width - 1,
    height - 1
);

const int label_x = 16;
const int field_x = 90;
const int field_y = 14;
const int field_height = 26;
const int button_width = 30;
const int button_x =
    static_cast<int>(width) - button_width - 12;

/*
 * Input field.
 */
XSetForeground(
    display,
    gc,
    input
);

XFillRectangle(
    display,
    window,
    gc,
    field_x,
    field_y,
    static_cast<unsigned int>(
        button_x - field_x - 8
    ),
    field_height
);

XSetForeground(
    display,
    gc,
    accent
);

XDrawRectangle(
    display,
    window,
    gc,
    field_x,
    field_y,
    static_cast<unsigned int>(
        button_x - field_x - 8
    ),
    field_height
);

/*
 * Submit button.
 */
XSetForeground(
    display,
    gc,
    button
);

XFillRectangle(
    display,
    window,
    gc,
    button_x,
    field_y,
    button_width,
    field_height
);

XSetForeground(
    display,
    gc,
    accent
);

XDrawRectangle(
    display,
    window,
    gc,
    button_x,
    field_y,
    button_width,
    field_height
);

if (font) {
    XftColor label_color{};
    XRenderColor label_render{};

    label_render.red =
        static_cast<unsigned short>(
            ((style.text >> 16) & 0xFF) * 257
        );

    label_render.green =
        static_cast<unsigned short>(
            ((style.text >> 8) & 0xFF) * 257
        );

    label_render.blue =
        static_cast<unsigned short>(
            (style.text & 0xFF) * 257
        );

    label_render.alpha = 0xFFFF;

    XftColorAllocValue(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &label_render,
        &label_color
    );

    XftDrawStringUtf8(
        xft_draw,
        &label_color,
        font,
        label_x,
        32,
        reinterpret_cast<const FcChar8*>(
            "Password:"
        ),
        9
    );

    XftColorFree(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &label_color
    );

    XRenderColor password_render{};

    password_render.red =
        static_cast<unsigned short>(
            ((style.input_text >> 16) & 0xFF) * 257
        );

    password_render.green =
        static_cast<unsigned short>(
            ((style.input_text >> 8) & 0xFF) * 257
        );

    password_render.blue =
        static_cast<unsigned short>(
            (style.input_text & 0xFF) * 257
        );

    password_render.alpha = 0xFFFF;

    XftColor password_color{};

    XftColorAllocValue(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &password_render,
        &password_color
    );

    std::string masked(
        password.size(),
        '*'
    );

    XftDrawStringUtf8(
        xft_draw,
        &password_color,
        font,
        field_x + 8,
        32,
        reinterpret_cast<const FcChar8*>(
            masked.c_str()
        ),
        static_cast<int>(masked.size())
    );

    XftColorFree(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &password_color
    );

    XRenderColor button_render{};

    button_render.red =
        static_cast<unsigned short>(
            ((style.button_text >> 16) & 0xFF) * 257
        );

    button_render.green =
        static_cast<unsigned short>(
            ((style.button_text >> 8) & 0xFF) * 257
        );

    button_render.blue =
        static_cast<unsigned short>(
            (style.button_text & 0xFF) * 257
        );

    button_render.alpha = 0xFFFF;

    XftColor button_color{};

    XftColorAllocValue(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &button_render,
        &button_color
    );

    XftDrawStringUtf8(
        xft_draw,
        &button_color,
        font,
        button_x + 11,
        32,
        reinterpret_cast<const FcChar8*>(">"),
        1
    );

    XftColorFree(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &button_color
    );
}

/*
 * Silence the currently unused colors while keeping them part of
 * the style interface for the next visual pass.
 */
(void)text;
(void)input_text;

XFlush(display);


}

static bool authenticate(
std::string& password,
const std::vector<std::string>& command)
{
if (command.empty())
return false;


int pipefd[2];

if (pipe(pipefd) == -1) {
    std::perror("hbsudo: pipe");
    return false;
}

pid_t pid = fork();

if (pid == -1) {
    std::perror("hbsudo: fork");
    close(pipefd[0]);
    close(pipefd[1]);
    return false;
}

if (pid == 0) {
    close(pipefd[1]);

    if (dup2(pipefd[0], STDIN_FILENO) == -1)
        _exit(127);

    close(pipefd[0]);

    static char shell_script[] =
        "exec </dev/null\n"
        "exec \"$@\"\n";

    std::vector<char*> args;

    args.push_back(const_cast<char*>("sudo"));
    args.push_back(const_cast<char*>("-S"));
    args.push_back(const_cast<char*>("-p"));
    args.push_back(const_cast<char*>(""));
    args.push_back(const_cast<char*>(SH));
    args.push_back(const_cast<char*>("-c"));
    args.push_back(shell_script);
    args.push_back(const_cast<char*>("hbsudo"));

    for (const std::string& arg : command)
        args.push_back(const_cast<char*>(arg.c_str()));

    args.push_back(nullptr);

    execv(SUDO, args.data());

    _exit(127);
}

close(pipefd[0]);

std::string input = password;
input.push_back('\n');

std::fill(
    password.begin(),
    password.end(),
    '\0'
);

password.clear();

size_t offset = 0;

while (offset < input.size()) {
    ssize_t written = write(
        pipefd[1],
        input.data() + offset,
        input.size() - offset
    );

    if (written > 0) {
        offset += static_cast<size_t>(written);
        continue;
    }

    if (written == -1 && errno == EINTR)
        continue;

    break;
}

close(pipefd[1]);

std::fill(
    input.begin(),
    input.end(),
    '\0'
);

input.clear();

int status = 0;

while (waitpid(pid, &status, 0) == -1) {
    if (errno == EINTR)
        continue;

    return false;
}

if (!WIFEXITED(status))
    return false;

return WEXITSTATUS(status) == 0;


}

static Window get_active_window(
Display* display,
Window root)
{
Atom net_active_window =
XInternAtom(
display,
"_NET_ACTIVE_WINDOW",
False
);


Atom actual_type = None;
int actual_format = 0;
unsigned long nitems = 0;
unsigned long bytes_after = 0;
unsigned char* data = nullptr;

Window active = None;

int result = XGetWindowProperty(
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
    &data
);

if (result == Success &&
    data != nullptr &&
    nitems == 1) {

    active = *reinterpret_cast<Window*>(data);
}

if (data)
    XFree(data);

return active;


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


DialogStyle style;

load_dialog_style(style);

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

Window parent = get_active_window(
    display,
    root
);

int x =
    (DisplayWidth(display, screen) - WIDTH) / 2;

int y =
    (DisplayHeight(display, screen) - HEIGHT) / 2;

XSetWindowAttributes attributes{};

attributes.override_redirect = True;
attributes.background_pixel =
    alloc_color(
        display,
        screen,
        style.background
    );
attributes.border_pixel =
    alloc_color(
        display,
        screen,
        style.border
    );

unsigned long value_mask =
    CWOverrideRedirect |
    CWBackPixel |
    CWBorderPixel;

Window window = XCreateWindow(
    display,
    root,
    x,
    y,
    WIDTH,
    HEIGHT,
    style.border_width,
    CopyFromParent,
    InputOutput,
    CopyFromParent,
    value_mask,
    &attributes
);

if (window == None) {
    std::fprintf(
        stderr,
        "hbsudo: cannot create dialog window\n"
    );

    XCloseDisplay(display);
    return 1;
}

if (parent != None && parent != window) {
    XSetTransientForHint(
        display,
        window,
        parent
    );
}

Atom net_wm_window_type =
    XInternAtom(
        display,
        "_NET_WM_WINDOW_TYPE",
        False
    );

Atom net_wm_window_type_dialog =
    XInternAtom(
        display,
        "_NET_WM_WINDOW_TYPE_DIALOG",
        False
    );

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

XStoreName(
    display,
    window,
    "hbsudo"
);

Atom wm_delete =
    XInternAtom(
        display,
        "WM_DELETE_WINDOW",
        False
    );

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

XMapRaised(
    display,
    window
);

XFlush(display);

bool mapped = false;

while (!mapped) {
    XEvent event;

    XNextEvent(
        display,
        &event
    );

    if (event.type == MapNotify &&
        event.xmap.window == window) {

        mapped = true;
    }
}

XSetInputFocus(
    display,
    window,
    RevertToParent,
    CurrentTime
);

XFlush(display);

int grab_result = XGrabKeyboard(
    display,
    window,
    True,
    GrabModeAsync,
    GrabModeAsync,
    CurrentTime
);

if (grab_result != GrabSuccess) {
    std::fprintf(
        stderr,
        "hbsudo: XGrabKeyboard failed: %d\n",
        grab_result
    );

    XDestroyWindow(
        display,
        window
    );

    XCloseDisplay(
        display
    );

    return 1;
}

GC gc = XCreateGC(
    display,
    window,
    0,
    nullptr
);

if (!gc) {
    XUngrabKeyboard(
        display,
        CurrentTime
    );

    XDestroyWindow(
        display,
        window
    );

    XCloseDisplay(
        display
    );

    return 1;
}

XftFont* font =
    XftFontOpenName(
        display,
        screen,
        style.font.c_str()
    );

if (!font) {
    /*
     * Style font failed. Fall back to the existing X11 fixed font
     * so the dialog never fails merely because a font is missing.
     */
    font =
        XftFontOpenName(
            display,
            screen,
            "monospace-13"
        );
}

XftDraw* xft_draw =
    XftDrawCreate(
        display,
        window,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen)
    );

if (!xft_draw) {
    XUngrabKeyboard(
        display,
        CurrentTime
    );

    XFreeGC(
        display,
        gc
    );

    if (font)
        XftFontClose(
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

    return 1;
}

std::string password;

bool done = false;
bool cancelled = false;

while (!done) {
    XEvent event;

    XNextEvent(
        display,
        &event
    );

    if (event.type == Expose) {
        draw_dialog(
            display,
            window,
            gc,
            xft_draw,
            font,
            style,
            WIDTH,
            HEIGHT,
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
                unsigned char c =
                    static_cast<unsigned char>(
                        buffer[i]
                    );

                if (c >= 32 && c != 127)
                    password.push_back(
                        static_cast<char>(c)
                    );
            }
        }

        if (!done) {
            draw_dialog(
                display,
                window,
                gc,
                xft_draw,
                font,
                style,
                WIDTH,
                HEIGHT,
                password
            );
        }
    }

    else if (event.type == ButtonPress) {

        if (event.xbutton.x >=
            static_cast<int>(WIDTH - 50)) {

            cancelled = true;
            done = true;
        }
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

XftDrawDestroy(
    xft_draw
);

XFreeGC(
    display,
    gc
);

if (font)
    XftFontClose(
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

if (cancelled || password.empty()) {
    std::fill(
        password.begin(),
        password.end(),
        '\0'
    );

    return 1;
}

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
