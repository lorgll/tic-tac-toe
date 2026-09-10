#include <game.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <prelude.h>
#include <renderer.h>
#include <terminal.h>

void init_grid(struct grid_state *gs) {
    gs->grid_position_row = 0;
    gs->grid_position_col = 0;
    gs->cursor_position = CURSOR_POSITION_CENTER;
    fill_empty_grid(gs->field);
}

void init_game(struct game_state *gs,
    player_action action_1, player_action action_2,
    struct grid_state *grid)
{
    gs->action_1 = action_1;
    gs->action_2 = action_2;
    gs->current_player = PLAYER_1;
    gs->grid = grid;
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
    print_info_msg(tm, game, info_msg);
    loop {
        int res = read_player_input(game->grid);
        if (res == READ_INTERRUPTED) return READ_INTERRUPTED;
        if (checked_set(game, res)) break;
    }
    return SUCCESS;
}

bool checked_set(struct game_state *state, int pos) {
    assert((pos >= 0) && (pos <= 8));
    if (state->grid->field[pos] != EMPTY) return false;
    state->grid->field[pos] = state->current_player;
    return true;
}

void switch_player(struct game_state *gs) {
    gs->current_player = (gs->current_player == PLAYER_1) ? PLAYER_2 : PLAYER_1;
}

void fill_empty_grid(field_t grid) {
    for (int i = 0; i < 9; ++i) {
        grid[i] = EMPTY;
    }
}

bool check_win(struct game_state *gs) {
    int win_conditions[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // columns
        {0, 4, 8}, {2, 4, 6}, // crosses
    };
    char *grid = gs->grid->field;
    char player = gs->current_player;
    for (int i = 0; i < 8; ++i) {
        int idx_1 = win_conditions[i][0];
        int idx_2 = win_conditions[i][1];
        int idx_3 =  win_conditions[i][2];
        if ((grid[idx_1] == grid[idx_2]) && (grid[idx_1] == grid[idx_3]) && (grid[idx_1] == player))
            return true;
    }
    return false;
}

bool is_grid_full(field_t grid) {
    for (int i = 0; i < 9; ++i)
        if (grid[i] == EMPTY) return false;
    return true;
}
