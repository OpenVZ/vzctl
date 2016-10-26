/* Copyright (C) SWsoft, 1999-2007. All rights reserved.*/
#ifndef _VZLIST_H
#define _VZLIST_H

#include <vzctl/libvzctl.h>

#include <linux/vzlist.h>

#define PROCVEINFO	"/proc/vz/veinfo"
#define PROCUBC		"/proc/user_beancounters"
#define PROC_BC_RES	"/proc/bc/resources"
#define PROCVEINFO	"/proc/vz/veinfo"

#define MAXCPUUNITS	500000
#define MAX_STR		16384

struct Cubc {			// held maxheld barrier limit failcnt
	unsigned long kmemsize[5];
	unsigned long lockedpages[5];
	unsigned long privvmpages[5];
	unsigned long shmpages[5];
	unsigned long numproc[5];
	unsigned long physpages[5];
	unsigned long vmguarpages[5];
	unsigned long oomguarpages[5];
	unsigned long numtcpsock[5];
	unsigned long numflock[5];
	unsigned long numpty[5];
	unsigned long numsiginfo[5];
	unsigned long tcpsndbuf[5];
	unsigned long tcprcvbuf[5];
	unsigned long othersockbuf[5];
	unsigned long dgramrcvbuf[5];
	unsigned long numothersock[5];
	unsigned long dcachesize[5];
	unsigned long numfile[5];
	unsigned long numiptent[5];
	unsigned long swappages[5];
};

struct Cslm {
	int mode;
	unsigned long memorylimit[3];	// inst avg quality
};

struct Cquota {
	unsigned long quota_block[3];	// 0-usage 1-softlimit 2-hardlimit
	unsigned long quota_inode[3];	// 0-usage 1-softlimit 2-hardlimit
};

struct Ccpu {
	unsigned long limit[3];		// 0-limit %, 1-limit Mhz 2-units
};

struct vzctl_io_param {
	int *prio;
	unsigned int *limit;
	unsigned int *iopslimit;
};

struct Cveinfo {
	ctid_t ctid;
	int tm;
	char *hostname;
	char *name;
	char *description;
	char *ip;
	char *ext_ip;
	char *ve_private;
	char *ve_root;
	char *ostmpl;
	struct Cubc *ubc;
	struct Cquota *quota;
	struct Ccpu *cpu;
	veth_param *veth;
	int status;
	int hide;
	int onboot;
	char *clust_srv_name;
	unsigned long *bootorder;
	struct vzctl_cpustat *cpustat;
	struct vzctl_io_param io;
	char *cpumask;
	char *nodemask;
	struct vzctl_env_features *features;
	int mm;
	int ha_enable;
	unsigned long *ha_prio;
	char *netfilter;
	char *uuid;
	char *devnodes;
	int autostop;
};

#define RES_NONE	0
#define RES_HOSTNAME	1
#define RES_UBC		2
#define RES_QUOTA	3
#define RES_IP		4
#define RES_LA		5
#define RES_CPU		6
#define RES_TM		7
#define RES_OSTEMPLATE	8
#define RES_DESCRIPTION	9
#define RES_NAME	10
#define RES_NETIF	13
#define RES_IFNAME	14
#define RES_ONBOOT	15
#define RES_BOOTORDER	18
#define RES_UPTIME	19
#define RES_IO		20
#define RES_CONFIGURED_IP 21
#define RES_HA_ENABLE	22
#define RES_HA_PRIO	23
#define RES_STATUS	24

struct Cfield {
	char *name;
	char *hdr;
	char *hdr_fmt;
	int index;
	int res_type;
	void (* const print_fn)(struct Cveinfo *p, int index);
	int (* const sort_fn)(const void* val1, const void* val2);
};

struct Cfield_order {
	int order;
	struct Cfield_order *next;
};

#endif /* _VZLIST_H */
