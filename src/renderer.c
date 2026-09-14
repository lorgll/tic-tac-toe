#include <renderer.h>

#ifndef _WIN32
#include <asm-generic/errno-base.h>
#include <bits/types/struct_timeval.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#else
#include <windows.h>
#include <conio.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <game.h>
#include <prelude.h>
#include <terminal.h>


int update_active_screen_info(struct terminal *tm, struct top_frame *tf) {
    if (!is_size_sufficient(tm, tf)) return INSUFFICIENT_TERM_SIZE;
    switch (tf->discriminant) {
        case WELCOME_SCREEN:
            struct welcome_screen *welcome= tf->welcome;
            welcome->x = (tm->height - WELCOME_HEIGHT) / 2 + 1;
            welcome->y = (tm->width - WELCOME_WIDTH) / 2 + 1;
            break;
        case START_SCREEN:
            struct start_screen *start= tf->start;
            start->x = (tm->height - START_SCREEN_HEIGHT) / 2 + 1;
            start->y = (tm->width - START_SCREEN_WIDTH) / 2 + 1;
            break;
        case IN_GAME:
            struct game_state *gs = tf->game;
            unsigned short start_row = (tm->height - GRID_HEIGHT) / 2 + 1;
            unsigned short start_col = (tm->width - GRID_WIDTH) / 2 + 1;
            gs->grid->grid_position_row = start_row;
            gs->grid->grid_position_col = start_col;
            break;
        case AFTER_GAME:
            struct after_game *after_menu = tf->after_menu;
            after_menu->x = (tm->height - AFTER_GAME_MENU_HEIGHT) / 2 + 1;
            after_menu->y = (tm->width - AFTER_GAME_MENU_WIDTH) / 2 + 1;
            break;
    }
    return SUCCESS;
}

bool is_size_sufficient(struct terminal *tm, struct top_frame *tf) {
    switch (tf->discriminant) {
        case WELCOME_SCREEN:
            return ((tm->height >= WELCOME_HEIGHT) && (tm->width >= WELCOME_WIDTH));
        case START_SCREEN:
            return ((tm->height >= START_SCREEN_HEIGHT) && (tm->width >= START_SCREEN_WIDTH));
        case IN_GAME:
            return ((tm->height >= GRID_HEIGHT + 2) && (tm->width >= MAX_HELP_MSG_LEN));
        case AFTER_GAME:
            return ((tm->height >= AFTER_GAME_MENU_HEIGHT) && (tm->width >= AFTER_GAME_MENU_WIDTH));
    }
    return false; // unreachable
}

void display_in_grid_position(struct terminal *tm, struct game_state *state) {
    assert((tm->height >= GRID_HEIGHT) && (tm->width >= GRID_WIDTH));
    unsigned short start_row = (tm->height - GRID_HEIGHT) / 2 + 1;
    unsigned short start_col = (tm->width - GRID_WIDTH) / 2 + 1;
    state->grid->grid_position_row = start_row;
    state->grid->grid_position_col = start_col;
    print_grid(state->grid);
}

void print_difficulty_selector(struct start_screen *ss) {
    int x = ss->x;
    int y = ss->y;
    printf("\033[%hu;%huH┌──────────────────────────────┐", x, y);
    printf("\033[%hu;%huH│                              │", x+1, y);
    printf("\033[%hu;%huH│      Choose difficulty       │", x+2, y);
    printf("\033[%hu;%huH│                              │", x+3, y);
    printf("\033[%hu;%huH│   1. Random                  │", x+4, y);
    printf("\033[%hu;%huH│   2. Medium                  │", x+5, y);
    printf("\033[%hu;%huH│   3. Impossible              │", x+6, y);
    printf("\033[%hu;%huH│                              │", x+7, y);
    printf("\033[%hu;%huH└──────────────────────────────┘", x+8, y);
    fflush(stdout);
}

player_action select_comp_difficulty(struct start_screen *ss) {
    print_difficulty_selector(ss);
    int current_selected = ss->choice;
    int offset = ss->choice;
    char options[3][31] = {
        "   1. Random                  ",
        "   2. Medium                  ",
        "   3. Impossible              ",
    };
    player_action diffs[3] = { &computer_move_random, &computer_move_medium, &computer_move_impossible };
    loop {
        printf("\033[%hu;%huH│\033[7m%s\033[0m│", ss->x+4+offset, ss->y, options[offset]);
        fflush(stdout);
        switch (read_special_key()) {
            case READ_INTERRUPTED: return NULL;
            case KC_UP:
                current_selected -= 1;
                break;
            case KC_DOWN:
                current_selected += 1;
                break;
            case KC_ENTER:
                ss->choice = 0;
                return diffs[offset];
        }
        printf("\033[%hu;%huH│\033[0m%s│", ss->x+4+offset, ss->y, options[offset]);
        fflush(stdout);
        offset = (3 + current_selected) % 3;
        current_selected = offset;
        ss->choice = offset;
    }
    return NULL; // unreachable
}

void print_player_selector(struct start_screen *ss, int player_num) {
    int x = ss->x;
    int y = ss->y;
    printf("\033[%hu;%huH┌──────────────────────────────┐", x, y);
    printf("\033[%hu;%huH│                              │", x+1, y);
    printf("\033[%hu;%huH│  Who will play as Player %d?  │", x+2, y, player_num);
    printf("\033[%hu;%huH│                              │", x+3, y);
    printf("\033[%hu;%huH│   1. Person                  │", x+4, y);
    printf("\033[%hu;%huH│   2. Computer                │", x+5, y);
    printf("\033[%hu;%huH│                              │", x+6, y);
    printf("\033[%hu;%huH│                              │", x+7, y);
    printf("\033[%hu;%huH└──────────────────────────────┘", x+8, y);
    fflush(stdout);
}

// TODO: refactor if possible
enum selector select_action(struct start_screen *ss, int player_num) {
    print_player_selector(ss, player_num);
    int current_selected = ss->choice;
    int offset = ss->choice;
    char options[2][31] = {
        "   1. Person                  ",
        "   2. Computer                ",
    };
    loop {
        printf("\033[%hu;%huH│\033[7m%s\033[0m│", ss->x+4+offset, ss->y, options[offset]);
        fflush(stdout);
        switch (read_special_key()) {
            case READ_INTERRUPTED:
                return INTERRUPTED;
            case KC_UP:
                current_selected -= 1;
                break;
            case KC_DOWN:
                current_selected += 1;
                break;
            case KC_ENTER:
                ss->choice = 0;
                if (offset == 0) return PLAYER_MOVE;
                else return COMPUTER_MOVE;
        }
        printf("\033[%hu;%huH│\033[0m%s│", ss->x+4+offset, ss->y, options[offset]);
        fflush(stdout);
        offset = (2 + current_selected) % 2;
        current_selected = offset;
        ss->choice = offset;
    }
    return INTERRUPTED; // unreachable
}

void print_welcome(struct welcome_screen *welcome) {
    int x = welcome->x;
    int y = welcome->y;
    printf("\033[%hu;%huH┌──────────────────────────────┐", x, y);
    printf("\033[%hu;%huH│                              │", x+1, y);
    printf("\033[%hu;%huH│  Welcome to the Tic-Tac-Toe! │", x+2, y);
    printf("\033[%hu;%huH│                              │", x+3, y);
    printf("\033[%hu;%huH│     Press Enter to begin     │", x+4, y);
    printf("\033[%hu;%huH│                              │", x+5, y);
    printf("\033[%hu;%huH│                              │", x+6, y);
    printf("\033[%hu;%huH│     Developed by lorglL      │", x+7, y);
    printf("\033[%hu;%huH└──────────────────────────────┘", x+8, y);
    fflush(stdout);
}

void print_after_game_menu(struct after_game *menu) {
    int x = menu->x;
    int y = menu->y;
    printf("\033[%hu;%huH┌──────────────────────────────┐", x, y);
    printf("\033[%hu;%huH│                              │", x+1, y);
    printf("\033[%hu;%huH│      Do you want to play     │", x+2, y);
    printf("\033[%hu;%huH│        one more time?        │", x+3, y);
    printf("\033[%hu;%huH│                              │", x+4, y);
    printf("\033[%hu;%huH│    [Y] - Yes, another game   │", x+5, y);
    printf("\033[%hu;%huH│    [N] - No, quit the game   │", x+6, y);
    printf("\033[%hu;%huH│                              │", x+7, y);
    printf("\033[%hu;%huH└──────────────────────────────┘", x+8, y);
    fflush(stdout);
}

void print_grid(struct grid_state *state) {
    char *grid = state->field;
    unsigned short start_row = state->grid_position_row;
    unsigned short start_col = state->grid_position_col;
    printf("\033[%hu;%huH┌───┬───┬───┐", start_row, start_col);
    printf("\033[%hu;%huH│ %c │ %c │ %c │", start_row+1, start_col, grid[0], grid[1], grid[2]);
    printf("\033[%hu;%huH├───┼───┼───┤", start_row+2, start_col);
    printf("\033[%hu;%huH│ %c │ %c │ %c │", start_row+3, start_col, grid[3], grid[4], grid[5]);
    printf("\033[%hu;%huH├───┼───┼───┤", start_row+4, start_col);
    printf("\033[%hu;%huH│ %c │ %c │ %c │", start_row+5, start_col, grid[6], grid[7], grid[8]);
    printf("\033[%hu;%huH└───┴───┴───┘", start_row+6, start_col);
    fflush(stdout);
}

void print_info_msg(struct terminal *tm, struct grid_state *state, char *info_msg) {
    int len = strlen(info_msg);
    if (len > MAX_INFO_MSG_LEN) return;
    unsigned short row = state->grid_position_row - 1;
    unsigned short col = (tm->width - len) / 2 + 1;
    printf("\033[%hu;%huH%s", row, col, info_msg);
    fflush(stdout);
}

void print_help_msg(struct terminal *tm, struct grid_state *state, char *help_msg) {
    int len = strlen(help_msg);
    if (len >= MAX_HELP_MSG_LEN) return;
    unsigned short row = state->grid_position_row + 7;
    unsigned short col = (tm->width - len) / 2 + 1;
    printf("\033[%hu;%huH%s", row, col, help_msg);
    fflush(stdout);
}

int safe_read_char(char *dest) {
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD record;
    DWORD read;

    loop {
        if (!ReadConsoleInput(hStdin, &record, 1, &read) || read == 0) {
            return READ_INTERRUPTED;
        }
        if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            return READ_INTERRUPTED;
        }

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            char ch = record.Event.KeyEvent.uChar.AsciiChar;
            if (ch != 0) {
                *dest = ch;
                return SUCCESS;
            }
        }
    }
    return READ_INTERRUPTED;
#else
    int result = read(STDIN_FILENO, dest, 1);
    if (result != 1) return READ_INTERRUPTED;
    return SUCCESS;
#endif
}

bool read_next_byte_nonblocking(char *dest) {
#ifndef _WIN32
    // Linux-версия остаётся без изменений "as is"
    fd_set fd;
    struct timeval tv = {0, 0};
    FD_ZERO(&fd);
    FD_SET(STDIN_FILENO, &fd);
    if (select(STDIN_FILENO + 1, &fd, NULL, NULL, &tv) <= 0) return false;
    return read(STDIN_FILENO, dest, 1) == 1;
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD numEvents = 0;

    // Если в буфере вообще ничего нет — выходим
    if (!GetNumberOfConsoleInputEvents(hStdin, &numEvents) || numEvents == 0) {
        return false;
    }

    // Читаем события по одному, пока очередь не опустеет на эту итерацию
    while (numEvents > 0) {
        INPUT_RECORD record;
        DWORD read;

        // Смотрим на текущее первое событие в очереди
        if (!PeekConsoleInput(hStdin, &record, 1, &read) || read == 0) {
            return false;
        }

        // 1. ЕСЛИ ЭТО РЕСАЙЗ: Взводим флаг, удаляем событие и сразу сигнализируем игре
        if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            trigger_redraw(0); // Ваш atomic-флаг
            ReadConsoleInput(hStdin, &record, 1, &read); // Извлекаем (удаляем) его из очереди
            return false; // Прерываемся, чтобы loop_preprocessor успел вызвать on_resize
        }

        // 2. ЕСЛИ ЭТО КЛАВИАТУРА:
        if (record.EventType == KEY_EVENT) {
            // Нам нужны только нажатия печатных символов (для стрелок AsciiChar уже генерируется благодаря ANSI-режиму)
            if (record.Event.KeyEvent.bKeyDown && record.Event.KeyEvent.uChar.AsciiChar != 0) {
                ReadConsoleInput(hStdin, &record, 1, &read); // Окончательно забираем байт
                *dest = record.Event.KeyEvent.uChar.AsciiChar;
                return true; // Возвращаем true — байт успешно считан
            }
        }

        // 3. ЕСЛИ ЭТО МУСОР (отпускание клавиш, мышь, фокус):
        // Просто удаляем его из очереди, чтобы продвинуться к следующему событию
        ReadConsoleInput(hStdin, &record, 1, &read);

        // Обновляем количество оставшихся событий в очереди
        if (!GetNumberOfConsoleInputEvents(hStdin, &numEvents)) {
            break;
        }
    }

    return false;
#endif
}


int read_key_unchecked(void) {
    char c;
    if (safe_read_char(&c) != SUCCESS) return READ_INTERRUPTED;

    if (c == '\033') {
        char cc[2];
        if (!read_next_byte_nonblocking(&cc[0])) return UNKNOWN_KEY;
        if (!read_next_byte_nonblocking(&cc[1])) return UNKNOWN_KEY;

        if (cc[0] == '[') {
            switch (cc[1]) {
                case 'A': return KC_UP;
                case 'B': return KC_DOWN;
                case 'C': return KC_RIGHT;
                case 'D': return KC_LEFT;
            }
        }
    }
    else if (c == '\r' || c == '\n')
        return KC_ENTER;
    else switch (c) {
        case 'n': return KC_N;
        case 'y': return KC_Y;
        case 'q': trigger_termination(0);
    }
    return UNKNOWN_KEY;
}

int read_special_key() {
#ifndef _WIN32
    tcflush(STDIN_FILENO, TCIFLUSH);
#else
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
    return read_key_unchecked();
}

void ai_delay_one_second(void) {
    for (int i = 0; i < 20; ++i) {
#ifndef _WIN32
        loop {
            fd_set fd;
            struct timeval tv = {0, 0};
            FD_ZERO(&fd);
            FD_SET(STDIN_FILENO, &fd);

            if (select(STDIN_FILENO + 1, &fd, NULL, NULL, &tv) <= 0) break;
            read_key_unchecked();
        }
        usleep(50000);
#else
        loop {
            char dummy;
            if (!read_next_byte_nonblocking(&dummy)) break;

            read_key_unchecked();
        }
        Sleep(50);
#endif
        if (is_exit_requested()) return;
    }
}

void on_computer_move(struct terminal *tm, struct grid_state *state) {
    char info_msg[MAX_INFO_MSG_LEN] = "Computer is thinking...";
    print_info_msg(tm, state, info_msg);
    ai_delay_one_second();
}

int read_player_input(struct grid_state *state) {
    int offsets[9][2] = {{1, 2}, {1, 6}, {1, 10},
                        {3, 2}, {3, 6}, {3, 10},
                        {5, 2}, {5, 6}, {5, 10}};
    int row_offset = state->cursor_position / 3;
    int col_offset = state->cursor_position % 3;
    show_cursor(); cursor_solid_bar();
    int key_code, offset;
    loop {
        row_offset = (3 + row_offset) % 3;
        col_offset = (3 + col_offset) % 3;
        offset = 3 * row_offset + col_offset;
        state->cursor_position = offset;
        printf("\033[%hu;%huH",
                       state->grid_position_row + offsets[offset][0],
                       state->grid_position_col + offsets[offset][1]);
                fflush(stdout);
        key_code = read_special_key();
        if (key_code == EINTR) {
            offset = READ_INTERRUPTED;
            break;
        }
        if (key_code == KC_ENTER) break;
        switch (key_code) {
            case KC_UP:
                row_offset -= 1;
                break;
            case KC_DOWN:
                row_offset += 1;
                break;
            case KC_LEFT:
                col_offset -= 1;
                break;
            case KC_RIGHT:
                col_offset += 1;
                break;
        }
    }
    if (offset != READ_INTERRUPTED)
        state->cursor_position = CURSOR_POSITION_CENTER;
    hide_cursor(); restore_cursor_style();
    fflush(stdout);
    return offset;
}
