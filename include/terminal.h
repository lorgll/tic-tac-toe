#pragma once

#ifndef _TERMINAL_H
#define _TERMINAL_H

struct terminal {
  unsigned short height;
  unsigned short width;
};

typedef int (*resize_handler_t)(struct terminal *tm, void *data);

int setup_terminal();
void enter_alternate_sb();
void exit_alternate_sb();
void cursor_solid_bar();
void restore_cursor_style();
void hide_cursor();
void show_cursor();
void clear_terminal();

bool is_exit_requested();
bool is_resized();

void trigger_redraw(int sig);
void trigger_termination(int sig);
void get_size(struct terminal *tm);
int on_resize(struct terminal *tm, resize_handler_t, void *data);
bool special_os_resize_processor(struct terminal *tm);

#endif // _TERMINAL_H
