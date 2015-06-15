/*
 *
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
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
#define	STR_ESC		37
#define	NETIF		38
#define	NETIF_ADD_T	39
#define	NETIF_DEL_T	40
#define	NETIF_DHCP	43
#define	NETIF_NAMESERVER 44
#define	NETIF_MAC	45
#define	NETIF_HOST_MAC	46
#define	NETIF_HOST_IFNAME 47
#define	NETIF_GW	48
#define	NETIF_NETWORK	49
#define	NETIF_NETTYPE	50
#define	NETIF_NAME	51
#define STR_UTF8	53
#define APPCONF_MAP	54
#define STR_NAME	55
#define NETIF_MAC_FILTER 58
#define CLUSTER_MODE_T	59
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
int ParseInt(const char *str, int *val);
int ParseCap(int veid, char *rtoken, cap_t *mask_on, cap_t *mask_off);
int ParseConfig(int veid, char *path, struct CParam *param, int logging);
void RemoveParam(struct CParam *param);
int SaveConfig(int veid, char *path, struct CParam *param, struct CParam *old_p);
int UnsetConfig(int veid, char *path, struct CParam *param);
int FindOption(int id);
int SetParam(int i, char *sp, int checkdup, int version, int unset);
int FindName(char *name);
int GetConfVer(int veid, int version, struct CParam *param);
void FreeParam(struct CParam *param);
struct CParam *ConvertUBC(struct CParam *p);
void FillParam(int veid, struct CParam *g, struct CParam *p);
int get_swap(unsigned long long *swap);
int get_mem(unsigned long long *mem);
int get_lowmem(unsigned long long *mem);
int get_ubc(int veid, struct CParam *param);
int get_pagesize(void);
int yesno_to_num(char *str);
char *get_ipname(unsigned int ip);
int get_ipaddr(const char *name, unsigned int *ip);
int GetConfVer(int veid, int version, struct CParam *param);
int isConfModified(struct CParam *param, struct CParam *local);
int get_num_cpu(void);
int add_veth_param(veth_param *list, veth_dev *dev);
int add_veth_ip(list_head_t *list, veth_ip *ip, int op);
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
int get_run_ve(int **buf);
int *find_veid(int veid, int *base, int nmemb);
int CheckRate(struct CList *rate);
int get_conf_mm_mode(struct CParam *param);
int get_ip_str(struct CList *ip, char *buf, int len);
#endif /* _CONFIG_H_ */
