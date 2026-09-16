#include <terminal.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <prelude.h>

#ifndef _WIN32
struct termios terminal_state;
#else
DWORD original_console_mode;
#endif

volatile sig_atomic_t should_exit = 0;
volatile sig_atomic_t is_resize_requested = 0;

bool is_resized() {
    return is_resize_requested == 1;
}

bool is_exit_requested() {
    return should_exit == 1;
}

void trigger_redraw([[maybe_unused]] int sig) {
    is_resize_requested = 1;
}

void trigger_termination([[maybe_unused]] int sig) {
    should_exit = 1;
}

void enter_alternate_sb() { printf("\033[?1049h"); }
void exit_alternate_sb() { printf("\033[?1049l"); }
void cursor_solid_bar() { printf("\033[2 q"); }
void restore_cursor_style() { printf("\033[0 q"); }
void hide_cursor() { printf("\033[?25l"); }
void show_cursor() { printf("\033[?25h"); }
void clear_terminal() { printf("\033[3J\033[2J\033[H"); }

void setup_resize_signal() {
#ifndef _WIN32
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &trigger_redraw;
    sigaction(SIGWINCH, &sa, NULL);
#endif
}

void setup_termination_signal() {
#ifndef _WIN32
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &trigger_termination;
    int signals[] = {SIGINT, SIGQUIT, SIGTERM, SIGHUP};
    int count = sizeof(signals) / sizeof(signals[0]);
    for (int i = 0; i < count; ++i) {
        sigaction(signals[i], &sa, NULL);
    }
#else
    signal(SIGINT, &trigger_termination);
    signal(SIGTERM, &trigger_termination);
#endif
}


int begin_session() {
    hide_cursor();
    enter_alternate_sb();
    fflush(stdout);

#ifndef _WIN32
    if (tcgetattr(STDOUT_FILENO, &terminal_state) == ENOTTY)
        return NOT_A_TERMINAL;

    struct termios raw = terminal_state;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDOUT_FILENO, TCSADRAIN, &raw);
#else
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE || hOutput == INVALID_HANDLE_VALUE) {
        return NOT_A_TERMINAL;
    }

    DWORD outMode;
    if (GetConsoleMode(hOutput, &outMode)) {
        outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOutput, outMode);
    }

    DWORD inMode;
    if (GetConsoleMode(hInput, &inMode)) {
        inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        SetConsoleMode(hInput, inMode);
    }
#endif
    return SUCCESS;
}

void end_session() {
#ifndef _WIN32
    tcsetattr(STDOUT_FILENO, TCSADRAIN, &terminal_state);
#else
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput != INVALID_HANDLE_VALUE)
        SetConsoleMode(hInput, original_console_mode);
#endif
    exit_alternate_sb();
    show_cursor();
}

int setup_terminal() {
    setup_resize_signal();
    setup_termination_signal();
    if (begin_session() == NOT_A_TERMINAL) return NOT_A_TERMINAL;
    atexit(&end_session);
    return SUCCESS;
}

void get_size(struct terminal *tm) {
#ifndef _WIN32
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    tm->height = ws.ws_row;
    tm->width = ws.ws_col;
#else
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (hStdOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        tm->width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        tm->height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        tm->width = 80;
        tm->height = 24;
    }
#endif
}

int on_resize(struct terminal *tm, resize_handler_t handler, void *data) {
    is_resize_requested = 0;
    get_size(tm);
    if (handler != NULL) return handler(tm, data);
    return SUCCESS;
}

bool special_os_resize_processor(struct terminal *tm) {
#ifdef _WIN32
    unsigned short old_height = tm->height,
                old_width = tm->width;
    get_size(tm);
    if ((tm->height != old_height) || (tm->width != old_width)) {
        return true;
    }
#endif
    return false;
}
