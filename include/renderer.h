#pragma once

#ifndef _RENDERER_H
#define _RENDERER_H

#include <game.h>
#include <terminal.h>

#define WELCOME_HEIGHT 9
#define WELCOME_WIDTH 32
#define START_SCREEN_HEIGHT 9
#define START_SCREEN_WIDTH 32
#define GRID_HEIGHT 7
#define GRID_WIDTH 13
#define MAX_INFO_MSG_LEN 32
#define MAX_HELP_MSG_LEN 32
#define AFTER_GAME_MENU_HEIGHT 9
#define AFTER_GAME_MENU_WIDTH 32

struct welcome_screen {
  unsigned int x;
  unsigned int y;
};
struct start_screen {
  unsigned short x;
  unsigned short y;
  int choice;
};
struct after_game {
  unsigned short x;
  unsigned short y;
};
enum frame_verdict {
  ALLOW,
  SKIP,
};
enum app_state {
  WELCOME_SCREEN,
  START_SCREEN,
  IN_GAME,
  AFTER_GAME,
};
struct top_frame {
  enum app_state discriminant;
  union {
    struct welcome_screen *welcome;
    struct start_screen *start;
    struct game_state *game;
    struct after_game *after_menu;
  };
};
int update_active_screen_info(struct terminal *tm, struct top_frame *tf);
bool is_size_sufficient(struct terminal *tm, struct top_frame *tf);

enum selector {
  PLAYER_MOVE,
  COMPUTER_MOVE,
  INTERRUPTED,
};
enum selector select_action(struct start_screen *ss, int player_num);
player_action select_comp_difficulty(struct start_screen *ss);
void display_in_grid_position(struct terminal *tm, struct game_state *state);
void print_welcome(struct welcome_screen *welcome);
void print_after_game_menu(struct after_game *menu);
void print_grid(struct grid_state *state);
void print_info_msg(struct terminal *tm, struct grid_state *state,
                    char *info_msg);
void print_help_msg(struct terminal *tm, struct grid_state *state,
                    char *help_msg);
void on_computer_move(struct terminal *tm, struct grid_state *state);
int read_special_key();
int read_player_input(struct grid_state *state);

#endif // _RENDERER_H
