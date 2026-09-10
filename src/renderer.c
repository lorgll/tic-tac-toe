#include <renderer.h>

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <game.h>
#include <prelude.h>
#include <terminal.h>


int update_active_screen_info(struct terminal *tm, struct top_frame *tf) {
    if (!is_size_sufficient(tm, tf)) return INSUFFICIENT_TERM_SIZE;
    switch (tf->discriminant) {
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

void print_info_msg(struct terminal *tm, struct game_state *gs, char *info_msg) {
    int len = strlen(info_msg);
    if (len > MAX_INFO_MSG_LEN) return;
    unsigned short row = gs->grid->grid_position_row - 1;
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

int read_special_key() {
    tcflush(STDIN_FILENO, TCIFLUSH);
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return EINTR;
    if (c == '\033') {
        char cc[2];
        if (read(STDIN_FILENO, &cc[0], 1) != 1) return EINTR;
        if (read(STDIN_FILENO, &cc[1], 1) != 1) return EINTR;
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
