/* $Id$ */

/*
 *  (C) Copyright 2001 Wojtek Kaniewski <wojtekka@irc.pl>
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

#ifndef __STUFF_H
#define __STUFF_H

#include <stdio.h>
#include <time.h>
#include "libgg.h"
#include "dynstuff.h"

struct userlist {
	char *first_name, *last_name, *nickname, *comment, *mobile, *group;
	uin_t uin;
	int status;
	unsigned long ip;
	unsigned short port;
};

struct ignored {
	uin_t uin;
};

struct process {
	int pid;
	char *name;
};

struct alias {
	char *alias;
	char *cmd;
};

struct list *userlist;
struct list *ignored;
struct list *children;
struct list *aliases;
int ignored_count;
int in_readline, away, in_autoexec, auto_reconnect, reconnect_timer;
time_t last_action;
struct gg_session *sess;
int auto_away;
int log;
char *log_path;
int display_color;
int enable_beep, enable_beep_msg, enable_beep_chat, enable_beep_notify;
int config_uin;
char *config_password;
char *config_user;
int sms_away;
char *sms_number;
char *sms_send_app;
int sms_max_length;
struct gg_search *search;
int search_type;
int no_prompt;
int config_changed;
int display_ack;
int completion_notify;
char *bold_font;
int private_mode;
int connecting;

void my_puts(char *format, ...);
char *my_readline();

int read_config(char *filename);
int read_userlist(char *filename);
int write_config(char *filename);
int write_userlist(char *filename);
void clear_userlist(void);
int add_user(uin_t uin, char *comment);
int del_user(uin_t uin);
int replace_user(struct userlist *u);
struct userlist *find_user(uin_t uin, char *comment);
char *format_user(uin_t uin);
uin_t get_uin(char *text);
int add_ignored(uin_t uin);
int del_ignored(uin_t uin);
int is_ignored(uin_t uin);
void cp_to_iso(unsigned char *buf);
void iso_to_cp(unsigned char *buf);
void unidle();
char *timestamp(char *format);
char *prepare_path(char *filename);
void parse_autoexec(char *filename);
void send_userlist();
void do_reconnect();
void put_log(uin_t uin, char *format, ...);
char *full_timestamp();
int send_sms(char *recipient, char *message, int show_result);
char *read_file(FILE *f);
int add_process(int pid, char *name);
int del_process(int pid);
int on_off(char *value);
int add_alias(char *string, int quiet);
int del_alias(char *name);
char *is_alias(char *foo);

#endif
