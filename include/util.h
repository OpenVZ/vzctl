/*
 * Copyright (c) 1999-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
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
#define STR2MAC(dev)                    \
	((unsigned char *)dev)[0],      \
	((unsigned char *)dev)[1],      \
	((unsigned char *)dev)[2],      \
	((unsigned char *)dev)[3],      \
	((unsigned char *)dev)[4],      \
	((unsigned char *)dev)[5]


#define VZCTL_CLOSE_STD		0x1
#define VZCTL_CLOSE_NOCHECK	0x2

#ifdef __cplusplus
extern "C" {
#endif
int utf8tostr(const char *src, char *dst, int dst_size, const char *enc);
int xstrdup(char **dst, const char *src);
char *parse_line(char *str, char *ltoken, int lsz);
int stat_file(const char *file);
int check_var(const void *val, const char *message);
int make_dir(const char *path, int full, int mode);
int reset_std(void);
int yesno2id(const char *str);
const char *id2yesno(int id);
int get_net_family(const char *ipstr);
int get_netaddr(const char *ip_str, unsigned int *ip);
int parse_int(const char *str, int *val);
double max(double val1, double val2);
unsigned long max_ul(unsigned long val1, unsigned long val2);
unsigned long min_ul(unsigned long val1, unsigned long val2);
void free_str(list_head_t *head);
struct vzctl_str_param *add_str_param(list_head_t *head, const char *str);
int env_is_mounted(ctid_t ctid);
void free_ar_str(char **ar);
int copy_file(const char *src, const char *dst);
char *list2str(const char *prefix, list_head_t *head);
int is_vswap_mode(void);
void features_mask2str(unsigned long long mask, unsigned long long known, char *delim,
		char *buf, int len);
void print_json_features(unsigned long long mask, unsigned long long known);
FILE *vzctl_popen(char *argv[], char *env[], int quiet);
int vzctl_pclose(FILE *fp);
char *arg2str(char **arg);
int vzctl_get_normalized_guid(const char *str, char *out, int len);
int get_mul(char c, unsigned long long *n);
#ifdef __cplusplus
}
#endif
#endif /* _UTIL_H_ */
