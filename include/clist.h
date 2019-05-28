/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
 *
 * This file is part of OpenVZ. OpenVZ is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef	_CLIST_H_
#define	_CLIST_H_

#include "list.h"

struct CList
{
	char *str;
	unsigned long *val1;
	unsigned long *val2;
	int flag;
	struct CList *next;
	struct CList *end;
};

struct CList *ListAddElem(struct CList *list, char *val, unsigned long *val1,
		    unsigned long *val2);
struct CList *ListAddIpElem(struct CList *list, char *ip, unsigned long *mask);
void ListFree(struct CList *list);
int ListSize(struct CList *list);
struct CList *ListLast(struct CList *list);
struct CList *ListFind(struct CList *list, const char *elem);
struct CList *ListFindVar(struct CList *list, char *elem, char delim);
struct CList *ListFindVal(struct CList *list, unsigned long *val1,
		unsigned long *val2);
char * List_genstr(struct CList *list, char *buf, int size);
int ListCmp(struct CList *l1, struct CList *l2);
#endif /* _CLIST_H_ */
