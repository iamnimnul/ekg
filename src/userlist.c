/* $Id$ */

/*
 *  (C) Copyright 2001-2002 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Robert J. Wo�ny <speedy@ziew.org>
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#ifndef _AIX
#  include <string.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "libgadu.h"
#include "stuff.h"
#include "dynstuff.h"
#include "themes.h"
#include "commands.h"
#include "vars.h"
#include "userlist.h"
#include "xmalloc.h"

list_t userlist = NULL;
list_t ignored = NULL;

/*
 * userlist_compare()
 *
 * funkcja pomocna przy list_add_sorted().
 *
 *  - data1, data2 - dwa wpisy userlisty do por�wnania.
 *
 * zwraca wynik strcasecmp() na nazwach user�w.
 */
static int userlist_compare(void *data1, void *data2)
{
	struct userlist *a = data1, *b = data2;
	
	if (!a || !a->display || !b || !b->display)
		return 0;

	return strcasecmp(a->display, b->display);
}

/*
 * userlist_read()
 *
 * wczytuje list� kontakt�w z pliku ~/.gg/userlist. mo�e ona by� w postaci
 * linii ,,<numerek> <opis>'' lub w postaci eksportu tekstowego listy
 * kontakt�w windzianego klienta.
 */
int userlist_read()
{
	const char *filename;
	char *buf;
	FILE *f;

	if (!(filename = prepare_path("userlist", 0)))
		return -1;
	
	if (!(f = fopen(filename, "r")))
		return -1;

	while ((buf = read_file(f))) {
		struct userlist u;
		char *display;
		
		if (buf[0] == '#') {
			xfree(buf);
			continue;
		}

		if (!strchr(buf, ';')) {
			if (!(display = strchr(buf, ' '))) {
				xfree(buf);
				continue;
			}

			u.uin = strtol(buf, NULL, 0);
		
			if (!u.uin) {
				xfree(buf);
				continue;
			}

			u.first_name = NULL;
			u.last_name = NULL;
			u.nickname = NULL;
			u.display = xstrdup(++display);
			u.mobile = NULL;
			u.groups = NULL;
			u.descr = NULL;

		} else {
			char **entry = array_make(buf, ";", 7, 0, 0);
			int i;
			
			if (!entry[0] || !entry[1] || !entry[2] || !entry[3] || !entry[4] || !entry[5] || !entry[6] || !(u.uin = strtol(entry[6], NULL, 0))) {
				array_free(entry);
				xfree(buf);
				continue;
			}

			for (i = 0; i < 6; i++) {
				if (entry[i] && !strcmp(entry[i], "(null)")) {
					xfree(entry[i]);
					entry[i] = NULL;
				}
			}
			
			u.first_name = xstrdup(entry[0]);
			u.last_name = xstrdup(entry[1]);
			u.nickname = xstrdup(entry[2]);
			u.display = xstrdup(entry[3]);
			u.mobile = xstrdup(entry[4]);
			u.groups = group_init(entry[5]);
			u.descr = NULL;

			array_free(entry);
		}

		xfree(buf);

		u.status = GG_STATUS_NOT_AVAIL;

		list_add_sorted(&userlist, &u, sizeof(u), userlist_compare);
	}
	
	fclose(f);

	return 0;
}

/*
 * userlist_set()
 *
 * ustawia list� kontakt�w na podan�.
 */
int userlist_set(char *contacts, int config)
{
	string_t vars = NULL;
	char *buf;

	userlist_clear();

	if (config)
		vars = string_init(NULL);
	
	/* XXX argh! zmieni� na nie ruszaj�ce ,,contacts'' */
	
	while ((buf = gg_get_line(&contacts))) {
		struct userlist u;
		char *display;
		
		if (buf[0] == '#') {
			continue;
		}

		if (!strncmp(buf, "__config", 8)) {
			char **entry;
			int i;

			if (!config)
				continue;
			
			entry = array_make(buf, ";", 7, 0, 0);
			
			for (i = 1; i < 6; i++)
				string_append(vars, entry[i]);

			array_free(entry);

			continue;
		}

		if (!strchr(buf, ';')) {
			if (!(display = strchr(buf, ' '))) {
				continue;
			}

			u.uin = strtol(buf, NULL, 0);
		
			if (!u.uin) {
				continue;
			}

			u.first_name = NULL;
			u.last_name = NULL;
			u.nickname = NULL;
			u.display = xstrdup(++display);
			u.mobile = NULL;
			u.groups = NULL;
			u.descr = NULL;

		} else {
			char *q, **entry = array_make(buf, ";", 7, 0, 0);
			
			if (!entry[0] || !entry[1] || !entry[2] || !entry[3] || !entry[4] || !entry[5] || !entry[6] || !(u.uin = strtol(entry[6], NULL, 0))) {
				array_free(entry);
				continue;
			}

			u.first_name = xstrdup(entry[0]);
			u.last_name = xstrdup(entry[1]);
			u.nickname = xstrdup(entry[2]);
			u.display = xstrdup(entry[3]);
			for (q = u.display; *q; q++)
				if (*q == ' ')
					*q = '_';
			u.mobile = xstrdup(entry[4]);
			u.groups = group_init(entry[5]);
			u.descr = NULL;

			array_free(entry);
		}

		u.status = GG_STATUS_NOT_AVAIL;

		list_add_sorted(&userlist, &u, sizeof(u), userlist_compare);
	}

	if (config) {
		char *tmp = string_free(vars, 0);
		gg_debug(GG_DEBUG_MISC, "// received ekg variables digest: %s\n", tmp);
		variable_undigest(tmp);	/* XXX kod b��du */
		xfree(tmp);
	}

	return 0;
}

/*
 * userlist_dump()
 *
 * zapisuje list� kontakt�w w postaci tekstowej.
 *
 * zwraca zaalokowany bufor, kt�ry nale�y zwolni�.
 */
char *userlist_dump()
{
	string_t s;
	list_t l;

	if (!(s = string_init(NULL)))
		return NULL;
	
	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;
		char *groups, *line;
		
		groups = group_to_string(u->groups, 1);
		
		line = saprintf("%s;%s;%s;%s;%s;%s;%lu\r\n",
			(u->first_name) ?
			u->first_name : u->display, (u->last_name) ?
			u->last_name : "", (u->nickname) ? u->nickname :
			u->display, u->display, (u->mobile) ? u->mobile :
			"", groups, u->uin);
		
		string_append(s, line);

		xfree(line);
		xfree(groups);
	}	

	return string_free(s, 0);
}

/*
 * userlist_write()
 *
 * zapisuje list� kontakt�w w pliku ~/.gg/userlist
 */
int userlist_write()
{
	const char *filename;
	char *contacts;
	FILE *f;

	if (!(contacts = userlist_dump()))
		return -1;
	
	if (!(filename = prepare_path("userlist", 1))) {
		xfree(contacts);
		return -1;
	}
	
	if (!(f = fopen(filename, "w"))) {
		xfree(contacts);
		return -2;
	}
	fchmod(fileno(f), 0600);
	fputs(contacts, f);
	fclose(f);
	
	xfree(contacts);

	return 0;
}

#ifdef WITH_WAP
/*
 * userlist_write_wap()
 *
 * zapisuje list� kontakt�w w pliku ~/.gg/wapstatus
 */
int userlist_write_wap()
{
	const char *filename;
	list_t l;
	FILE *f;

	if (!(filename = prepare_path("wapstatus", 1)))
		return -1;

	if (!(f = fopen(filename, "w")))
		return -1;

	fchmod(fileno(f), 0600);
	fprintf(f, "%s\n", (sess) ? "C" : "D");

	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;
		
		fprintf(f, "%s:%d%s%s\n", u->display, u->status,(u->descr)?":":"" ,(u->descr)?u->descr:"");
	}

	fclose(f);

	return 0;
}
#endif

/*
 * userlist_write_crash()
 *
 * zapisuje list� kontakt�w w sytuacji kryzysowej jak najmniejszym
 * nak�adem pami�ci i pracy.
 */
void userlist_write_crash()
{
	list_t l;
	char name[32];
	FILE *f;

	chdir(config_dir);
	
	snprintf(name, sizeof(name), "userlist.%d", getpid());
	if (!(f = fopen(name, "w")))
		return;

	chmod(name, 0400);
		
	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;
		list_t m;
		
		fprintf(f, "%s;%s;%s;%s;%s;", 
			(u->first_name) ? u->first_name : u->display,
			(u->last_name) ? u->last_name : "", (u->nickname) ?
			u->nickname : u->display, u->display, (u->mobile) ?
			u->mobile : "");
		
		for (m = u->groups; m; m = m->next) {
			struct group *g = m->data;

			if (m != u->groups)
				fprintf(f, ",");

			fprintf(f, g->name);
		}
		
		fprintf(f, ";%u\r\n", u->uin);
	}	

	fclose(f);
}

/*
 * userlist_clear_status()
 *
 * czy�ci stan u�ytkownik�w na li�cie.
 */
void userlist_clear_status()
{
        list_t l;

        for (l = userlist; l; l = l->next) {
                struct userlist *u = l->data;

                u->status = GG_STATUS_NOT_AVAIL;
        }
}

/*
 * userlist_clear()
 *
 * czy�ci list� u�ytkownik�w.
 */
void userlist_clear()
{
	while (userlist)
		userlist_remove(userlist->data);
}

/*
 * userlist_add()
 *
 * dodaje u�ytkownika do listy.
 *
 *  - uin,
 *  - display.
 */
struct userlist *userlist_add(uin_t uin, const char *display)
{
	struct userlist u;

	memset(&u, 0, sizeof(u));

	u.uin = uin;
	u.status = GG_STATUS_NOT_AVAIL;
	u.display = xstrdup(display);

	return list_add_sorted(&userlist, &u, sizeof(u), userlist_compare);
}

/*
 * userlist_remove()
 *
 * usuwa danego u�ytkownika z listy kontakt�w.
 *
 *  - u.
 */
int userlist_remove(struct userlist *u)
{
	list_t l;

	if (!u)
		return -1;
	
	xfree(u->first_name);
	xfree(u->last_name);
	xfree(u->nickname);
	xfree(u->display);
	xfree(u->mobile);
	xfree(u->descr);

	for (l = u->groups; l; l = l->next) {
		struct group *g = l->data;

		xfree(g->name);
	}
	list_destroy(u->groups, 1);

	list_remove(&userlist, u, 1);

	return 0;
}

/*
 * userlist_replace()
 *
 * usuwa i dodaje na nowo u�ytkownika, �eby zosta� umieszczony na odpowiednim
 * (pod wzgl�dem kolejno�ci alfabetycznej) miejscu. g�upie to troch�, ale
 * przy listach jednokierunkowych nie za bardzo chce mi si� miesza� z
 * przesuwaniem element�w listy.
 * 
 *  - u.
 *
 * zwraca zero je�li jest ok, -1 je�li b��d.
 */
int userlist_replace(struct userlist *u)
{
	if (list_remove(&userlist, u, 0))
		return -1;
	if (list_add_sorted(&userlist, u, 0, userlist_compare))
		return -1;

	return 0;
}

/*
 * userlist_find()
 *
 * znajduje odpowiedni� struktur� `userlist' odpowiadaj�c� danemu numerkowi
 * lub jego opisowi.
 *
 *  - uin,
 *  - display.
 */
struct userlist *userlist_find(uin_t uin, const char *display)
{
	list_t l;

	for (l = userlist; l; l = l->next) {
		struct userlist *u = l->data;

                if (uin && u->uin == uin)
			return u;
                if (display && u->display && !strcasecmp(u->display, display))
                        return u;
        }

        return NULL;
}

/*
 * userlist_type()
 *
 * zwraca rodzaj u�ytkownika dla funkcji gg_*_notify_ex().
 *
 *  - u - wpis u�ytkownika
 *
 * GG_USER_*
 */
char userlist_type(struct userlist *u)
{
	char res = GG_USER_NORMAL;

	if (!u)
		return res;
	
	if (group_member(u, "__offline"))
		res = GG_USER_OFFLINE;
		
	if (group_member(u, "__blocked"))
		res = GG_USER_BLOCKED;

	return res;
}

/*
 * get_uin()
 *
 * je�li podany tekst jest liczb� (ale nie jednocze�nie nazw� u�ytkownika),
 * zwraca jej warto��. je�li jest nazw� u�ytkownika w naszej li�cie kontakt�w,
 * zwraca jego numerek. inaczej zwraca zero.
 *
 *  - text.
 */
uin_t get_uin(const char *text)
{
	uin_t uin = str_to_uin(text);
	struct userlist *u = userlist_find(uin, text);

	if (u)
		return u->uin;
	else
		return uin;
}

/*
 * format_user()
 *
 * zwraca �adny (ew. kolorowy) tekst opisuj�cy dany numerek. je�li jest
 * w naszej li�cie kontakt�w, formatuje u�ywaj�c `known_user', w przeciwnym
 * wypadku u�ywa `unknown_user'. wynik jest w statycznym buforze.
 *
 *  - uin - numerek danej osoby.
 */
const char *format_user(uin_t uin)
{
	struct userlist *u = userlist_find(uin, NULL);
	static char buf[100], *tmp;
	
	if (!u || !u->display)
		tmp = format_string(format_find("unknown_user"), itoa(uin));
	else
		tmp = format_string(format_find("known_user"), u->display, itoa(uin));
	
	strncpy(buf, tmp, sizeof(buf) - 1);
	
	xfree(tmp);

	return buf;
}

/*
 * ignored_remove()
 *
 * usuwa z listy ignorowanych numerk�w.
 *
 *  - uin.
 */
int ignored_remove(uin_t uin)
{
	struct userlist *u = userlist_find(uin, NULL);
	list_t l;

	if (!u)
		return -1;

	if (!ignored_check(uin))
		return -1;

	for (l = u->groups; l; ) {
		struct group *g = l->data;

		l = l->next;

		if (!g || !g->name || strncasecmp(g->name, "__ignored", 9))
			continue;

		xfree(g->name);
		list_remove(&u->groups, g, 1);
	}

	if (!u->display && !u->groups)
		userlist_remove(u);

	return 0;
}

/*
 * ignored_add()
 *
 * dopisuje do listy ignorowanych numerk�w.
 *
 *  - uin.
 *  - level.
 */
int ignored_add(uin_t uin, int level)
{
	struct userlist *u;
	char *tmp;

	if (ignored_check(uin))
		return -1;
	
	if (!(u = userlist_find(uin, NULL)))
		u = userlist_add(uin, NULL);

	tmp = saprintf("__ignored_%d", level);
	group_add(u, tmp);
	xfree(tmp);
	
	return 0;
}

/*
 * ignored_check()
 *
 * czy dany numerek znajduje si� na li�cie ignorowanych.
 *
 *  - uin.
 */
int ignored_check(uin_t uin)
{
	struct userlist *u = userlist_find(uin, NULL);
	list_t l;

	if (!u)
		return 0;

	for (l = u->groups; l; l = l->next) {
		struct group *g = l->data;

		if (!g->name)
			continue;

		if (!strcasecmp(g->name, "__ignored"))
			return IGNORE_ALL;

		if (!strncasecmp(g->name, "__ignored_", 10))
			return atoi(g->name + 10);
	}

	return 0;
}

/*
 * blocked_remove()
 *
 * usuwa z listy blokowanych numerk�w.
 *
 *  - uin.
 */
int blocked_remove(uin_t uin)
{
	struct userlist *u = userlist_find(uin, NULL);
	list_t l;

	if (!u)
		return -1;

	if (!group_member(u, "__blocked"))
		return -1;

	gg_remove_notify_ex(sess, u->uin, userlist_type(u));

	for (l = u->groups; l; ) {
		struct group *g = l->data;

		l = l->next;

		if (!g || !g->name || strcasecmp(g->name, "__blocked"))
			continue;

		xfree(g->name);
		list_remove(&u->groups, g, 1);
	}

	if (!u->display && !u->groups)
		userlist_remove(u);
	else
		gg_add_notify_ex(sess, u->uin, userlist_type(u));

	return 0;
}

/*
 * blocked_add()
 *
 * dopisuje do listy blokowanych numerk�w.
 *
 *  - uin.
 */
int blocked_add(uin_t uin)
{
	struct userlist *u = userlist_find(uin, NULL);

	if (u && group_member(u, "__blocked"))
		return 0;
	
	if (!u)
		u = userlist_add(uin, NULL);
	else
		gg_remove_notify_ex(sess, uin, userlist_type(u));

	group_add(u, "__blocked");

	gg_add_notify_ex(sess, uin, userlist_type(u));
	
	return 0;
}

/*
 * userlist_send()
 *
 * wysy�a do serwera userlist�, wywo�uj�c gg_notify()
 */
void userlist_send()
{
        list_t l;
        uin_t *uins;
	char *types;
        int i, count;

	count = list_count(userlist);

        uins = xmalloc(count * sizeof(uin_t));
	types = xmalloc(count * sizeof(char));

	for (i = 0, l = userlist; l; i++, l = l->next) {
		struct userlist *u = l->data;

                uins[i] = u->uin;
		types[i] = userlist_type(u);
	}

        gg_notify_ex(sess, uins, types, count);

        xfree(uins);
	xfree(types);
}

/*
 * group_compare()
 *
 * funkcja pomocna przy list_add_sorted().
 *
 *  - data1, data2 - dwa wpisy grup do por�wnania.
 *
 * zwraca wynik strcasecmp() na nazwach grup.
 */
static int group_compare(void *data1, void *data2)
{
	struct group *a = data1, *b = data2;
	
	if (!a || !a->name || !b || !b->name)
		return 0;

	return strcasecmp(a->name, b->name);
}

/*
 * group_add()
 *
 * dodaje u�ytkownika do podanej grupy.
 *
 *  - u - wpis usera,
 *  - group - nazwa grupy.
 */
int group_add(struct userlist *u, const char *group)
{
	struct group g;
	list_t l;

	for (l = u->groups; l; l = l->next) {
		struct group *g = l->data;

		if (!strcasecmp(g->name, group))
			return -1;
	}
	
	g.name = xstrdup(group);

	list_add_sorted(&u->groups, &g, sizeof(g), group_compare);

	return 0;
}

/*
 * group_remove()
 *
 * usuwa u�ytkownika z podanej grupy.
 *
 *  - u - wpis usera,
 *  - group - nazwa grupy.
 *
 * zwraca 0 je�li si� uda�o, inaczej -1.
 */
int group_remove(struct userlist *u, const char *group)
{
	list_t l;

	if (!u || !group)
		return -1;
	
	for (l = u->groups; l; l = l->next) {
		struct group *g = l->data;

		if (!g || !g->name)
			continue;

		if (!strcasecmp(g->name, group)) {
			xfree(g->name);
			list_remove(&u->groups, g, 1);
			
			return 0;
		}
	}
	
	return -1;
}

/*
 * group_member()
 *
 * sprawdza czy u�ytkownik jest cz�onkiem danej grupy.
 *
 * zwraca 1 je�li tak, 0 je�li nie.
 */
int group_member(struct userlist *u, const char *group)
{
	list_t l;

	if (!u || !group)
		return 0;

	for (l = u->groups; l; l = l->next) {
		struct group *g = l->data;

		if (!strcasecmp(g->name, group))
			return 1;
	}

	return 0;
}

/*
 * group_init()
 *
 * inicjuje list� grup u�ytkownika na podstawie danego ci�gu znak�w,
 * w kt�rym kolejne nazwy grup s� rozdzielone przecinkiem.
 * 
 *  - names - nazwy grup.
 *
 * zwraca list� `struct group' je�li si� uda�o, inaczej NULL.
 */
list_t group_init(const char *names)
{
	list_t l = NULL;
	char **groups;
	int i;

	if (!names)
		return NULL;

	groups = array_make(names, ",", 0, 1, 0);

	for (i = 0; groups[i]; i++) {
		struct group g;

		g.name = xstrdup(groups[i]);
		list_add_sorted(&l, &g, sizeof(g), group_compare);
	}
	
	array_free(groups);
	
	return l;
}

/*
 * group_to_string()
 *
 * zmienia list� grup na ci�g znak�w rodzielony przecinkami.
 *
 *  - groups - lista grup.
 *  - meta - czy do��czy� ,,meta-grupy''?
 *
 * zwraca zaalokowany ci�g znak�w lub NULL w przypadku b��du.
 */
char *group_to_string(list_t groups, int meta)
{
	string_t foo;
	list_t l;

	if (!(foo = string_init(NULL)))
		return NULL;

	for (l = groups; l; l = l->next) {
		struct group *g = l->data;

		if (!g || !g->name)
			continue;
		
		if (!meta && !strncmp(g->name, "__", 2))
			continue;

		if (l != groups)
			string_append_c(foo, ',');
		
		string_append(foo, g->name);
	}

	return string_free(foo, 0);
}
