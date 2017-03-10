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

/* $Id$ */
#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "vzctl.h"
#include "veth.h"
#include "list.h"

#define STRNCOMP(x, y)  strncmp(x, y, strlen(y))
#define NONE		-1
#define	TWOULONG	1
#define ULONG		2
#define INT		3
#define LIST		4
#define STR		5
#define	YESNO		7
#define IP_ADDR		9
#define FLAG		10
#define VETYPE		11
#define FLOAT		15
#define LIST_PW		23
#define SETMODE		27
#define	TWOULONG_P	31
#define	TWOULONG_B2P	32
#define	TWOULONG_N	33
#define	TWOULONG_K	34
#define STR_UTF8	53
#define APPCONF_MAP	54
#define ONOFF		60
#define	NETIF_DHCP6	64
#define	NETIF_GW6	65
#define PLOOP_TYPE_T	71
#define STR_GUID	73
#define NETIF_CONFIGURE_MODE 74
#define DISK_T		75

struct CParam tmpparam;

struct Cconfig
{
	char *name;	/* Name in config file	*/
	char *alias;
	void *pvar;	/* parameter pointer	*/
	void *pvar1;	/* parameter pointer2	*/
	int option_id;	/* Command line option id*/
	int type;       /* Type			*/
	int version;	/* UBC parameters version
			* 0 - none
			* 1 - old
			* 2 - new
			*/
	int opt_srch;	/* 0 - full compare
			* 1 - partial compare
			*/
};

extern struct Cconfig Config[];

struct CSkipVeParam {
	char *name;
};

#define SET_NONE	0
#define SET_IGNORE	1
#define SET_RESTART	2
struct CSetMode {
	char *name;
	int mode;
};

struct CParam;
struct CList;

int ParseTwoLongs(const char *str, unsigned long *val, int type);
int FindOption(int id);
int SetParam(int i, char *sp, int checkdup, int version, int unset);
int FindName(char *name);
void FreeParam(struct CParam *param);
int get_mem(unsigned long long *mem);
int get_lowmem(unsigned long long *mem);
int get_ubc(int veid, struct CParam *param);
int get_pagesize(void);
int yesno_to_num(char *str);
char *get_ipname(unsigned int ip);
int get_ipaddr(const char *name, unsigned int *ip);
int add_veth_param(veth_param *list, veth_dev *dev);
int add_veth_ip(list_head_t *list, veth_ip *ip, int op);
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
int *find_veid(int veid, int *base, int nmemb);
int get_ip_str(struct CList *ip, char *buf, int len);
#endif /* _CONFIG_H_ */
