/* $Id$ */

/*
 *  (C) Copyright 2002-2004 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Adam Wysocki <gophi@ekg.apcoh.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __UI_H
#define __UI_H

#include "config.h"

void (*ui_init)(void);
void (*ui_postinit)(void);
void (*ui_loop)(void);
void (*ui_print)(const char *target, int separate, const char *line);
void (*ui_beep)(void);
int (*ui_event)(const char *event, ...);
void (*ui_deinit)(void);

int ui_screen_width;
int ui_screen_height;

int ui_need_refresh;

extern void ui_none_init();
extern void ui_batch_init();

#ifdef WITH_UI_READLINE

#define MAX_LINES_PER_SCREEN 50

extern void ui_readline_init();
#endif

#ifdef WITH_UI_NCURSES

#ifdef HAVE_NCURSES_H
#  include <ncurses.h>
#else
#  ifdef HAVE_NCURSES_NCURSES_H
#    include <ncurses/ncurses.h>
#  endif
#endif

WINDOW *header, *status;

int window_printat(WINDOW *w, int x, int y, const char *format, void *data_, int fgcolor, int bold, int bgcolor, int status);
int config_backlog_overlap;
int config_backlog_size;
extern void ui_ncurses_init();
extern void header_statusbar_resize();

#endif

#endif /* __UI_H */
