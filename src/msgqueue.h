/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Piotr Domagalski <szalik@szalik.net>
 *                          Wojtek Kaniewski <wojtekka@irc.pl>
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

#ifndef __MSGQUEUE_H
#define __MSGQUEUE_H

#include <time.h>
#include <sys/types.h>
#include "config.h"
#include "libgadu.h"
#include "dynstuff.h"

struct msg_queue {
	int msg_class;
	int msg_seq;
	int uin_count;
	uin_t *uins;
	int secure;
	time_t time;
	char *msg;
};

list_t msg_queue;

int msg_queue_add(int msg_class, int msg_seq, int uin_count, uin_t *uins, const char *msg, int secure);
int msg_queue_remove(int msg_seq);
int msg_queue_remove_uin(uin_t uin);
void msg_queue_free();
int msg_queue_flush();
int msg_queue_count();
int msg_queue_count_uin(uin_t uin);
int msg_queue_read();
int msg_queue_write();

#endif /* __MSGQUEUE_H */