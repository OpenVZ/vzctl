/*
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */

#ifndef __LIST_H__
#define __LIST_H__


struct list_elem;
typedef struct {
	struct list_elem *prev, *next;
} list_head_t;

typedef struct list_elem {
	struct list_elem *prev, *next;
} list_elem_t;

struct vzctl_str_param {
	list_elem_t list;
	char *str;
};

#ifdef __cplusplus
extern "C" {
#endif
static inline void list_head_init(list_head_t *head)
{
	head->next = (list_elem_t *)head;
	head->prev = (list_elem_t *)head;
}

static inline void list_elem_init(list_elem_t *entry)
{
	entry->next = entry;
	entry->prev = entry;
}

static inline void list_add(list_elem_t *new_el, list_head_t *list)
{
	new_el->next = list->next;
	new_el->prev = (list_elem_t *)list;
	list->next->prev = new_el;
	list->next = new_el;
}

static inline void list_add_tail(list_elem_t *new_el, list_head_t *list)
{
	new_el->next = (list_elem_t *)list;
	new_el->prev = list->prev;
	list->prev->next = new_el;
	list->prev = new_el;
}

static inline void list_del(list_elem_t *el)
{
	el->prev->next = el->next;
	el->next->prev = el->prev;
	el->prev = NULL;
	el->next = NULL;
}

static inline void list_del_init(list_elem_t *el)
{
	el->prev->next = el->next;
	el->next->prev = el->prev;
	list_elem_init(el);
}

static inline int list_is_init(list_head_t *h)
{
	return h->next == NULL;
}

static inline int list_empty(list_head_t *h)
{
	if (list_is_init(h))
		return 1;
	return h->next == (list_elem_t *)h;
}

static inline int list_elem_inserted(list_elem_t *el)
{
	return el->next != el;
}

static inline void list_moveall(list_head_t *src, list_head_t *dst)
{
	list_add((list_elem_t *)dst, src);
	list_del((list_elem_t *)src);
	list_head_init(src);
}
#ifdef __cplusplus
}
#endif

#define LIST_HEAD(name) \
	list_head_t name = { (void *) &name, (void *) &name }

#define list_entry(ptr, type, field)					\
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->field)))

#define list_first_entry(head, type, field)				\
	list_entry((head)->next, type, field)

#define list_for_each(entry, head, field)				\
	for (entry = list_entry((head)->next, typeof(*entry), field);\
	     &entry->field != (list_elem_t*)(head);			\
	     entry = list_entry(entry->field.next, typeof(*entry), field))

#define list_for_each_prev(entry, head, field)				\
	for (entry = list_entry((head)->prev, typeof(*entry), field);\
	     &entry->field != (list_elem_t*)(head);			\
	     entry = list_entry(entry->field.prev, typeof(*entry), field))

#define list_for_each_safe(entry, tmp, head, field)			\
	for (entry = list_entry((head)->next, typeof(*entry), field),\
		tmp = list_entry(entry->field.next, typeof(*entry), field); \
	     &entry->field != (list_elem_t*)(head);			\
	     entry = tmp,						\
		tmp = list_entry(tmp->field.next, typeof(*tmp), field))

#endif /* __LIST_H__ */
