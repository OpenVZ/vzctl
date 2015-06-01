/*
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */
#ifndef _UTIL_H_
#define	_UTIL_H_
#include <stdio.h>
#include "list.h"

#define BACKUP		0
#define DESTR		1

#define PROCMEM		"/proc/meminfo"
#define PROCTHR		"/proc/sys/kernel/threads-max"
#define SAMPLE_CONF_PATTERN	"ve-%s.conf-sample"

#define FREE_P(x)	free(x); x = NULL;
#define MERGE_STR(x)\
	if ((src->x) != NULL) {				\
		free(dst->x);				\
		dst->x = strdup(src->x);		\
	}
#define STR2MAC(dev)                    \
	((unsigned char *)dev)[0],      \
	((unsigned char *)dev)[1],      \
	((unsigned char *)dev)[2],      \
	((unsigned char *)dev)[3],      \
	((unsigned char *)dev)[4],      \
	((unsigned char *)dev)[5]

#define VZCTL_LOCK_EX	0x1
#define VZCTL_LOCK_SH	0x2
#define VZCTL_LOCK_NB	0x4

#define VZCTL_CLOSE_STD		0x1
#define VZCTL_CLOSE_NOCHECK	0x2

#ifdef __cplusplus
extern "C" {
#endif
/** Convert text from UTF-8 to current locale
 *
 * @param src		source text
 * @param dst		destination text should have enougth space
 * @param dst_size	destination buffer size
 * @return		0 on success
 */
int utf8tostr(const char *src, char *dst, int dst_size, const char *enc);
int xstrdup(char **dst, const char *src);
char *parse_line(char *str, char *ltoken, int lsz);
int stat_file(const char *file);
int check_var(const void *val, const char *message);
int make_dir(const char *path, int full, int mode);
int set_not_blk(int fd);
int reset_std(void);

int yesno2id(const char *str);
const char *id2yesno(int id);
int get_net_family(const char *ipstr);
int get_netaddr(const char *ip_str, unsigned int *ip);
int parse_int(const char *str, int *val);
int parse_ul(const char *str, unsigned long *val);

void str_tolower(const char *from, char *to);
char *unescapestr(char *src);

double max(double val1, double val2);
unsigned long max_ul(unsigned long val1, unsigned long val2);
unsigned long min_ul(unsigned long val1, unsigned long val2);

/** Close all fd.
 * @param close_std     flag for closing the [0-2] fds
 * @param ...		list of fds are skiped, (-1 is the end mark)
*/
void close_fds(int close_std, ...);


int parse_hwaddr(const char *str, char *addr);
int vzctl_lock(const char *lockfile, int mode, unsigned int timeout);
void vzctl_unlock(int fd, const char *lockfile);
void free_str(list_head_t *head);
int copy_str(list_head_t *dst, list_head_t *src);
struct vzctl_str_param *add_str_param(list_head_t *head, const char *str);
const struct vzctl_str_param *find_str(list_head_t *head, const char *str);

int is_str_valid(const char *name);
int env_is_mounted(ctid_t ctid);
int vzctl_get_dump_file(unsigned veid, char *ve_private, char *dumpdir,
	char *buf, int size);
char *get_ip4_name(unsigned int ip);
void free_ar_str(char **ar);
char **list2ar_str(list_head_t *head);
int copy_file(const char *src, const char *dst);
int run_script(char *argv[], char *env[], int quiet);
int merge_str_list(list_head_t *old, list_head_t *add,
	list_head_t *del, int delall, list_head_t *merged);
char *list2str(const char *prefix, list_head_t *head);
int is_vswap_mode(void);
void features_mask2str(unsigned long long mask, unsigned long long known, char *delim,
		char *buf, int len);
FILE *vzctl_popen(char *argv[], char *env[], int quiet);
int vzctl_pclose(FILE *fp);
char *arg2str(char **arg);
int vzctl_get_normalized_guid(const char *str, char *out, int len);
int get_mul(char c, unsigned long long *n);
void vzctl_register_running_state(const char *ve_private);
void vzctl_unregister_running_state(const char *ve_private);
char *get_ipname(unsigned int ip);
char *get_script_path(const char *name, char *buf, int size);
const char *get_dir_lock_file(const char *dir, char *buf, int size);
#ifdef __cplusplus
}
#endif
#endif /* _UTIL_H_ */
