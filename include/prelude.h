#pragma once

#ifndef _PRELUDE_H
#define _PRELUDE_H

#define loop while (!is_exit_requested())

#define SUCCESS 0

#define NOT_A_TERMINAL 1

#define INSUFFICIENT_TERM_SIZE -1
#define READ_INTERRUPTED -2

#define UNKNOWN_KEY 1000
#define KC_LEFT 1001
#define KC_RIGHT 1002
#define KC_UP 1003
#define KC_DOWN 1004
#define KC_ENTER 1005
#define KC_N 2014
#define KC_Y 2025

#define CURSOR_POSITION_CENTER 4

#define FPS_30 33333
#define FPS_60 16666

#endif // _PRELUDE_H
