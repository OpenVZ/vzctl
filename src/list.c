#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vzctl.h"
#include "vzerror.h"
#include "clist.h"

struct CList *ListAddElem(struct CList *list, char *val, unsigned long *val1,
			unsigned long *val2)
{
	struct CList *lp_t, *lp;

	if ((lp_t = calloc(sizeof(struct CList), 1)) == NULL)
	{
		exit(VZ_NOMEM_ERROR);
	}
	if (val != NULL)
	{
		lp_t->str = strdup(val);
		lp_t->val1 = val1;
		lp_t->val2 = val2;
	}
	if (list)
	{
		lp = list->end;
		lp->next = lp_t;
		list->end = lp_t;
	}
	else
	{
		list = lp_t;
		list->end = list;
	}
	return list;
}

struct CList *ListAddIpElem(struct CList *list, char *ip, unsigned long *mask)
{
	struct CList *lp_t, *lp;

	if ((lp_t = calloc(sizeof(struct CList), 1)) == NULL)
	{
		exit(VZ_NOMEM_ERROR);
	}
	if (ip != NULL)
	{
		lp_t->str = strdup(ip);
		if (mask != NULL) {
			lp_t->val1 = malloc(sizeof(unsigned long));
			if (lp_t->val1 != NULL)
				*lp_t->val1 = *mask;
		}
	}
	if (list)
	{
		lp = list->end;
		lp->next = lp_t;
		list->end = lp_t;
	}
	else
	{
		list = lp_t;
		list->end = list;
	}
	return list;
}

void ListFree(struct CList *list)
{
	struct CList *l_t;

	while (list != NULL)
	{
		l_t = list;
		list = list->next;
		free(l_t->str);
		free(l_t->val1);
		free(l_t->val2);
		free(l_t);
	}
}

int ListSize(struct CList *list)
{
	struct CList *l = list;
	int cnt = 0;
	while (l)
	{
		cnt++;
		l = l->next;
	}
	return cnt;
}

struct CList *ListLast(struct CList *list)
{
	struct CList *l = list;

	if (!l)
		return NULL;

	while (l->next)
		l = l->next;

	return l;
}


struct CList *ListFindVal(struct CList *list, unsigned long *val1,
		unsigned long *val2)
{
	struct CList *l;
	int found;

	if (list == NULL)
		return NULL;
	if (val1 == NULL && val2 == NULL)
		return NULL;
	found = 0;
	for (l = list; l != NULL; l = l->next) {
		if (val1 != NULL && l->val1 != NULL)
			found = (*val1 == *l->val1);
		if (val2 != NULL && l->val2 != NULL)
			found = (*val2 == *l->val2);
		if (found)
			return l;
	}
	return NULL;
}

struct CList *ListFind(struct CList *list, const char *elem)
{
	struct CList *l;

	if (elem == NULL || list == NULL)
		return NULL;

	for (l = list; l != NULL; l = l->next)
		if (l->str != NULL && !strcmp(l->str, elem))
			return l;

	return NULL;
}

struct CList *ListFindVar(struct CList *list, char *elem, char delim)
{
	struct CList *l = list;
	char *p;
	int len1, len2 ;

	if (!elem) return 0;

	while (l)
	{
		if (l->str)
		{
			if ((p = strchr(l->str, delim)))
			{
				len1 = p - l->str;
				len2 = strlen(elem);
				if (len1 == len2 && !strncmp(l->str, elem, len1))
					return l;
			}
		}
		l = l->next;
	}
	return NULL;
}

char * List_genstr(struct CList *list, char *buf, int size)
{
	struct CList *l;
	char *sp, *ep;

	sp = buf;
	*sp = 0;
	ep = buf + size;
	for (l = list; l != NULL; l = l->next)
	{
		if (l->str == NULL)
			continue;
		sp += snprintf(sp, ep - sp, "%s ", l->str);
		if (sp >= ep)
			break;
	}
	return buf;
}

int ListCmp(struct CList *l1, struct CList *l2)
{
	if (ListSize(l1) != ListSize(l2))
		return 1;
	for (; l2 != NULL; l2 = l2->next) {
		if (ListFind(l1, l2->str) == NULL)
			return 1;
	}
	return 0;
}
