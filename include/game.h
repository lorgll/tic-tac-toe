#pragma once

#ifndef _GAME_H
#define _GAME_H

#include <terminal.h>

#define FIELD_LEN 9

#define EMPTY ' '
#define PLAYER_1 'X'
#define PLAYER_2 'O'

enum game_result {
  DRAW = 0,
  PLAYER_1_WON,
  PLAYER_2_WON,
};

struct game_state;
typedef char field_t[FIELD_LEN];
typedef int (*player_action)(struct game_state *state, struct terminal *tm);
struct grid_state {
  int cursor_position;
  int grid_position_row;
  int grid_position_col;
  field_t field;
};
void init_grid(struct grid_state *gs);
struct game_state {
  player_action action_1;
  player_action action_2;
  struct grid_state *grid;
  char current_player;
};
void init_game(struct game_state *gs, player_action action_1,
               player_action action_2, struct grid_state *grid);

int invoke_current_action(struct game_state *game, struct terminal *tm);
int player_move(struct game_state *game, struct terminal *tm);
int computer_move_random(struct game_state *game, struct terminal *tm);
int computer_move_medium(struct game_state *game, struct terminal *tm);
int computer_move_impossible(struct game_state *game, struct terminal *tm);

bool checked_set(struct game_state *state, int pos);
void switch_player(struct game_state *gs);
bool check_win(struct game_state *gs);
void fill_empty_grid(field_t grid);
bool is_grid_full(field_t grid);

#endif // _GAME_H
