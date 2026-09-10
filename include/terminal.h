#pragma once

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include <termios.h>

extern struct termios terminal_state;
int begin_session();
void end_session();

bool is_resized();
bool is_exit_requested();
void trigger_redraw(int sig);
void trigger_termination(int sig);
void setup_resize_signal();
void setup_termination_signal();
int setup_terminal();

// ansi codes
void enter_alternate_sb();
void exit_alternate_sb();
void cursor_solid_bar();
void restore_cursor_style();
void hide_cursor();
void show_cursor();
void clear_terminal();

struct terminal {
  unsigned short height;
  unsigned short width;
};

void get_size(struct terminal *tm);

typedef int (*resize_handler_t)(struct terminal *tm, void *data);
int on_resize(struct terminal *tm, resize_handler_t, void *data);

#endif // _TERMINAL_h
