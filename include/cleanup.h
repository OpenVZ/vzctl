#ifndef _VZCTL_CLEANUP_H_
#define _VZCTL_CLEANUP_H_

#include "list.h"

typedef void (* cleanupFN)(void *data);

struct vzctl_cleanup_handler {
	list_elem_t list;
	cleanupFN f;
	void *data;
};

#ifdef __cplusplus
extern "C" {
#endif

struct vzctl_cleanup_handler *add_cleanup_handler(cleanupFN f, void *data);
void run_cleanup(void);
void del_cleanup_handler(struct vzctl_cleanup_handler *h);
void free_cleanup(void);
void cleanup_kill_process(void *data);
#ifdef __cplusplus
}
#endif

#endif /* _VZCTL_CLEANUP_H_ */
