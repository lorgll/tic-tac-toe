#include <game.h>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <prelude.h>
#include <renderer.h>
#include <terminal.h>

#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)

#define WIN_COND_LEN 8
static int win_conditions[WIN_COND_LEN][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // columns
    {0, 4, 8}, {2, 4, 6}, // crosses
};

void init_grid(struct grid_state *gs) {
    gs->grid_position_row = 0;
    gs->grid_position_col = 0;
    gs->cursor_position = CURSOR_POSITION_CENTER;
    fill_empty_grid(gs->field);
}

int invoke_current_action(struct game_state *game, struct terminal *tm) {
    player_action action = (game->current_player == PLAYER_1) ?
        game->action_1 : game->action_2;
    return action(game, tm);
}

int player_move(struct game_state *game, struct terminal *tm) {
    char info_msg[MAX_INFO_MSG_LEN];
    int current_player = (game->current_player == PLAYER_1) ? 1 : 2;
    snprintf(info_msg, MAX_INFO_MSG_LEN, "Player %d's turn", current_player);
    print_info_msg(tm, game->grid, info_msg);
    loop {
        int res = read_player_input(game->grid);
        if (res == READ_INTERRUPTED) return READ_INTERRUPTED;
        if (checked_set(game, res)) break;
    }
    return SUCCESS;
}

void random_move(struct game_state *game) {
    int empties = 0;
    for (int i = 0; i < FIELD_LEN; ++i)
        if (game->grid->field[i] == EMPTY)
            ++empties;
    int empty_to_play = 1 + rand() % empties;
    for (int i = 0; i < FIELD_LEN; ++i) {
        if (game->grid->field[i] == EMPTY) --empty_to_play;
        if (empty_to_play == 0) checked_set(game, i);
    }
}
bool defensive_move(struct game_state *game) {
    char *field = game->grid->field;
    for (int i = 0; i < WIN_COND_LEN; ++i) {
        int i0 = win_conditions[i][0];
        int i1 = win_conditions[i][1];
        int i2 = win_conditions[i][2];
        if ((field[i0] == field[i1]) && (field[i0] != EMPTY) && (field[i2] == EMPTY)) {
            checked_set(game, i2); return true;
        }
        if ((field[i1] == field[i2]) && (field[i1] != EMPTY) && (field[i0] == EMPTY)) {
            checked_set(game, i0); return true;
        }
        if ((field[i0] == field[i2]) && (field[i0] != EMPTY) && (field[i1] == EMPTY)) {
            checked_set(game, i1); return true;
        }
    }
    return false;
}

int computer_move_random(struct game_state *game, struct terminal *tm) {
    on_computer_move(tm, game->grid);
    random_move(game);
    return SUCCESS;
}

int computer_move_medium(struct game_state *game, struct terminal *tm) {
    on_computer_move(tm, game->grid);
    if (!defensive_move(game)) random_move(game);
    return SUCCESS;
}

int minimax(struct game_state *gs, int depth, char our_player) {
    int best_val;
    char *field = gs->grid->field;
    if (check_win(gs)) {
        if (gs->current_player != our_player) return 10 - depth;
        else return depth - 10;
    }
    if (is_grid_full(field)) return 0;
    if (gs->current_player == our_player) {
        best_val = INT_MIN;
        for (int i = 0; i < FIELD_LEN; ++i) {
            if (checked_set(gs, i)) {
                switch_player(gs);
                int value = minimax(gs, depth+1, our_player);
                best_val = MAX(best_val, value);
                gs->grid->field[i] = EMPTY; // undoing the move
                switch_player(gs);
            }
        }
    }
    else {
        best_val = INT_MAX;
        for (int i = 0; i < FIELD_LEN; ++i) {
            if (checked_set(gs, i)) {
                switch_player(gs);
                int value = minimax(gs, depth+1, our_player);
                best_val = MIN(best_val, value);
                gs->grid->field[i] = EMPTY; // undoing the move
                switch_player(gs);
            }
        }
    }
    return best_val;
}

int computer_move_impossible(struct game_state *game, struct terminal *tm) {
    on_computer_move(tm, game->grid);
    if (defensive_move(game)) return SUCCESS;
    int cell_to_play = 0;
    int max_value = INT_MIN;
    char our_player = game->current_player;
    for (int i = 0; i < FIELD_LEN; ++i) {
        if (checked_set(game, i)) {
            switch_player(game);
            int value = minimax(game, 0, our_player);
            switch_player(game);
            if (value > max_value) {
                cell_to_play = i;
                max_value = value;
            }
            game->grid->field[i] = EMPTY;
        }
    }
    checked_set(game, cell_to_play);
    return SUCCESS;
}

bool checked_set(struct game_state *state, int pos) {
    assert((pos >= 0) && (pos < FIELD_LEN));
    if (state->grid->field[pos] != EMPTY) return false;
    state->grid->field[pos] = state->current_player;
    return true;
}

void switch_player(struct game_state *gs) {
    gs->current_player = (gs->current_player == PLAYER_1) ? PLAYER_2 : PLAYER_1;
}

void fill_empty_grid(field_t grid) {
    for (int i = 0; i < FIELD_LEN; ++i) grid[i] = EMPTY;
}

bool check_win(struct game_state *gs) {
    char *grid = gs->grid->field;
    for (int i = 0; i < WIN_COND_LEN; ++i) {
        int idx_1 = win_conditions[i][0];
        int idx_2 = win_conditions[i][1];
        int idx_3 =  win_conditions[i][2];
        if ((grid[idx_1] == grid[idx_2]) && (grid[idx_1] == grid[idx_3]) && (grid[idx_1] != EMPTY))
            return true;
    }
    return false;
}

bool is_grid_full(field_t grid) {
    for (int i = 0; i < FIELD_LEN; ++i)
        if (grid[i] == EMPTY) return false;
    return true;
}
