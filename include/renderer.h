#pragma once

#ifndef _RENDERER_H
#define _RENDERER_H

#include <game.h>
#include <terminal.h>

#define GRID_HEIGHT 7
#define GRID_WIDTH 13
#define MAX_INFO_MSG_LEN 32
#define MAX_HELP_MSG_LEN 32
#define AFTER_GAME_MENU_HEIGHT 9
#define AFTER_GAME_MENU_WIDTH 32

struct after_game {
  unsigned short x;
  unsigned short y;
};
enum frame_verdict {
  ALLOW,
  SKIP,
};
enum app_state {
  IN_GAME,
  AFTER_GAME,
};
struct top_frame {
  enum app_state discriminant;
  union {
    struct game_state *game;
    struct after_game *after_menu;
  };
};
int update_active_screen_info(struct terminal *tm, struct top_frame *tf);
bool is_size_sufficient(struct terminal *tm, struct top_frame *tf);

void display_in_grid_position(struct terminal *tm, struct game_state *state);
void print_after_game_menu(struct after_game *menu);
void print_grid(struct grid_state *state);
void print_info_msg(struct terminal *tm, struct game_state *gs, char *info_msg);
void print_help_msg(struct terminal *tm, struct grid_state *state,
                    char *help_msg);
int read_special_key();
int read_player_input(struct grid_state *state);

#endif // _RENDERER_H
