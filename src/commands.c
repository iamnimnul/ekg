/* $Id$ */

/*
 *  (C) Copyright 2001 Wojtek Kaniewski <wojtekka@irc.pl>
 *		        Robert J. Wo�ny <speedy@ziew.org>
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

#include <stdio.h>
#include <unistd.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "libgg.h"
#include "stuff.h"
#include "dynstuff.h"
#include "commands.h"
#include "events.h"
#include "themes.h"
#include "vars.h"

/*
 * g�upie readline z wersji na wersj� ma inne include'y, grr.
 */
extern void rl_extend_line_buffer(int len);
extern char **completion_matches();

/*
 * jaka� malutka lista tych, do kt�rych by�y wysy�ane wiadomo�ci.
 */

#define SEND_NICKS_MAX 100

char *send_nicks[SEND_NICKS_MAX] = { NULL };
int send_nicks_count = 0, send_nicks_index = 0;

/* 
 * definicje dost�pnych komend.
 */

int command_add(), command_away(), command_del(), command_alias(),
	command_exec(), command_help(), command_ignore(), command_list(),
	command_save(), command_msg(), command_quit(), command_test_send(),
	command_test_add(), command_theme(), command_set(), command_connect(),
	command_sms(), command_find(), command_modify(), command_cleartab(),
	command_status(), command_register(), command_test_watches(),
	command_remind(), command_dcc(), command_query(), command_passwd(),
	command_test_ping();

/*
 * drugi parametr definiuje ilo�� oraz rodzaje parametr�w (tym samym
 * ich dope�nie�)
 *
 * '?' - olewamy,
 * 'U' - r�cznie wpisany uin, nadawca mesg�w,
 * 'u' - nazwa lub uin z kontakt�w, r�cznie wpisany uin, nadawca mesg�w,
 * 'c' - komenda,
 * 'i' - nicki z listy ignorowanych os�b,
 * 'd' - komenda dcc,
 * 'f' - plik.
 */

struct command commands[] = {
	{ "add", "U??", command_add, " <numer> <alias> [opcje]", "Dodaje u�ytkownika do listy kontakt�w", "Opcje identyczne jak dla polecenia %Wmodify%n" },
	{ "alias", "??", command_alias, " [opcje]", "Zarz�dzanie aliasami", "  --add <alias> <komenda>\n  --del <alias>\n  [--list]\n" },
	{ "away", "", command_away, "", "Zmienia stan na zaj�ty", "" },
	{ "back", "", command_away, "", "Zmienia stan na dost�pny", "" },
	{ "chat", "u?", command_msg, " <numer/alias> <wiadomo��>", "Wysy�a wiadomo�� w ramach rozmowy", "" },
	{ "cleartab", "", command_cleartab, "", "Czy�ci list� nick�w do dope�nienia", "" },
	{ "connect", "", command_connect, "", "��czy si� z serwerem", "" },
	{ "dcc", "duf", command_dcc, " [opcje]", "Obs�uga bezpo�rednich po��cze�", "  send <numer/alias> <�cie�ka>\n  get [numer/alias]\n  close [numer/alias/id]\n  show\n" },
	{ "del", "u", command_del, " <numer/alias>", "Usuwa u�ytkownika z listy kontakt�w", "" },
	{ "disconnect", "", command_connect, "", "Roz��cza si� z serwerem", "" },
	{ "exec", "?", command_exec, " <polecenie>", "Uruchamia polecenie systemowe", "" },
	{ "!", "?", command_exec, " <polecenie>", "Synonim dla %Wexec%n", "" },
	{ "find", "u", command_find, " [opcje]", "Interfejs do katalogu publicznego", "  --uin <numerek>\n  --first <imi�>\n  --last <nazwisko>\n  --nick <pseudonim>\n  --city <miasto>\n  --birth <min:max>\n  --phone <telefon>\n  --email <e-mail>\n  --active\n  --female\n  --male\n" },
	{ "info", "u", command_find, " <numer/alias>", "Interfejs do katalogu publicznego", "" },
	{ "help", "c", command_help, " [polecenie]", "Wy�wietla informacj� o poleceniach", "" },
	{ "?", "c", command_help, " [polecenie]", "Synonim dla %Whelp%n", "" },
	{ "ignore", "u", command_ignore, " [numer/alias]", "Dodaje do listy ignorowanych lub j� wy�wietla", "" },
	{ "invisible", "", command_away, "", "Zmienia stan na niewidoczny", "" },
	{ "list", "", command_list, " [opcje]", "Wy�wietla list� kontakt�w", "  --active\n  --busy\n  --inactive\n" },
	{ "msg", "u?", command_msg, " <numer/alias> <wiadomo��>", "Wysy�a wiadomo�� do podanego u�ytkownika", "" },
	{ "modify", "u?", command_modify, " <alias> [opcje]", "Zmienia informacje w li�cie kontakt�w", "  --first <imi�>\n  --last <nazwisko>\n  --nick <pseudonim>  // tylko informacja\n  --alias <alias>  // nazwa w li�cie kontakt�w\n  --phone <telefon>\n  --uin <numerek>\n" },
	{ "passwd", "??", command_passwd, " <has�o> <e-mail>", "Zmienia has�o i adres e-mail u�ytkownika", "" },
	{ "private", "", command_away, " [on/off]", "W��cza/wy��cza tryb ,,tylko dla przyjaci�''", "" },
	{ "query", "u", command_query, " <numer/alias>", "W��cza rozmow� z dan� osob�", "" },
	{ "register", "??", command_register, " <email> <has�o>", "Rejestruje nowy uin", "" },
	{ "remind", "", command_remind, "", "Wysy�a has�o na skrzynk� pocztow�", "" },
	{ "save", "", command_save, "", "Zapisuje ustawienia programu", "" },
	{ "set", "v?", command_set, " <zmienna> <warto��>", "Wy�wietla lub zmienia ustawienia", "" },
	{ "sms", "u?", command_sms, " <numer/alias> <tre��>", "Wysy�a SMSa do podanej osoby", "" },
	{ "status", "", command_status, "", "Wy�wietla aktualny stan", "" },
	{ "theme", "f", command_theme, " <plik>", "�aduje opis wygl�du z podanego pliku", "" },
	{ "quit", "", command_quit, "", "Wychodzi z programu", "" },
	{ "unignore", "i", command_ignore, " <numer/alias>", "Usuwa z listy ignorowanych os�b", "" },
	{ "_msg", "u?", command_test_send, "", "", "" },
	{ "_add", "?", command_test_add, "", "", "" },
	{ "_watches", "", command_test_watches, "", "", "" },
	{ "_ping", "", command_test_ping, "", "", "" },
	{ NULL, NULL, NULL, NULL, NULL }
};

char *command_generator(char *text, int state)
{
	static int index = 0, len;
	char *name;

	if (*text == '/')
		text++;

	if (!*rl_line_buffer) {
		if (state)
			return NULL;
		if (!send_nicks_count)
			return strdup("msg");
		send_nicks_index = (send_nicks_count > 1) ? 1 : 0;
		if (!(name = malloc(6 + strlen(send_nicks[0]))))
			return NULL;
		sprintf(name, "chat %s", send_nicks[0]);
		
		return name;
	}

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while ((name = commands[index++].name))
		if (!strncasecmp(text, name, len))
			return strdup(name);

	return NULL;
}

char *known_uin_generator(char *text, int state)
{
	static struct list *l;
	static int len;

	if (!state) {
		l = userlist;
		len = strlen(text);
	}

	while (l) {
		struct userlist *u = l->data;

		l = l->next;

		if (!strncasecmp(text, u->comment, len))
			return strdup(u->comment);
	}

	return NULL;
}

char *unknown_uin_generator(char *text, int state)
{
	static int index = 0, len;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while (index < send_nicks_count)
		if (isdigit(send_nicks[index++][0]))
			if (!strncasecmp(text, send_nicks[index - 1], len))
				return strdup(send_nicks[index - 1]);

	return NULL;
}

char *variable_generator(char *text, int state)
{
	static int index = 0, len;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while (variables[index++].name)
		if (!strncasecmp(text, variables[index - 1].name, len))
			return strdup(variables[index - 1].name);

	return NULL;
}

char *ignored_uin_generator(char *text, int state)
{
	static struct list *l;
	static int len;
	struct userlist *u;

	if (!state) {
		l = ignored;
		len = strlen(text);
	}

	while (l) {
		struct ignored *i = l->data;

		l = l->next;

		if (!(u = find_user(i->uin, NULL))) {
			if (!strncasecmp(text, itoa(i->uin), len))
				return strdup(itoa(i->uin));
		} else {
			if (!strncasecmp(text, u->comment, len))
				return strdup(u->comment);
		}
	}

	return NULL;
}

char *dcc_generator(char *text, int state)
{
	char *commands[] = { "close", "get", "send", "show", NULL };
	static int len, i;

	if (!state) {
		i = 0;
		len = strlen(text);
	}

	while (commands[i]) {
		if (!strncasecmp(text, commands[i], len))
			return strdup(commands[i++]);
		i++;
	}

	return NULL;
}

char *empty_generator(char *text, int state)
{
	return NULL;
}

char **my_completion(char *text, int start, int end)
{
	struct command *c;
	char *params = NULL;
	int word = 0, i, abbrs = 0;
	CPFunction *func = empty_generator;

	if (start) {
		if (!strncasecmp(rl_line_buffer, "chat ", 5)) {
			word = 0;
			for (i = 0; i < strlen(rl_line_buffer); i++) {
				if (isspace(rl_line_buffer[i]))
					word++;
			}
			if (word == 2) {
				char buf[100];
				
				snprintf(buf, sizeof(buf), "chat %s ", send_nicks[send_nicks_index++]);
				rl_extend_line_buffer(strlen(buf));
				strcpy(rl_line_buffer, buf);
				rl_end = strlen(buf);
				rl_point = rl_end;
				rl_redisplay();

				if (send_nicks_index == send_nicks_count)
					send_nicks_index = 0;
					
				return NULL;
			}
			word = 0;
		}
	}

	if (start) {
		for (i = 1; i <= start; i++) {
			if (isspace(rl_line_buffer[i]) && !isspace(rl_line_buffer[i - 1]))
				word++;
		}
		word--;

		for (c = commands; c->name; c++) {
			int len = strlen(c->name);

			if (!strncasecmp(rl_line_buffer, c->name, len) && isspace(rl_line_buffer[len])) {
				params = c->params;
				abbrs = 1;
				break;
			}
			
			for (len = 0; rl_line_buffer[len] && rl_line_buffer[len] != ' '; len++);

			if (!strncasecmp(rl_line_buffer, c->name, len)) {
				params = c->params;
				abbrs++;
			} else
				if (params && abbrs == 1)
					break;
		}

		if (params && abbrs == 1) {
			if (word >= strlen(params))
				func = empty_generator;
			else {
				switch (params[word]) {
					case 'u':
						func = known_uin_generator;
	    					break;
					case 'U':
						func = unknown_uin_generator;
						break;
					case 'c':
						func = command_generator;
						break;
					case 's':	/* XXX */
						func = empty_generator;
						break;
					case 'i':
						func = ignored_uin_generator;
						break;
					case 'v':
						func = variable_generator;
						break;
					case 'd':
						func = dcc_generator;
						break;
					case 'f':
#ifdef HAS_RL_COMPLETION
						func = rl_filename_completion_function;
#endif
						break;
				}
			}
		}
	}

	if (start == 0)
		func = command_generator;

	return completion_matches(text, func);
}

void add_send_nick(char *nick)
{
	int i, count = send_nicks_count, dont_add = 0;

	for (i = 0; i < send_nicks_count; i++)
		if (!strcmp(nick, send_nicks[i])) {
			count = i;
			dont_add = 1;
			break;
		}
	
	if (count == SEND_NICKS_MAX) {
		free(send_nicks[SEND_NICKS_MAX - 1]);
		count--;
	}

	for (i = count; i > 0; i--)
		send_nicks[i] = send_nicks[i - 1];

	if (send_nicks_count != SEND_NICKS_MAX && !dont_add)
		send_nicks_count++;
	
	send_nicks[0] = strdup(nick);	
}

COMMAND(command_cleartab)
{
	int i;

	for (i = 0; i < send_nicks_count; i++) {
		free(send_nicks[i]);
		send_nicks[i] = NULL;
	}
	send_nicks_count = 0;
	send_nicks_index = 0;

	return 0;
}

COMMAND(command_add)
{
	uin_t uin;

	if (!params[0] || !params[1]) {
		my_printf("not_enough_params");
		return 0;
	}

	if (find_user(atoi(params[0]), params[1])) {
		my_printf("user_exists", params[1]);
		return 0;
	}

	if (!(uin = atoi(params[0]))) {
		my_printf("invalid_uin");
		return 0;
	}

	if (!add_user(uin, params[1])) {
		my_printf("user_added", params[1]);
		gg_add_notify(sess, uin);
		config_changed = 1;
	} else
		my_printf("error_adding");

	if (params[2]) {
		params++;
		command_modify("add", params);
	}

	return 0;
}

COMMAND(command_alias)
{
	if (params[0] && params[0][0] == '-' && params[0][1] == '-')
		params[0]++;

	if (!params[0] || !strncasecmp(params[0], "-l", 2)) {
		struct list *l;
		int count = 0;

		for (l = aliases; l; l = l->next) {
			struct alias *a = l->data;

			my_printf("aliases_list", a->alias, a->cmd);
			count++;
		}

		if (!count)
			my_printf("aliases_list_empty");

		return 0;
	}

	if (!strncasecmp(params[0], "-a", 2)) {
		if (!add_alias(params[1], 0))
			config_changed = 1;

		return 0;
	}

	if (!strncasecmp(params[0], "-d", 2)) {
		if (!del_alias(params[1]))
			config_changed = 1;

		return 0;
	}

	my_printf("aliases_invalid");
	return 0;
}

COMMAND(command_away)
{
	int status_table[3] = { GG_STATUS_AVAIL, GG_STATUS_BUSY, GG_STATUS_INVISIBLE };	unidle();
	
	if (!strcasecmp(name, "away")) {
		away = 1;
		my_printf("away");
	} else if (!strcasecmp(name, "invisible")) {
		away = 2;
		my_printf("invisible");
	} else if (!strcasecmp(name, "back")) {
		away = 0;
		my_printf("back");
	} else {
		int tmp;

		if (!params[0]) {
			my_printf((private_mode) ? "private_mode_is_on" : "private_mode_is_off");
			return 0;
		}
		
		if ((tmp = on_off(params[0])) == -1) {
			my_printf("private_mode_invalid");
			return 0;
		}

		private_mode = tmp;
		my_printf((private_mode) ? "private_mode_on" : "private_mode_off");
	}

	default_status = status_table[away] | ((private_mode) ? GG_STATUS_FRIENDS_MASK : 0);

	if (sess && sess->state == GG_STATE_CONNECTED)
		gg_change_status(sess, default_status);

	return 0;
}

COMMAND(command_status)
{
	char *av, *bs, *na, *in, *pr, *np;

	av = format_string(find_format("show_status_avail"));
	bs = format_string(find_format("show_status_busy"));
	na = format_string(find_format("show_status_not_avail"));
	in = format_string(find_format("show_status_invisible"));
	pr = format_string(find_format("show_status_private_on"));
	np = format_string(find_format("show_status_private_off"));

	if (!sess || sess->state != GG_STATE_CONNECTED) {
		my_printf("show_status", na, "");
	} else {
		char *foo[3] = { av, bs, in };

		my_printf("show_status", foo[away], (private_mode) ? pr : np);
	}

	free(av);
	free(bs);
	free(na);
	free(in);
	free(pr);
	free(np);

	return 0;
}

COMMAND(command_connect)
{
	if (!strcasecmp(name, "connect")) {
		if (sess) {
			my_printf((sess->state == GG_STATE_CONNECTED) ? "already_connected" : "during_connect");
			return 0;
		}
                if (config_uin && config_password) {
			my_printf("connecting");
			connecting = 1;
			prepare_connect();
			if (!(sess = gg_login(config_uin, config_password, 1))) {
	                        my_printf("conn_failed", strerror(errno));
	                        do_reconnect();
	                } else {
				sess->initial_status = default_status;
			}
			list_add(&watches, sess, 0);
		} else
			my_printf("no_config");
	} else if (sess) {
		connecting = 0;
		if (sess->state == GG_STATE_CONNECTED)
			my_printf("disconnected");
		else if (sess->state != GG_STATE_IDLE)
			my_printf("conn_stopped");
		gg_logoff(sess);
		list_remove(&watches, sess, 0);
		gg_free_session(sess);
		sess = NULL;
		reconnect_timer = 0;
	}

	return 0;
}

COMMAND(command_del)
{
	uin_t uin;
	char *tmp;

	if (!params[0]) {
		my_printf("not_enough_params");
		return 0;
	}

	if (!(uin = get_uin(params[0])) || !find_user(uin, NULL)) {
		my_printf("user_not_found", params[0]);
		return 0;
	}

	tmp = format_user(uin);

	if (!del_user(uin)) {
		my_printf("user_deleted", tmp);
		gg_remove_notify(sess, uin);
	} else
		my_printf("error_deleting");

	return 0;
}

COMMAND(command_exec)
{
	struct list *l;
	int pid;

	if (params[0]) {
		if (!(pid = fork())) {
			execl("/bin/sh", "sh", "-c", params[0], NULL);
			exit(1);
		}
		add_process(pid, params[0]);
	} else {
		for (l = children; l; l = l->next) {
			struct process *p = l->data;

			my_printf("process", itoa(p->pid), p->name);
		}
		if (!children)
			my_printf("no_processes");
	}

	return 0;
}

COMMAND(command_find)
{
	struct gg_search_request r;
	struct gg_http *h;
	struct list *l;
	char **argv, *query = NULL;
	int i, id = 1;

	/* wybieramy sobie identyfikator sercza */
	for (l = watches; l; l = l->next) {
		struct gg_http *h = l->data;

		if (h->type != GG_SESSION_SEARCH)
			continue;

		if (h->id / 2 >= id)
			id = h->id / 2 + 1;
	}
	
	if (params[0])
		query = strdup(params[0]);
	
	memset(&r, 0, sizeof(r));

	if (!strcasecmp(name, "info") && !params[0]) {
		r.uin = config_uin;
		if (!(h = gg_search(&r, 1))) {
			my_printf("search_failed", strerror(errno));
			free(query);
			return 0;
		}
		h->id = id * 2;
		h->user_data = strdup("self");
		list_add(&watches, h, 0);
		free(query);
		return 0;
	};
	
	if (!params[0] || !(argv = split_params(params[0], -1)) || !argv[0]) {
		my_printf("not_enough_params");
		free(query);
		return 0;
	}

/*	XXX przerobi� to na watches 
	
	if (!strncasecmp(argv[0], "--s", 3) || !strncasecmp(argv[0], "-s", 2)) {
		if (search)
			gg_http_stop(search);
		gg_free_search(search);
		search = NULL;
		my_printf("search_stopped");
		return 0;
	}
*/
	if (!argv[1]) {
		if (!(r.uin = get_uin(params[0]))) {
			my_printf("user_not_found", params[0]);
			free(query);
			return 0;
		}
	} else {
		for (i = 0; argv[i]; i++) {
			if (argv[i][0] == '-' && argv[i][1] == '-')
				argv[i]++;
			if (!strncmp(argv[i], "-f", 2) && argv[i][2] != 'e' && argv[i + 1])
				r.first_name = argv[++i];
			if (!strncmp(argv[i], "-l", 2) && argv[i + 1])
				r.last_name = argv[++i];
			if (!strncmp(argv[i], "-n", 2) && argv[i + 1])
				r.nickname = argv[++i];
			if (!strncmp(argv[i], "-c", 2) && argv[i + 1])
				r.city = argv[++i];
			if (!strncmp(argv[i], "-p", 2) && argv[i + 1])
				r.phone = argv[++i];
			if (!strncmp(argv[i], "-e", 2) && argv[i + 1])
				r.email = argv[++i];
			if (!strncmp(argv[i], "-u", 2) && argv[i + 1])
				r.uin = strtol(argv[++i], NULL, 0);
			if (!strncmp(argv[i], "-fe", 3))
				r.gender = GG_GENDER_FEMALE;
			if (!strncmp(argv[i], "-m", 2))
				r.gender = GG_GENDER_MALE;
			if (!strncmp(argv[i], "-a", 2))
				r.active = 1;
			if (!strncmp(argv[i], "-b", 2) && argv[i + 1]) {
				char *foo = strchr(argv[++i], ':');
	
				if (!foo) {
					r.min_birth = atoi(argv[i]);
					r.max_birth = atoi(argv[i]);
				} else {
					*foo = 0;
					r.min_birth = atoi(argv[i]);
					r.max_birth = atoi(++foo);
				}
				if (r.min_birth < 100)
					r.min_birth += 1900;
				if (r.max_birth < 100)
					r.max_birth += 1900;
			}
		}
	}

	if (!(h = gg_search(&r, 1))) {
		my_printf("search_failed", strerror(errno));
		free(query);
		return 0;
	}

	h->id = id * 2 + ((argv[1]) ? 1 : 0);
	h->user_data = query;

	list_add(&watches, h, 0);
	
	return 0;
}

COMMAND(command_modify)
{
	struct userlist *u;
	char **argv;
	uin_t uin;
	int i;

	if (!params[0]) {
		my_printf("not_enough_params");
		return 0;
	}

	if (!(uin = get_uin(params[0])) || !(u = find_user(uin, NULL))) {
		my_printf("user_not_found", params[0]);
		return 0;
	}

	if (!params[1]) {
		my_printf("user_info", u->first_name, u->last_name, u->nickname, u->comment, u->mobile, u->group);
		return 0;
	} else {
		argv = split_params(params[1], -1);
	}

	for (i = 0; argv[i]; i++) {
		if (argv[i][0] == '-' && argv[i][1] == '-')
			argv[i]++;
		if (!strncmp(argv[i], "-f", 2) && argv[i + 1]) {
			free(u->first_name);
			u->first_name = strdup(argv[++i]);
		}
		if (!strncmp(argv[i], "-l", 2) && argv[i + 1]) {
			free(u->last_name);
			u->last_name = strdup(argv[++i]);
		}
		if (!strncmp(argv[i], "-n", 2) && argv[i + 1]) {
			free(u->nickname);
			u->nickname = strdup(argv[++i]);
		}
		if (!strncmp(argv[i], "-a", 2) && argv[i + 1]) {
			free(u->comment);
			u->comment = strdup(argv[++i]);
			replace_user(u);
		}
		if ((!strncmp(argv[i], "-p", 2) || !strncmp(argv[i], "-m", 2) || !strncmp(argv[i], "-s", 2)) && argv[i + 1]) {
			free(u->mobile);
			u->mobile = strdup(argv[++i]);
		}
		if (!strncmp(argv[i], "-g", 2) && argv[i + 1]) {
			free(u->group);
			u->group = strdup(argv[++i]);
		}
		if (!strncmp(argv[i], "-u", 2) && argv[i + 1])
			u->uin = strtol(argv[++i], NULL, 0);
	}

	if (strcasecmp(name, "add"))
		my_printf("modify_done", params[0]);

	config_changed = 1;

	return 0;
}

COMMAND(command_help)
{
	struct command *c;
	
	if (params[0]) {
		for (c = commands; c->name; c++)
			if (!strcasecmp(c->name, params[0])) {
				my_printf("help", c->name, c->params_help, c->brief_help);
				if (c->long_help && strcmp(c->long_help, "")) {
					char *foo, *tmp, *plumk, *bar = strdup(c->long_help);

					if ((foo = bar)) {
						while ((tmp = gg_get_line(&foo))) {
							plumk = format_string(tmp);
							if (plumk) {
								my_printf("help_more", plumk);
								free(plumk);
							}
						}
						free(bar);
					}
				}

				return 0;
			}
	}

	for (c = commands; c->name; c++)
		if (isalnum(*c->name))
			my_printf("help", c->name, c->params_help, c->brief_help);

	return 0;
}

COMMAND(command_ignore)
{
	uin_t uin;

	if (*name == 'i' || *name == 'I') {
		if (!params[0]) {
			struct list *l;

			for (l = ignored; l; l = l->next) {
				struct ignored *i = l->data;

				my_printf("ignored_list", format_user(i->uin));
			}

			if (!ignored) 
				my_printf("ignored_list_empty");

			return 0;
		}
		
		if (!(uin = get_uin(params[0]))) {
			my_printf("user_not_found", params[0]);
			return 0;
		}
		
		if (!add_ignored(uin)) {
			if (!in_autoexec) 
				my_printf("ignored_added", params[0]);
		} else
			my_printf("error_adding_ignored");

	} else {
		if (!params[0]) {
			my_printf("not_enough_params");
			return 0;
		}
		
		if (!(uin = get_uin(params[0]))) {
			my_printf("user_not_found", params[0]);
			return 0;
		}
		
		if (!del_ignored(uin)) {
			if (!in_autoexec)
				my_printf("ignored_deleted", format_user(uin));
		} else
			my_printf("error_not_ignored", format_user(uin));
	
	}
	
	return 0;
}

COMMAND(command_list)
{
	struct list *l;
	int count = 0, show_all = 1, show_busy = 0, show_active = 0, show_inactive = 0;
	char *tmp, **argv;

        if (params[0] && (argv = split_params(params[0], -1))) {
		int i;

		for (i = 0; argv[i]; i++) {
			if (argv[i][0] == '-' && argv[i][1] == '-')
				argv[i]++;
			if (argv[i][0] == '-' && argv[i][1] == 'a') {
				show_all = 0;
				show_active = 1;
			}
			if (argv[i][0] == '-' && (argv[i][1] == 'i' || argv[i][1] == 'u' || argv[i][1] == 'n')) {
				show_all = 0;
				show_inactive = 1;
			}
			if (argv[i][0] == '-' && argv[i][1] == 'b') {
				show_all = 0;
				show_busy = 1;
			}
		}
	}

	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;
		struct in_addr in;

		tmp = "list_unknown";
		switch (u->status) {
			case GG_STATUS_AVAIL:
				tmp = "list_avail";
				break;
			case GG_STATUS_NOT_AVAIL:
				tmp = "list_not_avail";
				break;
			case GG_STATUS_BUSY:
				tmp = "list_busy";
				break;
		}

		in.s_addr = u->ip;

		if (show_all || (show_busy && u->status == GG_STATUS_BUSY) || (show_active && u->status == GG_STATUS_AVAIL) || (show_inactive && u->status == GG_STATUS_NOT_AVAIL)) {
			my_printf(tmp, format_user(u->uin), inet_ntoa(in), itoa(u->port));
			count++;
		}
	}

	if (!count && show_all)
		my_printf("list_empty");

	return 0;
}

COMMAND(command_msg)
{
	struct userlist *u;
	char sender[50], *msg = NULL;
	uin_t uin;
	int free_msg = 0;

	if (!sess || sess->state != GG_STATE_CONNECTED) {
		my_printf("not_connected");
		return 0;
	}

	if (!params[0] || !params[1]) {
		my_printf("not_enough_params");
		return 0;
	}
	
	if (!(uin = get_uin(params[0]))) {
		my_printf("user_not_found", params[0]);
		return 0;
	}

        if ((u = find_user(uin, NULL)))
                snprintf(sender, sizeof(sender), "%s/%lu", u->comment, u->uin);
        else
                snprintf(sender, sizeof(sender), "%lu", uin);

	if (!strcmp(params[1], "\\")) {
		struct string *s;
		char *line;

		if (!(s = string_init(NULL)))
			return 0;

		no_prompt = 1;

		while ((line = my_readline())) {
			if (!strcmp(line, ".")) {
				free(line);
				break;
			}
			string_append(s, line);
			string_append(s, "\r\n");
			free(line);
		}

		no_prompt = 0;

		if (!line) {
			printf("\n");
			string_free(s, 1);
			return 0;
		}
		msg = string_free(s, 0);
		free_msg = 1;
	} else
		msg = params[1];

        put_log(uin, "<< %s %s (%s)\n%s\n", (!strcasecmp(name, "chat")) ?
                "Rozmowa do" : "Wiadomo�� do", sender, full_timestamp(),
                msg);

	add_send_nick(params[0]);
	iso_to_cp(msg);
	gg_send_message(sess, (!strcasecmp(name, "msg")) ? GG_CLASS_MSG : GG_CLASS_CHAT, uin, msg);

	if (free_msg)
		free(msg);

	unidle();

	return 0;
}

COMMAND(command_save)
{
	if (!write_userlist(NULL) && !write_config(NULL)) {
		my_printf("saved");
		config_changed = 0;
	} else
		my_printf("error_saving");

	return 0;
}

COMMAND(command_theme)
{
	if (!params[0]) {
		my_printf("not_enough_params");
		return 0;
	}
	
	if (!strcmp(params[0], "-")) {
		init_theme();
		reset_theme_cache();
		if (!in_autoexec)
			my_printf("theme_default");
	} else {
		if (!read_theme(params[0], 1)) {
			reset_theme_cache();
			if (!in_autoexec)
				my_printf("theme_loaded", params[0]);
		} else
			my_printf("error_loading_theme", strerror(errno));
	}
	
	return 0;
}

COMMAND(command_set)
{
	struct variable *v = variables;
	int unset = 0;

	if (params[0] && params[0][0] == '-') {
		unset = 1;
		params[0]++;
	}

	if (!params[1] && !unset) {
		while (v->name) {
			if ((!params[0] || !strcasecmp(params[0], v->name)) && v->display != 2) {
				if (v->type == VAR_STR) {
					char *tmp = *(char**)(v->ptr);
					
					if (!tmp)
						tmp = "(brak)";
					if (!v->display)
						tmp = "(...)";
					my_printf("variable", v->name, tmp);
				} else {
					my_printf("variable", v->name, (!v->display) ? "(...)" : itoa(*(int*)(v->ptr)));
				}
			}

			v++;
		}
	} else {
		reset_theme_cache();
		switch (set_variable(params[0], params[1])) {
			case 0:
				if (!in_autoexec) {
					my_printf("variable", params[0], params[1]);
					config_changed = 1;
				}
				break;
			case -1:
				my_printf("variable_not_found", params[0]);
				break;
			case -2:
				my_printf("variable_invalid", params[0]);
				break;
		}
	}

	return 0;
}

COMMAND(command_sms)
{
	struct userlist *u;
	char *number = NULL;

	if (!params[1]) {
		my_printf("not_enough_params");
		return 0;
	}

	if ((u = find_user(0, params[0]))) {
		if (!u->mobile || !strcmp(u->mobile, "")) {
			my_printf("sms_unknown", format_user(u->uin));
			return 0;
		}
		number = u->mobile;
	} else
		number = params[0];

	if (send_sms(number, params[1], 1) == -1)
		my_printf("sms_error", strerror(errno));

	return 0;
}

COMMAND(command_quit)
{
	my_printf("quit");
	gg_logoff(sess);
	list_remove(&watches, sess, 0);
	gg_free_session(sess);
	sess = NULL;
	return -1;
}

COMMAND(command_dcc)
{
	struct transfer t;
	struct list *l;
	uin_t uin;

	if (!params[0])
		params[0] = "show";

	if (!strncasecmp(params[0], "sh", 2)) {		/* show */
		int pending = 0, active = 0;

		if (params[1] && params[1][0] == 'd') {	/* show debug */
			for (l = transfers; l; l = l->next) {
				struct transfer *t = l->data;
				
				my_printf("dcc_show_debug", itoa(t->id), (t->type == GG_SESSION_DCC_SEND) ? "SEND" : "GET", t->filename, format_user(t->uin), (t->dcc) ? "yes" : "no");
			}

			return 0;
		}

		for (l = transfers; l; l = l->next) {
			struct transfer *t = l->data;

			if (!t->dcc || (t->dcc->state != GG_STATE_SENDING_FILE || t->dcc->state != GG_STATE_GETTING_FILE)) {
				if (!pending) {
					my_printf("dcc_show_pending_header");
					pending = 1;
				}
				my_printf((t->type == GG_SESSION_DCC_SEND) ? "dcc_show_pending_send" : "dcc_show_pending_get", itoa(t->id), format_user(t->uin), (t->filename) ? t->filename : "(?)");
			}
		}

		for (l = transfers; l; l = l->next) {
			struct transfer *t = l->data;

			if (t->dcc && (t->dcc->state == GG_STATE_SENDING_FILE || t->dcc->state == GG_STATE_GETTING_FILE)) {
				if (!active) {
					my_printf("dcc_show_active_header");
					active = 1;
				}
				my_printf((t->type == GG_SESSION_DCC_SEND) ? "dcc_show_active_send" : "dcc_show_active_get", itoa(t->id), format_user(t->uin), t->filename);
			}
		}

		if (!active && !pending)
			my_printf("dcc_show_empty");
		
		return 0;
	}
	
	if (!strncasecmp(params[0], "se", 2)) {		/* send */
		struct userlist *u;
		struct stat st;
		int fd;

		if (!params[1] || !params[2]) {
			my_printf("not_enough_params");
			return 0;
		}
		
		if (!(uin = get_uin(params[1])) || !(u = find_user(uin, NULL))) {
			my_printf("user_not_found", params[1]);
			return 0;
		}

		if (!sess || sess->state != GG_STATE_CONNECTED) {
			my_printf("not_connected");
			return 0;
		}

		if ((fd = open(params[2], O_RDONLY)) == -1 || stat(params[2], &st)) {
			my_printf("dcc_open_error", params[2], strerror(errno));
			return 0;
		} else {
			close(fd);
			if (S_ISDIR(st.st_mode)) {
				my_printf("dcc_open_directory", params[2]);
				return 0;
			}
		}

		t.uin = uin;
		t.id = transfer_id();
		t.type = GG_SESSION_DCC_SEND;
		t.filename = strdup(params[2]);
		t.dcc = NULL;

		if (u->port < 10) {
			/* nie mo�emy si� z nim po��czy�, wi�c on spr�buje */
			gg_dcc_request(sess, uin);
		} else {
			struct gg_dcc *d;
			
			if (!(d = gg_dcc_send_file(u->ip, u->port, config_uin, uin))) {
				my_printf("dcc_error", strerror(errno));
				return 0;
			}

			if (gg_dcc_fill_file_info(d, params[2]) == -1) {
				my_printf("dcc_open_error", params[2], strerror(errno));
				gg_free_dcc(d);
				return 0;
			}

			list_add(&watches, d, 0);

			t.dcc = d;
		}

		list_add(&transfers, &t, sizeof(t));

		return 0;
	}

	if (!strncasecmp(params[0], "g", 1)) {		/* get */
		struct transfer *t;
		
		for (t = NULL, l = transfers; l; l = l->next) {
			struct userlist *u;
			
			t = l->data;

			if (!t->dcc || t->type != GG_SESSION_DCC_GET || !t->filename)
				continue;
			
			if (!params[1])
				break;

			if (params[1][0] == '#' && atoi(params[1] + 1) == t->id)
				break;

			if ((u = find_user(t->uin, NULL))) {
				if (!strcasecmp(params[1], itoa(u->uin)) || !strcasecmp(params[1], u->comment))
					break;
			}
		}

		if (!l || !t || !t->dcc) {
			my_printf("dcc_get_not_found", (params[1]) ? params[1] : "");
			return 0;
		}

		/* XXX wi�cej sprawdzania, zrzuca� do jakiego� $dcc_dir */
		if ((t->dcc->file_fd = open(t->filename, O_WRONLY | O_CREAT, 0600)) == -1) {
			my_printf("dcc_get_cant_create", t->filename);
			gg_free_dcc(t->dcc);
			list_remove(&transfers, t, 1);
			
			return 0;
		}
		
		my_printf("dcc_get_getting", format_user(t->uin), t->filename);
		
		list_add(&watches, t->dcc, 0);

		return 0;
	}
	
	if (!strncasecmp(params[0], "c", 1)) {		/* close */
		my_printf("dcc_not_supported", params[0]);
		return 0;
	}

	my_printf("dcc_unknown_command", params[0]);
	
	return 0;
}

COMMAND(command_test_ping)
{
	if (sess)
		gg_ping(sess);

	return 0;
}

COMMAND(command_test_send)
{
	struct gg_event *e = malloc(sizeof(struct gg_event));
	
	e->type = GG_EVENT_MSG;
	e->event.msg.sender = get_uin(params[0]);
	e->event.msg.message = strdup(params[1]);
	
	handle_msg(e);

	return 0;
}

COMMAND(command_test_add)
{
	if (params[0])
		add_send_nick(params[0]);

	return 0;
}

COMMAND(command_test_watches)
{
	struct list *l;
	char buf[200], *type, *state, *check;
	int no = 0;

	for (l = watches; l; l = l->next, no++) {
		struct gg_common *s = l->data;
		
		switch (s->type) {
			case GG_SESSION_GG: type = "GG"; break;
			case GG_SESSION_HTTP: type = "HTTP"; break;
			case GG_SESSION_SEARCH: type = "SEARCH"; break;
			case GG_SESSION_REGISTER: type = "REGISTER"; break;
			case GG_SESSION_REMIND: type = "REMIND"; break;
			case GG_SESSION_CHANGE: type = "CHANGE"; break;
			case GG_SESSION_PASSWD: type = "PASSWD"; break;
			case GG_SESSION_DCC: type = "DCC"; break;
			case GG_SESSION_DCC_SOCKET: type = "DCC_SOCKET"; break;
			case GG_SESSION_DCC_SEND: type = "DCC_SEND"; break;
			case GG_SESSION_DCC_GET: type = "DCC_GET"; break;
			case GG_SESSION_USER0: type = "USER0"; break;
			case GG_SESSION_USER1: type = "USER1"; break;
			case GG_SESSION_USER2: type = "USER2"; break;
			case GG_SESSION_USER3: type = "USER3"; break;
			default: type = "(unknown)"; break;
		}
		switch (s->check) {
			case GG_CHECK_READ: check = "R"; break;
			case GG_CHECK_WRITE: check = "W"; break;
			case GG_CHECK_READ | GG_CHECK_WRITE: check = "RW"; break;
			default: check = "?"; break;
		}
		switch (s->state) {
			/* gg_common */
			case GG_STATE_IDLE: state = "IDLE"; break;
			case GG_STATE_RESOLVING: state = "RESOLVING"; break;
			case GG_STATE_CONNECTING: state = "CONNECTING"; break;
			case GG_STATE_READING_DATA: state = "READING_DATA"; break;
			case GG_STATE_ERROR: state = "ERROR"; break;
			/* gg_session */	     
			case GG_STATE_CONNECTING_GG: state = "CONNECTING_GG"; break;
			case GG_STATE_READING_KEY: state = "READING_KEY"; break;
			case GG_STATE_READING_REPLY: state = "READING_REPLY"; break;
			case GG_STATE_CONNECTED: state = "CONNECTED"; break;
			/* gg_http */
			case GG_STATE_READING_HEADER: state = "READING_HEADER"; break;
			case GG_STATE_PARSING: state = "PARSING"; break;
			case GG_STATE_DONE: state = "DONE"; break;
			/* gg_dcc */
			case GG_STATE_LISTENING: state = "LISTENING"; break;
			case GG_STATE_READING_UIN_1: state = "READING_UIN_1"; break;
			case GG_STATE_READING_UIN_2: state = "READING_UIN_2"; break;
			case GG_STATE_SENDING_ACK: state = "SENDING_ACK"; break;
			case GG_STATE_SENDING_REQUEST: state = "SENDING_REQUEST"; break;
			case GG_STATE_READING_REQUEST: state = "READING_REQUEST"; break;
			case GG_STATE_SENDING_FILE_INFO: state = "SENDING_FILE_INFO"; break;
			case GG_STATE_READING_ACK: state = "READING_ACK"; break;
			case GG_STATE_READING_FILE_ACK: state = "READING_FILE_ACK"; break;
			case GG_STATE_SENDING_FILE_HEADER: state = "SENDING_FILE_HEADER"; break;
			case GG_STATE_GETTING_FILE: state = "SENDING_GETTING_FILE"; break;
			case GG_STATE_SENDING_FILE: state = "SENDING_SENDING_FILE"; break;
			default: state = "(unknown)"; break;
		}
		
		snprintf(buf, sizeof(buf), "%d: type=%s, fd=%d, state=%s, check=%s, id=%d, timeout=%d", no, type, s->fd, state, check, s->id, s->timeout);
		my_printf("generic", buf);
	}

	return 0;
}

COMMAND(command_register)
{
	struct gg_http *h;
	struct list *l;
	
	if (!params[0] || !params[1]) {
		my_printf("not_enough_params");
		return 0;
	}

	for (l = watches; l; l = l->next) {
		struct gg_common *s = l->data;

		if (s->type == GG_SESSION_REGISTER) {
			my_printf("register_pending");
			return 0;
		}
	}
	
	if (!(h = gg_register(params[0], params[1], 1))) {
		my_printf("register_failed", strerror(errno));
		return 0;
	}

	list_add(&watches, h, 0);

	reg_password = strdup(params[1]);
	
	return 0;
}

COMMAND(command_passwd)
{
	struct gg_http *h;
	
	if (!params[0] || !params[1]) {
		my_printf("not_enough_params");
		return 0;
	}

	if (!(h = gg_change_passwd(config_uin, config_password, params[0], params[1], 1))) {
		my_printf("passwd_failed", strerror(errno));
		return 0;
	}

	list_add(&watches, h, 0);

	reg_password = strdup(params[0]);
	
	return 0;
}

COMMAND(command_remind)
{
	struct gg_http *h;
	
	if (!(h = gg_remind_passwd(config_uin, 1))) {
		my_printf("remind_failed", strerror(errno));
		return 0;
	}

	list_add(&watches, h, 0);
	
	return 0;
}

COMMAND(command_query)
{
	uin_t uin;

	if (!params[0] && !query_nick)
		return 0;

	if (query_nick) {
		free(query_nick);
		query_nick = NULL;
	}

	if (!params[0]) {
		my_printf("query_finished", format_user(query_uin));
		query_uin = 0;
		return 0;
	}

	if (!(uin = get_uin(params[0]))) {
		my_printf("user_not_found", params[0]);
		return 0;
	}

	query_nick = strdup(params[0]);
	query_uin = uin;
	my_printf("query_started", format_user(uin));

	return 0;
}

char *strip_spaces(char *line)
{
	char *buf;
	
	for (buf = line; isspace(*buf); buf++);

	while (isspace(line[strlen(line) - 1]))
		line[strlen(line) - 1] = 0;
	
	return buf;
}

char **split_params(char *line, int count)
{
	char **res;
	char *p = line;
	int i;

	if (count == -1) {
		char *q;

		for (count = 1, q = line; *q; q++)
			if (isspace(*q))
				count++;
	}

	if (!(res = malloc((count + 2) * sizeof(char*))))
		return NULL;

	for (i = 0; i < count + 2; i++)
		res[i] = 0;
	
	if (!strcmp(strip_spaces(line), ""))
		return res;
	
	if (count < 2) {
		res[0] = p;
		return res;
	}
	
	for (i = 0; *p && i < count; i++) {
		while (isspace(*p))
			p++;

		res[i] = p;
		
		while (*p && !isspace(*p))
			p++;

		if (!*p)
			break;

		if (i != count - 1)
			*p++ = 0;

		while (isspace(*p))
			p++;
	}

	return res;
}

int execute_line(char *line)
{
	char *cmd = NULL, *tmp, *p = NULL, short_cmd[2] = ".", *last_name = NULL, *last_params = NULL;
	struct command *c;
	int (*last_abbr)(char *, char **) = NULL;
	int abbrs = 0;

	if (query_nick && *line != '/') {
		char *params[] = { query_nick, line, NULL };

		command_msg("chat", params);
		return 0;
	}
	
	send_nicks_index = 0;

	line = strdup(strip_spaces(line));
	
	if (*line == '/')
		line++;

	for (c = commands; c->name; c++)
		if (!isalpha(c->name[0]) && strlen(c->name) == 1 && line[0] == c->name[0]) {
			short_cmd[0] = c->name[0];
			cmd = short_cmd;
			p = line + 1;
		}

	if (!cmd) {
		tmp = cmd = line;
		while (*tmp && !isspace(*tmp))
			tmp++;
		p = (*tmp) ? tmp + 1 : tmp;
		*tmp = 0;
		p = strip_spaces(p);
	}
		
	for (c = commands; c->name; c++) {
		if (!strcasecmp(c->name, cmd)) {
			last_abbr = c->function;
			last_name = c->name;
			last_params = c->params;
			abbrs = 1;		
			break;
		}
		if (!strncasecmp(c->name, cmd, strlen(cmd))) {
			abbrs++;
			last_abbr = c->function;
			last_name = c->name;
			last_params = c->params;
		} else {
			if (last_abbr && abbrs == 1)
				break;
		}
	}

	if (last_abbr && abbrs == 1) {
		char **par;
		int res;

		c--;
			
		par = split_params(p, strlen(last_params));
		res = (last_abbr)(last_name, par);
		free(par);

		return res;
	}

	if (strcmp(cmd, ""))
		my_printf("unknown_command", cmd);
	
	return 0;
}

