#include <terminal.h>

#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <prelude.h>

struct termios terminal_state;
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
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &trigger_redraw;
    sigaction(SIGWINCH, &sa, NULL);
}

void setup_termination_signal() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &trigger_termination;
    int signals[] = {SIGINT, SIGQUIT, SIGTERM, SIGHUP};
    int count = sizeof(signals) / sizeof(signals[0]);
    for (int i = 0; i < count; ++i) {
        sigaction(signals[i], &sa, NULL);
    }
}

int setup_terminal() {
    setup_resize_signal();
    setup_termination_signal();
    if (begin_session() == NOT_A_TERMINAL) return NOT_A_TERMINAL;
    atexit(&end_session);
    return SUCCESS;
}

int begin_session() {
    hide_cursor();
    enter_alternate_sb();
    fflush(stdout);
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
    return SUCCESS;
}

void end_session() {
    tcsetattr(STDOUT_FILENO, TCSADRAIN, &terminal_state);
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
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    tm->height = ws.ws_row;
    tm->width = ws.ws_col;
}

int on_resize(struct terminal *tm, resize_handler_t handler, void *data) {
    is_resize_requested = 0;
    get_size(tm);
    if (handler != NULL) return handler(tm, data);
    return SUCCESS;
}
