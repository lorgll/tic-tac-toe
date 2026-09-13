#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include <game.h>
#include <prelude.h>
#include <renderer.h>
#include <terminal.h>

void print_welcome_screen(struct terminal *tm, struct top_frame *tf);
void start_screen(struct terminal *tm, struct top_frame *tf, struct game_state *gs);
player_action select_comp(struct terminal *tm, struct top_frame *tf);
enum game_result game_stage(struct terminal *tm, struct top_frame *tf);
void game_result_screen(struct terminal *tm, struct top_frame *tf, enum game_result result);
bool after_game_screen(struct terminal *tm, struct top_frame *tf);

int resize_handler(struct terminal *tm, void *data);
// the app is split into stages, so this function
// will be handling resizes, flushing and FPS control
// on each stage
enum frame_verdict loop_preprocessor(struct terminal *tm, struct top_frame *tf);

int main(void) {
    srand(time(NULL));
    if (setup_terminal() == NOT_A_TERMINAL) {
        perror("not a terminal");
        return NOT_A_TERMINAL;
    }
    struct terminal tm;
    get_size(&tm);

    struct top_frame tf;
    struct welcome_screen welcome;
    tf.discriminant = WELCOME_SCREEN;
    tf.welcome = &welcome;
    print_welcome_screen(&tm, &tf);

    loop {
        struct grid_state grid; init_grid(&grid);
        struct game_state game;
        struct start_screen start;
        tf.discriminant = START_SCREEN;
        tf.start = &start;
        game.grid = &grid;
        game.current_player = PLAYER_1;
        start_screen(&tm, &tf, &game);

        tf.discriminant = IN_GAME;
        tf.game = &game;
        enum game_result res = game_stage(&tm, &tf);
        game_result_screen(&tm, &tf, res);

        struct after_game after_menu;
        tf.discriminant = AFTER_GAME;
        tf.after_menu = &after_menu;
        bool another_game = after_game_screen(&tm, &tf);
        if (!another_game) break;
    }

    return SUCCESS;
}

void print_welcome_screen(struct terminal *tm, struct top_frame *tf) {
    trigger_redraw(0);
    loop {
        if (loop_preprocessor(tm, tf) == SKIP) continue;
        print_welcome(tf->welcome);
        if (read_special_key() == KC_ENTER) break;
    }
}

int resize_handler(struct terminal *tm, void *data) {
    struct top_frame *state = (struct top_frame *)data;
    return update_active_screen_info(tm, state);
}

enum frame_verdict loop_preprocessor(struct terminal *tm, struct top_frame *tf) {
    fflush(stdout);
    usleep(FPS_30);
    clear_terminal();
    if (is_resized()) on_resize(tm, &resize_handler, (void *)tf);
    if (!is_size_sufficient(tm, tf)) {
        printf("Expand your terminal!");
        return SKIP;
    }
    return ALLOW;
}

player_action select_comp(struct terminal *tm, struct top_frame *tf) {
    trigger_redraw(0);
    player_action comp;
    loop {
        if (loop_preprocessor(tm ,tf) == SKIP) continue;
        comp = select_comp_difficulty(tf->start);
        if (comp != NULL) break;
    }
    return comp;
}

void start_screen(struct terminal *tm, struct top_frame *tf, struct game_state *gs) {
    trigger_redraw(0);
    player_action action_1 = NULL;
    player_action action_2 = NULL;
    tf->start->choice = 0;
    loop {
        if (loop_preprocessor(tm, tf) == SKIP) continue;
        if (action_1 == NULL) {
            switch (select_action(tf->start, 1)) {
                case PLAYER_MOVE: action_1 = &player_move; break;
                case INTERRUPTED: continue;
                case COMPUTER_MOVE: action_1 = select_comp(tm, tf);
            }
        }
        else if (action_2 == NULL) {
            switch (select_action(tf->start, 2)) {
                case PLAYER_MOVE: action_2 = &player_move; break;
                case INTERRUPTED: continue;
                case COMPUTER_MOVE: action_2 = select_comp(tm, tf);
            }
        }
        else break;
    }
    gs->action_1 = action_1;
    gs->action_2 = action_2;
}

enum game_result game_stage(struct terminal *tm, struct top_frame *tf) {
    trigger_redraw(0);
    struct game_state *gs = tf->game;
    loop {
        if (loop_preprocessor(tm, tf) == SKIP) continue;
        display_in_grid_position(tm, gs);
        static char HELP_MSG[MAX_HELP_MSG_LEN] = "Arrow - control, Enter - choose";
        print_help_msg(tm, gs->grid, HELP_MSG);
        int read_result = invoke_current_action(gs, tm);
        if (check_win(gs)) break;
        if (is_grid_full(gs->grid->field)) return DRAW;
        if (read_result == SUCCESS) switch_player(gs);
    }
    return (gs->current_player == PLAYER_1) ? PLAYER_1_WON : PLAYER_2_WON;
}

void game_result_screen(struct terminal *tm, struct top_frame *tf, enum game_result res) {
    loop {
        if (loop_preprocessor(tm, tf) == SKIP) continue;
        display_in_grid_position(tm, tf->game);
        char info_msg[MAX_INFO_MSG_LEN];
        if (res == DRAW) snprintf(info_msg, MAX_INFO_MSG_LEN, "It's a draw!");
        else snprintf(info_msg, MAX_INFO_MSG_LEN, "Player %d has won!", (int)res);
        print_info_msg(tm, tf->game->grid, info_msg);
        static char HELP_MSG[MAX_HELP_MSG_LEN] = "Press Enter to continue";
        print_help_msg(tm, tf->game->grid, HELP_MSG);
        if (read_special_key() == KC_ENTER) return;
    }
}

bool after_game_screen(struct terminal *tm, struct top_frame *tf) {
    trigger_redraw(0);
    loop {
        if (loop_preprocessor(tm, tf) == SKIP) continue;
        print_after_game_menu(tf->after_menu);
        switch (read_special_key()) {
            case READ_INTERRUPTED: continue;
            case KC_N: return false;
            case KC_Y: return true;
        }
    }
    return false; // unreachable
}
