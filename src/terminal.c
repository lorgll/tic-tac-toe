#include <terminal.h>

#ifndef _WIN32
#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#else
#include <windows.h>
#include <consoleapi.h>
#include <handleapi.h>
#include <processenv.h>
#include <winbase.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <prelude.h>

#ifndef _WIN32
struct termios terminal_state;
#else
DWORD origStdin;
DWORD origStdout;
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
#endif
}

int setup_terminal() {
    setup_resize_signal();
    setup_termination_signal();
    if (begin_session() == NOT_A_TERMINAL) return NOT_A_TERMINAL;
    atexit(&end_session);
    return SUCCESS;
}

int begin_session() {
#ifndef _WIN32
    // --- LINUX: Исходная логика "as is" ---
    if (tcgetattr(STDOUT_FILENO, &terminal_state) == ENOTTY)
        return NOT_A_TERMINAL;
    hide_cursor();
    enter_alternate_sb();
    fflush(stdout);

    struct termios raw = terminal_state;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDOUT_FILENO, TCSADRAIN, &raw);
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE)
        return NOT_A_TERMINAL;

    DWORD inMode, outMode;
    if (!GetConsoleMode(hStdin, &inMode) || !GetConsoleMode(hStdout, &outMode))
        return NOT_A_TERMINAL;

    origStdin = inMode;
    origStdout = outMode;

    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    inMode |= ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT;

    outMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    outMode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;

    if (!SetConsoleMode(hStdin, inMode) || !SetConsoleMode(hStdout, outMode))
        return NOT_A_TERMINAL;

    hide_cursor();
    enter_alternate_sb();
    fflush(stdout);
#endif
    return SUCCESS;
}

void end_session() {
#ifndef _WIN32
    tcsetattr(STDOUT_FILENO, TCSADRAIN, &terminal_state);
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(hStdin, origStdin);
    SetConsoleMode(hStdout, origStdout);
#endif
    exit_alternate_sb();
    show_cursor();
}

void enter_alternate_sb() { printf("\033[?1049h"); }
void exit_alternate_sb() { printf("\033[?1049l"); }
void cursor_solid_bar() { printf("\033[2 q"); }
void restore_cursor_style() { printf("\033[0 q"); }
void hide_cursor() { printf("\033[?25l"); }
void show_cursor() { printf("\033[?25h"); }
void clear_terminal() { printf("\033[2J\033[1;1H"); }

void get_size(struct terminal *tm) {
#ifndef _WIN32
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    tm->height = ws.ws_row;
    tm->width = ws.ws_col;
#else
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    tm->height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    tm->width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif
}

int on_resize(struct terminal *tm, resize_handler_t handler, void *data) {
    is_resize_requested = 0;
    get_size(tm);
    if (handler != NULL) return handler(tm, data);
    return SUCCESS;
}
