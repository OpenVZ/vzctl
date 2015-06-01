/* Copyright (C) SWsoft, 1999-2007. All rights reserved.*/
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
