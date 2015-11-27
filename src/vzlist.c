#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <getopt.h>
#include <fnmatch.h>
#include <errno.h>
#include <math.h>

#include <ploop/libploop.h>
#include <vzctl/libvzctl.h>

#include "vzctl.h"
#include "veth.h"
#include "vzlist.h"
#include "config.h"
#include "util.h"


static const char *ve_status(int status)
{
	if (status & ENV_STATUS_RUNNING)
		return "running";
	else if (status & ENV_STATUS_MOUNTED)
		return "mounted";
	else if (status & ENV_STATUS_SUSPENDED)
		return "suspended";
	else if (status & ENV_STATUS_EXISTS)
		return "stopped";
	return "unknown";
}

static struct Cveinfo *veinfo = NULL;
static int n_veinfo = 0;

static char g_outbuffer[MAX_STR] = "";
static char *p_outbuffer = g_outbuffer;
static char *e_buf = g_outbuffer + sizeof(g_outbuffer) - 1;
static char *host_pattern = NULL;
static char *name_pattern = NULL;
static char *desc_pattern = NULL;
static char *netif_pattern = NULL;
//static int vzctlfd;
static struct Cfield_order *g_field_order = NULL;
static int last_field;
static char *default_field_order = "veid,numproc,status,configured_ip,hostname";
static char *default_compat_field_order = "status,configured_ip,smart_name";
static char *default_nm_field_order = "ctid,numproc,status,configured_ip,name";
static char *default_netif_field_order = "ctid,status,host_ifname,configured_ip,hostname";
static int g_sort_field = 0;
static char *g_ve_list;
static int n_ve_list = 0;
static int veid_only = 0;
static int sort_rev = 0;
static int show_hdr = 1;
static int all_ve = 0;
static int only_stopped_ve = 0;
static int with_names = 0;
static int g_compat_mode = 0;
static int g_skip_owner = 0;

struct CParam *gparam;

static inline int check_param(int res_type);
static char *get_real_ips(ctid_t ctid);

static void print_hostname(struct Cveinfo *p, int index);
static void print_name(struct Cveinfo *p, int index);
static void print_smart_name(struct Cveinfo *p, int index);
static void print_description(struct Cveinfo *p, int index);
static int match_ip(char *ip_list, char *ip);
static char *print_real_ip(struct Cveinfo *p, char *str, int dash);
static void print_ip(struct Cveinfo *p, int index);
static void print_dev_name(struct Cveinfo *p, int index);
static void print_netif_params(struct Cveinfo *p, int index);
static void print_ve_private(struct Cveinfo *p, int index);
static void print_ve_root(struct Cveinfo *p, int index);
static void print_ostemplate(struct Cveinfo *p, int index);

static void SET_P_OUTBUFFER(int r, int len, int short_len)
{
	if (!last_field && short_len != 0)
		r = short_len;
	if (r > len)
		r = len;
	p_outbuffer += r;
}

#define FOR_ALL_UBC(func)	\
	func(kmemsize)		\
	func(lockedpages)	\
	func(privvmpages)	\
	func(shmpages)		\
	func(numproc)		\
	func(physpages)		\
	func(vmguarpages)	\
	func(oomguarpages)	\
	func(numtcpsock)	\
	func(numflock)		\
	func(numpty)		\
	func(numsiginfo)	\
	func(tcpsndbuf)		\
	func(tcprcvbuf)		\
	func(othersockbuf)	\
	func(dgramrcvbuf)	\
	func(numothersock)	\
	func(dcachesize)	\
	func(numfile)		\
	func(numiptent)		\
	func(swappages)

/* Print functions */
static void print_na(struct Cveinfo *p, int index)
{
	p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%10s", "-");
}
static void print_eid(struct Cveinfo *p, int index)
{
	p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%36s",
		p->ctid);
}

static void print_uuid(struct Cveinfo *p, int index)
{
	int r;

	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%36s",
			p->uuid ? : "-");
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 36);
}

static void print_status(struct Cveinfo *p, int index)
{
	p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%-9s",
		ve_status(p->status));
}

static void print_laverage(struct Cveinfo *p, int index)
{
	if (p->cpustat == NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%14s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%1.2f/%1.2f/%1.2f",
			p->cpustat->loadavg[0],
			p->cpustat->loadavg[1],
			p->cpustat->loadavg[2]);
}

static void print_cpulimit(struct Cveinfo *p, int index)
{
	if (p->cpu == NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%7s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%7lu",
			p->cpu->limit[index]);
}

static void print_cpumask(struct Cveinfo *p, int index)
{
	if (p->cpumask != NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%16s", p->cpumask);
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%16s", "-");
}

static void print_nodemask(struct Cveinfo *p, int index)
{
	if (p->nodemask != NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%16s", p->nodemask);
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%16s", "-");
}

static void print_tm(struct Cveinfo *p, int index)
{
	if (p->tm == TM_EZ)
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%-2s", "EZ");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%-2s", "-");
}

static void print_yesno(const char *fmt, int i)
{
	p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
			fmt, i ? "yes" : "no");
}

static void print_onboot(struct Cveinfo *p, int index)
{
	print_yesno("%-6s", p->onboot);
}

static void print_bootorder(struct Cveinfo *p, int index)
{
	if (p->bootorder == NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10lu", p->bootorder[index]);
}

static void print_uptime(struct Cveinfo *p, int index)
{
	int r = 0;

	if (p->cpustat == NULL) {
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%15s", "-");
	} else {
		unsigned int days, hours, min, secs;

		days  = (unsigned int)(p->cpustat->uptime / (60 * 60 * 24));
		min = (unsigned int)(p->cpustat->uptime / 60);
		hours = min / 60;
		hours = hours % 24;
		min = min % 60;
		secs = (unsigned int)(p->cpustat->uptime -
				(60ull*min + 60ull*60*hours + 60ull*60*24*days));

		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%.3dd%.2dh:%.2dm:%.2ds",
				days, hours, min, secs);
	}
	p_outbuffer +=  r;
}

static void print_iolimit(struct Cveinfo *p, int index)
{
	if (p->io.limit == NULL || *p->io.limit == 0)
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10d", *p->io.limit);
}

static void print_iopslimit(struct Cveinfo *p, int index)
{
	if (p->io.iopslimit == NULL || *p->io.iopslimit == 0)
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10d", *p->io.iopslimit);
}

static void print_ioprio(struct Cveinfo *p, int index)
{
	if (p->io.prio == NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%6s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%6d", *p->io.prio);
}

static void print_features(struct Cveinfo *p, int index)
{
	if (p->features != NULL) {
		char buf[64] = "";

		features_mask2str(p->features->mask, p->features->known,
				",", buf, sizeof(buf));
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%-15s", buf);
	} else
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%-15s", "-");
}

static void print_layout(struct Cveinfo *p, int index)
{
	int r, layout = 0;

	if (p->ve_private != NULL)
		layout = vzctl_env_layout_version(p->ve_private);

	r = snprintf(p_outbuffer, e_buf-p_outbuffer, "%6d",
			layout);
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 0);

}

static void print_device(struct Cveinfo *p, int index)
{
	int r, layout = 0;
	char dev[64] = "";

	if (p->ve_private != NULL)
		layout = vzctl_env_layout_version(p->ve_private);

	if (layout == VZCTL_LAYOUT_5 && p->ve_root != NULL)
		ploop_get_partition_by_mnt(p->ve_root, dev, sizeof(dev));

	r = snprintf(p_outbuffer, e_buf-p_outbuffer, "%-16s", dev);
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 0);
}

static void print_ha_enable(struct Cveinfo *p, int index)
{
	print_yesno("%-9s", p->ha_enable);
}

static void print_ha_prio(struct Cveinfo *p, int index)
{
	if (p->ha_prio == NULL)
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10s", "-");
	else
		p_outbuffer += snprintf(p_outbuffer, e_buf-p_outbuffer,
				"%10lu", p->ha_prio[index]);
}

static void print_netfilter(struct Cveinfo *p, int index)
{
	int r;

	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-9s",
			p->netfilter ?: "-");
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 9);
}

static void print_devnodes(struct Cveinfo *p, int index)
{
	int r;
	char *tmp, *token;
	char *sptr;

	if (p->devnodes == NULL) {
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-16s", "-");
		SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 16);
		return;
	}

	tmp = strdup(p->devnodes);
	if ((token = strtok_r(tmp, " \t", &sptr)) != NULL) {
		do {
			r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%s ",
					token);
			SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 0);
		} while ((token = strtok_r(NULL, " \t", &sptr)));
	} else {
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-16s", "-");
		SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 16);
	}
	free(tmp);
}

#define PRINT_UBC(name)						\
static void print_ubc_ ## name(struct Cveinfo *p, int index)		\
{									\
	if (p->ubc == NULL ||						\
		(p->status != ENV_STATUS_RUNNING &&				\
			(index == 0 || index == 1 || index == 4)))	\
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%10s", "-");	\
	else								\
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%10lu",		\
					p->ubc->name[index]);		\
}									\

FOR_ALL_UBC(PRINT_UBC)

#define PRINT_DQ_RES(fn, res, name)					\
static void fn(struct Cveinfo *p, int index)				\
{									\
	if (p->res == NULL)						\
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%10s", "-");	\
	else								\
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer, "%10lu",		\
					p->res->name[index]);		\
}									\

PRINT_DQ_RES(print_dq_blocks, quota, quota_block)
PRINT_DQ_RES(print_dq_inodes, quota, quota_inode)

/* Sort functions */

static inline int check_empty_param(const void *val1, const void *val2)
{
	if (val1 == NULL && val2 == NULL)
		return 0;
	else if (val1 == NULL)
		return -1;
	else if (val2 == NULL)
		return 1;
	return 2;
}

static int none_sort_fn(const void *val1, const void *val2)
{
	return 0;
}

static int ostemplate_sort_fn(const void *val1, const void *val2)
{
	return strcmp(((struct Cveinfo*) val1)->ostmpl,
		((struct Cveinfo*) val2)->ostmpl);
}

static int laverage_sort_fn(const void *val1, const void *val2)
{
	struct vzctl_cpustat *v1 = ((struct Cveinfo *)val1)->cpustat;
	struct vzctl_cpustat *v2 = ((struct Cveinfo *)val2)->cpustat;
	int res;

	if ((res = check_empty_param(v1, v2)) != 2)
		return res;
	res = (v1->loadavg[0] - v2->loadavg[0]) * 100;
	if (res != 0)
		return res;
	res = (v1->loadavg[1] - v2->loadavg[1]) * 100;
	if (res != 0)
		return res;
	return (v1->loadavg[2] - v2->loadavg[2]) * 100;
}

static int uptime_sort_fn(const void *val1, const void *val2)
{
	struct vzctl_cpustat *v1 = ((struct Cveinfo *)val1)->cpustat;
	struct vzctl_cpustat *v2 = ((struct Cveinfo *)val2)->cpustat;
	int res;

	if ((res = check_empty_param(v1, v2)) != 2)
		return res;
	return (v2->uptime - v1->uptime);
}

static int tm_sort_fn(const void *val1, const void *val2)
{
	return (((struct Cveinfo*)val1)->tm > ((struct Cveinfo*)val2)->tm);
}

static int eid_sort_fn(const void *a, const void *b)
{
	return CMP_CTID(((struct Cveinfo*)a)->ctid, ((struct Cveinfo*)b)->ctid);
}

static int bootorder_sort_fn(const void *val1, const void *val2)
{
	int ret;
	unsigned long *r1 = ((struct Cveinfo*)val1)->bootorder;
	unsigned long *r2 = ((struct Cveinfo*)val2)->bootorder;

	ret = check_empty_param(r1, r2);
	switch (ret) {
		case 0: /* both NULL */
			return 0;
		case 2: /* both not NULL */
			break;
		default: /* one is NULL, other is not */
			return ret;
	}

	if (*r1 == *r2)
		return 0;

	return (*r1 > *r2);
}

static int ha_prio_sort_fn(const void *val1, const void *val2)
{
	int ret;
	unsigned long *r1 = ((struct Cveinfo*)val1)->ha_prio;
	unsigned long *r2 = ((struct Cveinfo*)val2)->ha_prio;

	if ((ret = check_empty_param(r1, r2)) == 2)
		return (*r1 < *r2);
	return (ret * -1);
}

static int status_sort_fn(const void *val1, const void *val2)
{
	int res;
	res = ((struct Cveinfo*)val1)->status-((struct Cveinfo*)val2)->status;
	if (!res)
		res = eid_sort_fn(val1, val2);
	return res;
}

#define SORT_STR_FN(fn, name)						\
static int fn(const void *val1, const void *val2)				\
{									\
	const char *h1 = ((struct Cveinfo*)val1)->name;			\
	const char *h2 = ((struct Cveinfo*)val2)->name;			\
	int ret;							\
	if ((ret = check_empty_param(h1, h2)) == 2)			\
		ret = strcmp(h1, h2);					\
	return ret;					\
}

SORT_STR_FN(hostnm_sort_fn, hostname)
SORT_STR_FN(name_sort_fn, name)
SORT_STR_FN(description_sort_fn, description)
SORT_STR_FN(ip_sort_fn, ip)

#define SORT_UL_RES(fn, type, res, name, index)				\
static int fn(const void *val1, const void *val2)				\
{									\
	struct type *r1 = ((struct Cveinfo *)val1)->res;		\
	struct type *r2 = ((struct Cveinfo *)val2)->res;		\
	int ret;							\
	if ((ret = check_empty_param(r1, r2)) == 2)			\
		ret = r1->name[index] > r2->name[index];		\
	return ret;							\
}

#define SORT_UBC(res)							\
SORT_UL_RES(res ## _h_sort_fn, Cubc, ubc, res, 0)			\
SORT_UL_RES(res ## _m_sort_fn, Cubc, ubc, res, 1)			\
SORT_UL_RES(res ## _l_sort_fn, Cubc, ubc, res, 2)			\
SORT_UL_RES(res ## _b_sort_fn, Cubc, ubc, res, 3)			\
SORT_UL_RES(res ## _f_sort_fn, Cubc, ubc, res, 4)

FOR_ALL_UBC(SORT_UBC)

SORT_UL_RES(dqblocks_u_sort_fn, Cquota, quota, quota_block, 0)
SORT_UL_RES(dqblocks_s_sort_fn, Cquota, quota, quota_block, 1)
SORT_UL_RES(dqblocks_h_sort_fn, Cquota, quota, quota_block, 2)

SORT_UL_RES(dqinodes_u_sort_fn, Cquota, quota, quota_inode, 0)
SORT_UL_RES(dqinodes_s_sort_fn, Cquota, quota, quota_inode, 1)
SORT_UL_RES(dqinodes_h_sort_fn, Cquota, quota, quota_inode, 2)

SORT_UL_RES(cpulimit_sort_fn, Ccpu, cpu, limit, 0)
SORT_UL_RES(cpulimitM_sort_fn, Ccpu, cpu, limit, 1)
SORT_UL_RES(cpuunits_sort_fn, Ccpu, cpu, limit, 2)

static int iolimit_sort_fn(const void *val1, const void *val2)
{
	unsigned *r1 = ((struct Cveinfo *)val1)->io.limit;
	unsigned *r2 = ((struct Cveinfo *)val2)->io.limit;
	int ret;

	if ((ret = check_empty_param(r1, r2)) == 2)
		return (*r1 < *r2);

	return (ret * -1);
}

static int ioprio_sort_fn(const void *val1, const void *val2)
{
	int *r1 = ((struct Cveinfo *)val1)->io.prio;
	int *r2 = ((struct Cveinfo *)val2)->io.prio;
	int ret;

	if ((ret = check_empty_param(r1, r2)) == 2)
		return (*r1 < *r2);
	return (ret * -1);
}

#define UBC_FIELD(name, header) \
{#name,      #header,      "%10s", 0, RES_UBC, print_ubc_ ## name, name ## _h_sort_fn}, \
{#name ".m", #header ".M", "%10s", 1, RES_UBC, print_ubc_ ## name, name ## _m_sort_fn}, \
{#name ".b", #header ".B", "%10s", 2, RES_UBC, print_ubc_ ## name, name ## _b_sort_fn}, \
{#name ".l", #header ".L", "%10s", 3, RES_UBC, print_ubc_ ## name, name ## _l_sort_fn}, \
{#name ".f", #header ".F", "%10s", 4, RES_UBC, print_ubc_ ## name, name ## _f_sort_fn}

static struct Cfield field_names[] =
{
/* veid should have index 0 */
{"ctid" , "CTID", "%36s",  0, RES_NONE, print_eid, eid_sort_fn},
{"private", "PRIVATE", "%-32s", 0, RES_NONE, print_ve_private, none_sort_fn},
{"root", "ROOT", "%-36s", 0, RES_NONE, print_ve_root, none_sort_fn},
/* veid is for backward compatibility  */
{"veid" , "CTID", "%36s",  0, RES_NONE, print_eid, eid_sort_fn},
{"uuid" , "UUID", "%36s",  0, RES_NONE, print_uuid, eid_sort_fn},
{"hostname", "HOSTNAME", "%-32s", 0, RES_HOSTNAME, print_hostname, hostnm_sort_fn},
{"name", "NAME", "%-32s", 0, RES_NAME, print_name, name_sort_fn},
{"smart_name", "NAME", "%-32s", 0, RES_NAME, print_smart_name, name_sort_fn},
{"description", "DESCRIPTION", "%-32s", 0, RES_DESCRIPTION, print_description, description_sort_fn},
{"tm", "TM", "%-2s", 0, RES_TM, print_tm, tm_sort_fn},
{"ostemplate", "OSTEMPLATE", "%-24s", 0, RES_OSTEMPLATE, print_ostemplate, ostemplate_sort_fn},
{"ip", "IP_ADDR", "%-15s", 0, RES_IP, print_ip, ip_sort_fn},
{"configured_ip", "IP_ADDR", "%-15s", 0, RES_CONFIGURED_IP, print_ip, ip_sort_fn},
{"status", "STATUS", "%-9s", 0, RES_STATUS, print_status, status_sort_fn},
/*	UBC	*/
UBC_FIELD(kmemsize, KMEMSIZE),
UBC_FIELD(lockedpages, LOCKEDP),
UBC_FIELD(privvmpages, PRIVVMP),
UBC_FIELD(shmpages, SHMP),
UBC_FIELD(numproc, NPROC),
UBC_FIELD(physpages, PHYSP),
UBC_FIELD(vmguarpages, VMGUARP),
UBC_FIELD(oomguarpages, OOMGUARP),
UBC_FIELD(numtcpsock, NTCPSOCK),
UBC_FIELD(numflock, NFLOCK),
UBC_FIELD(numpty, NPTY),
UBC_FIELD(numsiginfo, NSIGINFO),
UBC_FIELD(tcpsndbuf, TCPSNDB),
UBC_FIELD(tcprcvbuf, TCPRCVB),
UBC_FIELD(othersockbuf, OTHSOCKB),
UBC_FIELD(dgramrcvbuf, DGRAMRB),
UBC_FIELD(numothersock, NOTHSOCK),
UBC_FIELD(dcachesize, DCACHESZ),
UBC_FIELD(numfile, NFILE),
UBC_FIELD(numiptent, NIPTENT),
UBC_FIELD(swappages, SWAPP),

{"diskspace", "DQBLOCKS", "%10s", 0, RES_QUOTA, print_dq_blocks, dqblocks_u_sort_fn},
{"diskspace.s", "DQBLOCKS.S", "%10s", 1, RES_QUOTA, print_dq_blocks, dqblocks_s_sort_fn},
{"diskspace.h", "DQBLOCKS.H", "%10s", 2, RES_QUOTA, print_dq_blocks, dqblocks_h_sort_fn},

{"diskinodes", "DQINODES", "%10s", 0, RES_QUOTA, print_dq_inodes, dqinodes_u_sort_fn},
{"diskinodes.s", "DQINODES.S", "%10s", 1, RES_QUOTA, print_dq_inodes, dqinodes_s_sort_fn},
{"diskinodes.h", "DQINODES.H", "%10s", 2, RES_QUOTA, print_dq_inodes, dqinodes_h_sort_fn},

{"laverage", "LAVERAGE", "%14s", 0, RES_LA, print_laverage, laverage_sort_fn},

/* CPU */
{"cpulimit", "CPULIM", "%7s", 0, RES_CPU, print_cpulimit, cpulimit_sort_fn},
{"cpulimit%", "CPUL_%", "%7s", 0, RES_CPU, print_cpulimit, cpulimit_sort_fn},
{"cpulimitM", "CPUL_M", "%7s", 1, RES_CPU, print_cpulimit, cpulimitM_sort_fn},
{"cpuunits", "CPUUNI", "%7s", 2, RES_CPU, print_cpulimit, cpuunits_sort_fn},
{"cpumask", "CPUMASK", "%16s", 1, RES_CPU, print_cpumask, none_sort_fn},
{"nodemask", "NODEMASK", "%16s", 1, RES_CPU, print_nodemask, none_sort_fn},
/* SLM depricated */
{"slmmode", "SLMMOD", "%7s", 0, RES_NONE, print_na, none_sort_fn},
{"slminst", "SLMINST", "%10s", 1, RES_NONE, print_na, none_sort_fn},
{"slminst.l", "SLMINST.L", "%10s", 2, RES_NONE, print_na, none_sort_fn},
{"slmlowinst", "SLMLOWINST", "%10s", 3, RES_NONE, print_na, none_sort_fn},
{"slmlowinst.l", "SLMLOWINST.L", "%10s", 4, RES_NONE, print_na, none_sort_fn},
{"slmoomlimit", "SLMOOMLIMIT", "%10s", 5, RES_NONE, print_na, none_sort_fn},
{"slmavg", "SLMAVG", "%10s", 6, RES_NONE, print_na, none_sort_fn},
{"slmquality", "SLMQUALITY", "%10s", 7, RES_NONE, print_na, none_sort_fn},

/* veth */
{"host_ifname", "HOST_IFNAME", "%-16s", 1, RES_NETIF, print_netif_params, none_sort_fn},
{"mac", "MAC", "%-17s", 2, RES_NETIF, print_netif_params, none_sort_fn},
{"host_mac", "HOST_MAC", "%-17s", 3, RES_NETIF, print_netif_params, none_sort_fn},
{"gw", "GATEWAY", "%-15s", 4, RES_NETIF, print_netif_params, none_sort_fn},
{"network", "NETWORK", "%-32s", 5, RES_NETIF, print_netif_params, none_sort_fn},
{"ifname", "IFNAME", "%-8s", 6, RES_IFNAME, print_netif_params, none_sort_fn},
{"netif", "NET_INTERFACES", "%-32s", 0, RES_NETIF, print_dev_name, none_sort_fn},
{"onboot", "ONBOOT", "%-6s", 0, RES_ONBOOT, print_onboot, none_sort_fn},
{"bootorder", "BOOTORDER", "%10s", 0, RES_BOOTORDER,
	print_bootorder, bootorder_sort_fn},

{"uptime", "UPTIME", "%15s", 1, RES_UPTIME, print_uptime, uptime_sort_fn},
{"ioprio", "IOPRIO", "%6s", 0, RES_IO, print_ioprio, ioprio_sort_fn},
{"iolimit", "IOLIMIT", "%10s", 1, RES_IO, print_iolimit, iolimit_sort_fn},
{"iopslimit", "IOPSLIMIT", "%10s", 0, RES_IO, print_iopslimit, none_sort_fn},
{"features", "FEATURES", "%-15s", 0, RES_NONE, print_features, none_sort_fn},
{"layout", "LAYOUT", "%-6s", 0, RES_NONE, print_layout, none_sort_fn},
{"device", "DEVICE", "%-16s", 0, RES_NONE, print_device, none_sort_fn},

{"ha_enable", "HA_ENABLE", "%-9s", 0, RES_HA_ENABLE, print_ha_enable, none_sort_fn},
{"ha_prio", "HA_PRIO", "%10s", 0, RES_HA_PRIO, print_ha_prio, ha_prio_sort_fn},

{"netfilter", "NETFILTER", "%9s", 0, RES_NONE, print_netfilter, none_sort_fn},
{"devnodes", "DEVNODES", "%-16s", 0, RES_NONE, print_devnodes, none_sort_fn},
};

static void print_hostname(struct Cveinfo *p, int index)
{
	int r;
	char *str = "-";

	if (p->hostname != NULL)
		str = p->hostname;
	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-32s", str);
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 0);
}

static void print_name(struct Cveinfo *p, int index)
{
	int r;
	char *str = "-";
	int len;
	char *dst = NULL;

	if (p->name != NULL) {
		str = p->name;
		len = strlen(str) * 2 + 2;
		dst = malloc(len);
		if (utf8tostr(str, dst, len, NULL)) {
			free(dst);
			dst = NULL;
		}
	}
	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-32s",
		dst != NULL ? dst : str);
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 32);
	free(dst);
}

static void print_smart_name(struct Cveinfo *p, int index)
{
	int r;
	char *str = "-";
	int len;
	char *dst = NULL;

	if (p->name != NULL) {
		str = p->name;
		len = strlen(str) * 2 + 2;
		dst = malloc(len);
		if (utf8tostr(str, dst, len, NULL)) {
			free(dst);
			dst = NULL;
		}
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-32s",
				dst != NULL ? dst : str);
		free(dst);
	} else {
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-36s",
				p->ctid);
	}
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 32);
}

static void print_description(struct Cveinfo *p, int index)
{
	int r;
	char *str = "-";
	char *dst = NULL;
	int len;

	if (p->description != NULL) {
		str = p->description;
		len = strlen(str) * 2 + 2;
		dst = malloc(len);
		if (utf8tostr(str, dst, len, NULL)) {
			free(dst);
			dst = NULL;
		}
	}
	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-32s",
		dst != NULL ? dst : str);
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 32);
	free(dst);
}

static void print_ve_private(struct Cveinfo *p, int index)
{
	int r;

	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-32s",
		p->ve_private != NULL ? p->ve_private : "-");
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 32);
}

static void print_ve_root(struct Cveinfo *p, int index)
{
	int r;

	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-32s",
		p->ve_root != NULL ? p->ve_root : "-");
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 32);
}

static void print_ostemplate(struct Cveinfo *p, int index)
{
	int r;
	char *str = "-";

	if (p->ostmpl != NULL)
		str = p->ostmpl;
	r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-24s", str);
	SET_P_OUTBUFFER(r, e_buf - p_outbuffer, 24);
}

static int match_ip(char *ip_list, char *ip)
{
	char *tmp_list = strdup(ip_list);
	char *token = strtok(tmp_list, " ");
	int ret = 0;

	if (token == NULL)
		goto ret;
	do {
		if (!strcmp(token, ip)) {
			ret = 1;
			break;
		}
	} while ((token = strtok(NULL, " ")) != NULL);

ret:
	free(tmp_list);
	return ret;
}

static char *print_real_ip(struct Cveinfo *p, char *str, int dash)
{
	int r;
	char *ch, *end;
	int len;

	end = str + strlen(str);
	do {
		if ((ch = strchr(str, ' ')) != NULL)
			*ch++ = 0;
		else
			ch = end;
		if (p->ext_ip == NULL)
			break;
		if (!match_ip(p->ext_ip, str))
			break;
		str = ch;
	} while (str < end);
	if (str == end) {
		if (dash) {
			r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-15s ", "-");
			p_outbuffer += r;
		}
		return str;
	}
	len = strlen(str);
	if (last_field || len > 15)
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-s ", str);
	else
		r = snprintf(p_outbuffer, e_buf - p_outbuffer, "%-15s ", str);
	p_outbuffer += r;
	return (str + len + 1);
}

static void print_ip(struct Cveinfo *p, int index)
{
	int dash = 1;
	char *str;
	char *end, *bgn;

	if (p->ip != NULL)
		str = strdup(p->ip);
	else
		str = strdup("-");
	bgn = str;
	end = str + strlen(str);
	do {
		str = print_real_ip(p, str, dash);
		if (!last_field)
			break;
		dash = 0;

	} while (str < end);
	*(--p_outbuffer) = 0;

	free(bgn);
}

static void print_dev_name(struct Cveinfo *p, int index)
{
	veth_dev *dev;

	if (p->veth == NULL || list_empty(&p->veth->dev)) {
		p_outbuffer +=  snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%-32s", "-");
	} else {
		list_for_each(dev, &p->veth->dev, list) {
			if (netif_pattern != NULL && !dev->active)
				continue;
			p_outbuffer += snprintf(p_outbuffer,
				e_buf - p_outbuffer,
				"%s ", dev->dev_name);
			if (p_outbuffer >= e_buf)
				break;
		}
	}
}

static int print_netif_param(veth_dev *dev, int index, int blank)
{
	char *blank_fmt = "%s";
#define STR2MAC(dev)			\
	((unsigned char *)dev)[0],      \
	((unsigned char *)dev)[1],      \
	((unsigned char *)dev)[2],      \
	((unsigned char *)dev)[3],      \
	((unsigned char *)dev)[4],      \
	((unsigned char *)dev)[5]


	switch (index) {
	case 1:
		blank_fmt = "%-16s";
		if (blank)
			goto blank;
		if (dev->dev_name == NULL)
			return 0;
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%-16s", dev->dev_name);
		break;
	case 6:
		blank_fmt = "%-8s";
		if (blank)
			goto blank;
		if (dev->dev_name_ve == NULL)
			return 0;
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%-8s", dev->dev_name_ve);
		break;
	case 2:
		blank_fmt = "%-17s";
		if (blank)
			goto blank;
		if (dev->addrlen_ve == 0)
			return 0;
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%02X:%02X:%02X:%02X:%02X:%02X", STR2MAC(dev->mac_ve));
		break;
	case 3:
		blank_fmt = "%-17s";
		if (blank)
			goto blank;
		if (dev->addrlen == 0)
			return 0;
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%02X:%02X:%02X:%02X:%02X:%02X", STR2MAC(dev->mac));
		break;
	case 4:
		blank_fmt = "%-15s";
		if (blank)
			goto blank;
		if (dev->gw == 0)
			return 0;
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%-15s", dev->gw);
		break;
	case 5:
		blank_fmt = "%-32s";
		if (blank)
			goto blank;
		if (dev->network == NULL)
			return 0;
		p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
			"%-s", dev->network);
		break;
	}
	return 1;
blank:
	p_outbuffer += snprintf(p_outbuffer, e_buf - p_outbuffer,
		blank_fmt, "-");
	return 1;
}

static void print_netif_params(struct Cveinfo *p, int index)
{
	veth_dev *dev;
	int cnt;
	char *sp;

	cnt = 0;
	sp = p_outbuffer;
	if (p->veth == NULL || list_empty(&p->veth->dev))
		goto end;
	list_for_each(dev, &p->veth->dev, list) {
		if (netif_pattern != NULL && !dev->active)
			continue;
		cnt += print_netif_param(dev, index, 0);
		if (!last_field)
			break;
		if (cnt == 0)
			continue;
		/* skip spaces */
		while (sp < p_outbuffer && *(--p_outbuffer) == ' ');
		*(++p_outbuffer) = ' ';
		*(++p_outbuffer) = 0;
		if (p_outbuffer >= e_buf)
			break;
	}
end:
	if (cnt == 0)
		print_netif_param(dev, index, 1);
	return;
}

static void *x_malloc(int size)
{
	void *p;
	if ((p = malloc(size)) == NULL) {
		fprintf(stderr, "Error: unable to allocate %d bytes\n", size);
		exit(1);
	}
	return p;
}

static void *x_realloc(void *ptr, int size)
{
	void *tmp;

	if ((tmp = realloc(ptr, size)) == NULL) {
		fprintf(stderr, "Error: unable to allocate %d bytes\n", size);
		exit(1);
	}
	return tmp;
}

static void usage(void)
{
	fprintf(stderr,
"Usage: vzlist	[-a|-S] [-n] [-H] [-o field[,field...]] [-s [-]field]\n"
"		[-h <pattern>] [-N <pattern>] [-d <pattern>] [-i <ifname>]\n"
"		[CTID [CTID ...]|-1]\n"
"       vzlist -L\n"
"\n"
"   -a, -all		list all Containers\n"
"   -S, --stopped	list stopped Containers\n"
"   -n, --name		display Container names instead of hostnames\n"
"   -H, --no-header	suppress displaying the header\n"
"   -o, --output		display only specified fields\n"
"   -s, --sort		sort by the field ('-field' to reverse sort order)\n"
"   -h, -hostname	filter by hostname pattern\n"
"   -N, --name_filter	filter by name pattern\n"
"   -d, --description	filter by description pattern\n"
"   -i, --netif		filter by network interface name\n"
"   -L, --list		get the list of known fields\n");
}

static int ve_search_fn(const void* a, const void* b)
{
	return CMP_CTID((char *)a, ((struct Cveinfo*)b)->ctid);
}

static int eid_search_fn(const void* a, const void* b)
{
	return CMP_CTID((const char *)a,  (const char *)b);
}

static char* trim_eol_space(char *sp, char *ep)
{
	ep--;
	while (isspace(*ep) && ep >= sp) *ep-- = '\0';

	return sp;
}

static void print_hdr(void)
{
	struct Cfield_order *p;
	int f;

	for (p = g_field_order; p != NULL; p = p->next) {
		f = p->order;
		if (p->next == NULL && field_names[f].hdr_fmt[1] == '-') {
			printf("%s", field_names[f].hdr);
			break;
		}
		printf(field_names[f].hdr_fmt, field_names[f].hdr);
		printf(" ");
	}
	printf("\n");
}

/*
   1 - match
   0 - do not match
*/
static inline int check_pattern(char *str, char *pat)
{
	if (pat == NULL)
		return 1;
	if (str == NULL)
		return 0;
	return !fnmatch(pat, str, 0);
}

static void filter_by_hostname(void)
{
	int i;

	for (i = 0; i < n_veinfo; i++) {
		if (!check_pattern(veinfo[i].hostname, host_pattern))
			veinfo[i].hide = 1;
	}
}

static void filter_by_name(void)
{
	int i;

	for (i = 0; i < n_veinfo; i++) {
		if (!check_pattern(veinfo[i].name, name_pattern))
			veinfo[i].hide = 1;
	}
}

static void filter_by_description(void)
{
	int i;

	for (i = 0; i < n_veinfo; i++) {
		if (!check_pattern(veinfo[i].description, desc_pattern))
			veinfo[i].hide = 1;
	}
}

static char *get_veth_ip_str(list_head_t *h)
{
	veth_ip *ip;
	char *buf, *sp;
	int len = 0;

	if (list_empty(h))
		return NULL;
	list_for_each(ip, h, list) {
		len += strlen(ip->ip) + 1;
	}
	sp = buf = x_malloc(len + 1);
	list_for_each(ip, h, list) {
		sp += sprintf(sp, "%s ", ip->ip);
	}
	return buf;
}

static int is_match_netif_pattern(char *str)
{
	if (netif_pattern == NULL)
		return 1;
	if (!strcmp(netif_pattern, str))
		return 1;
	return 0;
}

static void update_ip(struct Cveinfo *ve)
{
	veth_dev *dev;
	int total, len;
	char *tmp;

	/* Get IPs from Container */
	if (ve->status == ENV_STATUS_RUNNING &&	check_param(RES_IP))
	{
		tmp = get_real_ips(ve->ctid);
		if (tmp != NULL) {
			free(ve->ip);
			ve->ip = tmp;
			return;
		}
	}
	if (netif_pattern != NULL && is_match_netif_pattern("venet0"))
		return;
	if (ve->veth == NULL || list_empty(&ve->veth->dev))
		return;
	if (netif_pattern != NULL) {
		free(ve->ip);
		ve->ip = NULL;
	}
	total = 0;
	if (ve->ip != NULL)
		total = strlen(ve->ip);
	list_for_each(dev, &ve->veth->dev, list) {
		if (is_match_netif_pattern(dev->dev_name) ||
				is_match_netif_pattern(dev->dev_name_ve))
		{
			tmp = get_veth_ip_str(&dev->ip);
			if (tmp == NULL)
				continue;
			len = strlen(tmp);
			ve->ip = x_realloc(ve->ip, total + len + 2);
			strcpy(ve->ip + total, tmp);
			total += len + 1;
			ve->ip[total] = 0;
			free(tmp);
		}
	}
}

static void filter_by_netif(void)
{
	int i;
	veth_dev *dev;

	for (i = 0; i < n_veinfo; i++) {
		veinfo[i].hide = 1;
		if (netif_pattern != NULL &&
				is_match_netif_pattern("venet0") &&
				veinfo[i].ip != NULL)
		{
			veinfo[i].hide = 0;
			continue;
		}

		if (veinfo[i].veth == NULL ||
		    list_empty(&veinfo[i].veth->dev))
		{
			continue;
		}
		list_for_each(dev, &veinfo[i].veth->dev, list) {
			if (is_match_netif_pattern(dev->dev_name) ||
			    is_match_netif_pattern(dev->dev_name_ve))
			{
				dev->active = 1;
				veinfo[i].hide = 0;
			}
		}
	}
}

static void print_ve(void)
{
	struct Cfield_order *p;
	int i, f, idx;

	/* If sort order != veid (already sorted by) */
	if (g_sort_field) {
		qsort(veinfo, n_veinfo, sizeof(struct Cveinfo),
			field_names[g_sort_field].sort_fn);
	}
	if (!(veid_only || !show_hdr))
		print_hdr();
	for (i = 0; i < n_veinfo; i++) {
		if (sort_rev)
			idx = n_veinfo - i - 1;
		else
			idx = i;
		if (veinfo[idx].hide)
			continue;
		if (only_stopped_ve && veinfo[idx].status == ENV_STATUS_RUNNING)
			continue;
		last_field = 0;
		for (p = g_field_order; p != NULL; p = p->next) {
			f = p->order;
			if (p->next == NULL)
				last_field = 1;
			field_names[f].print_fn(&veinfo[idx],
						field_names[f].index);
			if (p_outbuffer >= e_buf)
				break;;
			if (p->next != NULL)
				*p_outbuffer++ = ' ';
		}
		puts(trim_eol_space(g_outbuffer, p_outbuffer));
		g_outbuffer[0] = 0;
		p_outbuffer = g_outbuffer;
	}
}

static void add_elem(struct Cveinfo *ve)
{
	veinfo = (struct Cveinfo *)x_realloc(veinfo,
				sizeof(struct Cveinfo) * ++n_veinfo);
	memcpy(&veinfo[n_veinfo - 1], ve, sizeof(struct Cveinfo));
	return;
}

static inline struct Cveinfo *find_ve(ctid_t ctid)
{
	return (struct Cveinfo *) bsearch(ctid, veinfo, n_veinfo,
			sizeof(struct Cveinfo), ve_search_fn);
}

void update_ve(ctid_t ctid, char *ip, int status)
{
	struct Cveinfo *tmp, ve;

	tmp = find_ve(ctid);
	if (tmp == NULL) {
		memset(&ve, 0, sizeof(struct Cveinfo));
		SET_CTID(ve.ctid, ctid)
		ve.status = status;
		ve.ip = ip;
		add_elem(&ve);
		qsort(veinfo, n_veinfo, sizeof(struct Cveinfo), eid_sort_fn);
		return;
	} else {
		if (tmp->ip == NULL)
			tmp->ip = ip;
		else
			free(ip);
		tmp->status = status;
	}
	return;
}

void update_ubc(ctid_t ctid, const struct Cubc *ubc)
{
	struct Cveinfo *tmp;

	if ((tmp = find_ve(ctid)) == NULL)
		return;

	if (tmp->status != ENV_STATUS_RUNNING)
		return;

	if (tmp->ubc == NULL)
		tmp->ubc = x_malloc(sizeof(*ubc));

	memcpy(tmp->ubc, ubc, sizeof(*ubc));
}

static int fill_cpulimit(struct vzctl_cpulimit_param *c, struct Ccpu *cpu)
{
	static struct vzctl_cpuinfo info;

	if (vzctl2_get_cpuinfo(&info))
		return -1;

	if (c->type == VZCTL_CPULIMIT_PCT) {
		cpu->limit[0] = c->limit;
		cpu->limit[1] = info.freq * c->limit / (100 * 1000 * info.ncpu);
	} else {
		cpu->limit[0] = round(100.0 * c->limit * 1000 * info.ncpu / info.freq);
		cpu->limit[1] = c->limit;
	}

	return 0;
}

static const char *nf2str(unsigned nf)
{
	switch(nf) {
	case VZCTL_NF_DISABLED:
		return "disabled";
	case VZCTL_NF_STATELESS:
		return "stateless";
	case VZCTL_NF_STATEFUL:
		return "stateful";
	case VZCTL_NF_FULL:
		return "full";
	default:
		return "-";
	}
}

void merge_ub(struct vzctl_env_handle *h, int id, unsigned long *ub)
{
	struct vzctl_2UL_res res;

	if (vzctl2_env_get_ub_resource(vzctl2_get_env_param(h), id, &res) == 0) {
		ub[2] = res.b;
		ub[3] = res.l;
	}
}

#define MERGE_UB(id, name)	\
	merge_ub(h, id, ve->ubc->name);

#define FOR_ALL_UB(func)       \
        func(VZCTL_PARAM_KMEMSIZE, kmemsize)	\
        func(VZCTL_PARAM_LOCKEDPAGES, lockedpages)	\
        func(VZCTL_PARAM_PRIVVMPAGES, privvmpages)	\
        func(VZCTL_PARAM_SHMPAGES, shmpages)	\
        func(VZCTL_PARAM_NUMPROC, numproc)		\
        func(VZCTL_PARAM_PHYSPAGES, physpages)	\
        func(VZCTL_PARAM_VMGUARPAGES, vmguarpages)	\
        func(VZCTL_PARAM_OOMGUARPAGES, oomguarpages)\
        func(VZCTL_PARAM_NUMTCPSOCK, numtcpsock)	\
        func(VZCTL_PARAM_NUMFLOCK, numflock)	\
        func(VZCTL_PARAM_NUMPTY, numpty)		\
        func(VZCTL_PARAM_NUMSIGINFO, numsiginfo)	\
        func(VZCTL_PARAM_TCPSNDBUF, tcpsndbuf)	\
        func(VZCTL_PARAM_TCPRCVBUF, tcprcvbuf)	\
        func(VZCTL_PARAM_OTHERSOCKBUF, othersockbuf)\
        func(VZCTL_PARAM_DGRAMRCVBUF, dgramrcvbuf)	\
        func(VZCTL_PARAM_NUMOTHERSOCK, numothersock)\
        func(VZCTL_PARAM_DCACHESIZE, dcachesize)	\
        func(VZCTL_PARAM_NUMFILE, numfile)		\
        func(VZCTL_PARAM_NUMIPTENT, numiptent)	\
        func(VZCTL_PARAM_SWAPPAGES, swappages)

static void merge_conf(struct Cveinfo *ve, struct vzctl_env_handle *h)
{
	char buf[STR_SIZE];
	const char *p;
	unsigned long ul;
	unsigned u;
	int i, enable;

	if (ve->ubc == NULL) {
		ve->ubc = x_malloc(sizeof(struct Cubc));
		memset(ve->ubc, 0, sizeof(struct Cubc));
		FOR_ALL_UB(MERGE_UB)
	}

#if 0
	if (ve->ext_ip == NULL && param->ext_ipadd != NULL) {
		memset(buf, 0, sizeof(buf));
		List_genstr(param->ext_ipadd, buf, sizeof(buf));
		ve->ext_ip = strdup(buf);
	}
#endif
	vzctl_veth_dev_iterator itdev = NULL;
	while ((itdev = vzctl2_env_get_veth(vzctl2_get_env_param(h), itdev)) != NULL) {
		struct vzctl_veth_dev_param dev_param = {};
		struct veth_dev dev = {};

		list_head_init(&dev.ip);

		if (ve->veth == NULL) {
			ve->veth = x_malloc(sizeof(struct veth_param));
			list_head_init(&ve->veth->dev);
		}

		vzctl_ip_iterator it = NULL;
		while ((it = vzctl2_env_get_veth_ipaddress(itdev, it)) != NULL) {
			if (vzctl2_env_get_ipstr(it, buf, sizeof(buf)) == 0) {
				struct vzctl_ip_param ip;

				if (parse_ip(buf, &ip.ip, &ip.mask))
					add_veth_ip(&dev.ip, &ip, ADD);
			}
		}

		vzctl2_env_get_veth_param(itdev, &dev_param, sizeof(dev_param));
		if (dev_param.mac) {
			memcpy(dev.mac, dev_param.mac, ETH_ALEN);
			dev.addrlen = ETH_ALEN;
		}
		if (dev_param.mac_ve) {
			memcpy(dev.mac_ve, dev_param.mac_ve, ETH_ALEN);
			dev.addrlen_ve = ETH_ALEN;
		}
		if (dev_param.dev_name)
			strncpy(dev.dev_name, dev_param.dev_name,
						sizeof(dev.dev_name)-1);
		if (dev_param.dev_name_ve)
			strncpy(dev.dev_name_ve, dev_param.dev_name_ve,
						sizeof(dev.dev_name_ve)-1);
		if (dev_param.gw)
			dev.gw = strdup(dev_param.gw);
		if (dev_param.gw6)
			dev.gw6 = strdup(dev_param.gw6);
		if (dev_param.network)
			dev.network = strdup(dev_param.network);

		add_veth_param(ve->veth, &dev);
	}

	if (ve->ip == NULL) {
		LIST_HEAD(ip);

                vzctl_ip_iterator it = NULL;
		while ((it = vzctl2_env_get_ipaddress(vzctl2_get_env_param(h), it)) != NULL) {
			char *p;

			if (vzctl2_env_get_ipstr(it, buf, sizeof(buf)))
				continue;

			p = strrchr(buf, '/');
			if (p != NULL)
				*p = '\0';
			add_str_param(&ip, buf);
		}

		ve->ip = list2str(NULL, &ip);
		free_str(&ip);
	}

	struct vzctl_2UL_res res;
	ve->quota = x_malloc(sizeof(struct Cquota));
	memset(ve->quota, 0, sizeof(*ve->quota));
	if (vzctl2_env_get_diskspace(vzctl2_get_env_param(h), &res) == 0) {
		ve->quota->quota_block[1] = res.b;
		ve->quota->quota_block[2] = res.l;
	}
	if (vzctl2_env_get_diskinodes(vzctl2_get_env_param(h), &res) == 0) {
		ve->quota->quota_inode[1] = res.b;
		ve->quota->quota_inode[2] = res.l;
	}

	struct vzctl_cpulimit_param c;
	ve->cpu = x_malloc(sizeof(struct Ccpu));
	memset(ve->cpu, 0, sizeof(struct Ccpu));

	if (vzctl2_env_get_cpulimit(vzctl2_get_env_param(h), &c) == 0)
		fill_cpulimit(&c, ve->cpu);

	if (vzctl2_env_get_cpuunits(vzctl2_get_env_param(h), &ul) == 0)
		 ve->cpu->limit[2] = ul;

	if (vzctl2_env_get_hostname(vzctl2_get_env_param(h), &p) == 0)
		ve->hostname = strdup(p);

	if (vzctl2_env_get_name(h, &p) == 0)
		ve->name = strdup(p);

	if (vzctl2_env_get_description(vzctl2_get_env_param(h), &p) == 0)
		ve->description = strdup(p);

	if (vzctl2_env_get_ve_root_path(vzctl2_get_env_param(h), &p) == 0)
		ve->ve_root = strdup(p);

	if (vzctl2_env_get_ve_private_path(vzctl2_get_env_param(h), &p) == 0)
		ve->ve_private = strdup(p);

	if (vzctl2_env_get_ostemplate(vzctl2_get_env_param(h), &p) == 0) {
		ve->tm = TM_EZ;
		ve->ostmpl = strdup(p);
	} else {
		ve->tm = TM_UNKNOWN;
		ve->ostmpl = strdup("-");
	}

	enable = 0;
	if (vzctl2_env_get_autostart(vzctl2_get_env_param(h), &enable) == 0)
		ve->onboot = enable;

	if (vzctl2_env_get_bootorder(vzctl2_get_env_param(h), &ul) == 0) {
		ve->bootorder = x_malloc(sizeof(*ve->bootorder));
		*ve->bootorder = ul;
	}

	if (vzctl2_env_get_iolimit(vzctl2_get_env_param(h), &u) == 0) {
		ve->io.limit = x_malloc(sizeof(unsigned));
		*ve->io.limit = u;
	}

	if (vzctl2_env_get_iopslimit(vzctl2_get_env_param(h), &u) == 0) {
		ve->io.iopslimit = x_malloc(sizeof(unsigned));
		*ve->io.iopslimit = u;
	}

	if (vzctl2_env_get_ioprio(vzctl2_get_env_param(h), &i) == 0) {
		ve->io.prio = x_malloc(sizeof(int));
		*ve->io.prio = i;
	}

	if (vzctl2_env_get_cpumask(vzctl2_get_env_param(h), buf, sizeof(buf)) == 0)
		ve->cpumask = strdup(buf);

	if (vzctl2_env_get_nodemask(vzctl2_get_env_param(h), buf, sizeof(buf)) == 0)
		ve->nodemask = strdup(buf);

	struct vzctl_feature_param f;
	if (vzctl2_env_get_features(vzctl2_get_env_param(h), &f) == 0 &&
			(f.on | f.off)) 
	{
		ve->features = x_malloc(sizeof(struct vzctl_env_features));
		ve->features->known = f.on | f.off;
		ve->features->mask = f.on;
	}

	/* by default High Availability should be enabled */
	ve->ha_enable = 1;
	if (vzctl2_env_get_ha_enable(vzctl2_get_env_param(h), &i) == 0)
		ve->ha_enable = i;

	if (vzctl2_env_get_ha_prio(vzctl2_get_env_param(h), &ul) == 0) {
		ve->ha_prio = x_malloc(sizeof(*ve->ha_prio));
		*ve->ha_prio = ul;
	}

	if (vzctl2_env_get_netfilter(vzctl2_get_env_param(h), &u) == 0)
		ve->netfilter = strdup(nf2str(u));

	if (vzctl2_env_get_uuid(vzctl2_get_env_param(h), &p) == 0 && p != NULL)
		ve->uuid = strdup(p);

	if (vzctl2_env_get_param(h, "DEVNODES",  &p) == 0 && p != NULL)
		ve->devnodes = strdup(p);
}

static void parse_conf(ctid_t ctid, struct Cveinfo *ve)
{
	int err;
	struct vzctl_env_handle *h;

	h = vzctl2_env_open(ctid, VZCTL_CONF_RUNTIME_PARAM, &err);
	if (h == NULL)
		return;

	merge_conf(ve, h);
	update_ip(ve);

	vzctl2_env_close(h);
}

static void read_ves_param(void)
{
	int i;

	for (i = 0; i < n_veinfo; i++) {
		if (EMPTY_CTID(veinfo[i].ctid))
			continue;
		parse_conf(veinfo[i].ctid, &veinfo[i]);
	}
}

static int check_veid_restr(ctid_t ctid)
{
	if (g_ve_list == NULL)
		return 1;
	return (bsearch(ctid, g_ve_list, n_ve_list,
			sizeof(ctid_t), eid_search_fn) != NULL);
}

#define UPDATE_UBC(param)			\
	if (!strcmp(name, #param)) {		\
		ubc.param[0] = held;		\
		ubc.param[1] = maxheld;		\
		ubc.param[2] = barrier;		\
		ubc.param[3] = limit;		\
		ubc.param[4] = failcnt;		\
	}

static int get_ub(ctid_t ctid)
{
	char buf[256];
	unsigned long held, maxheld, barrier, limit, failcnt;
	char name[32];
	FILE *fp;
	struct Cubc ubc = {};

	snprintf(buf, sizeof(buf), "/proc/bc/%s/resources", ctid);
	if ((fp = fopen(buf, "r")) == NULL)
		return 1;

	while (!feof(fp)) {
               if (fgets(buf, sizeof(buf), fp) == NULL)
                       break;

		if (sscanf(buf, "%s %lu %lu %lu %lu %lu",
			name, &held, &maxheld, &barrier, &limit, &failcnt) != 6)
		{
			continue;
		}
		FOR_ALL_UBC(UPDATE_UBC)
	}
	if (check_veid_restr(ctid))
		update_ubc(ctid, &ubc);
	fclose(fp);

	return 0;
}

static void get_ubs(void)
{
	int i;

	for (i = 0; i < n_veinfo; i++)
		if (check_veid_restr(veinfo[i].ctid))
			get_ub(veinfo[i].ctid);
}

char *invert_ip(char *ips)
{
	char *tmp, *p, *ep, *tp;
	int len;
	if (ips == NULL)
		return NULL;
	len = strlen(ips);
	tp = tmp = x_malloc(len + 2);
	tmp[0] = 0;
	p = ep = ips + len - 1;
	while (p > ips) {
		/* Skip spaces */
		while (p > ips && isspace(*p)) {--p;}
		ep = p;
		/* find the string begin from */
		while (p > ips && !isspace(*(p - 1))) { --p;}
		snprintf(tp, ep - p + 3, "%s ", p);
		tp += ep - p + 2;
		--p;
	}
	return tmp;
}

static int _get_run_ve(int update)
{
	FILE *fp;
	struct Cveinfo ve;
	char buf[MAX_STR];
	char ips[MAX_STR];
	int res, classid, nproc;
	ctid_t id, ctid = {};

	if ((fp = fopen(PROCVEINFO, "r")) == NULL) {
		if (errno == ENOENT)
			return 0;
		fprintf(stderr, "Unable to open %s: %s\n",
				PROCVEINFO, strerror(errno));
		return 1;
	}
	memset(&ve, 0, sizeof(struct Cveinfo));
	while (!feof(fp)) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		res = sscanf(buf, "%37s %d %d %[^\n]",
			id, &classid, &nproc, ips);
		if (res < 3 || vzctl2_parse_ctid(id, ctid) || EMPTY_CTID(ctid))
			continue;
		if (!check_veid_restr(ctid) || strcmp(ctid, "0") == 0)
			continue;
		memset(&ve, 0, sizeof(struct Cveinfo));
		if (res == 4) {
			ve.ip = invert_ip(ips);
		} else if (res == 3)
			ve.ip = strdup("");
		SET_CTID(ve.ctid, ctid);
		ve.status = ENV_STATUS_RUNNING;
		if (update)
			update_ve(ctid, ve.ip, ve.status);
		else
			add_elem(&ve);
	}
	if (!update)
		qsort(veinfo, n_veinfo, sizeof(struct Cveinfo), eid_sort_fn);
	fclose(fp);

	return 0;
}

static char *get_real_ips(ctid_t ctid)
{
#if 0
	struct vznetwork_info *ni;
	struct vznetif_info **ii;
	vznetfilter_t fr;
	char **p;
	char *ip, *sp, *ep;
	unsigned int mask;
	char buf[4096];
	int flags = VZNETIFFLD_IFACE | VZNETIFFLD_ACTUAL_ADDRLIST;

	flags |= VZNETIFFLD_ACTUAL_ADDR6LIST;

	buf[0] = 0;
	fr = vznet_get_simple_filter(VZNETIFTYPE_ALL,
					NULL, NULL, NULL, veid, veid);
	ni = vznet_get_info(flags, fr);
	vznet_release_filter(fr);
	if (ni == NULL || ni->info == NULL)
		return strdup("");
	sp = buf; ep = buf + sizeof(buf);
	for (ii = ni->info; *ii; ii++) {
		if ((*ii)->flags != VZNETIFFLAG_UP)
			continue;
		for (p = (*ii)->actual_addrlist; p != NULL && *p != NULL; p++) {
			if (!is_match_netif_pattern((*ii)->iface))
				continue;
			if (!parse_ip(*p, &ip, &mask)) {
				if (strcmp(ip, "127.0.0.1") == 0 ||
				    strcmp(ip, "::1") == 0 ||
				    strcmp(ip, "::2") == 0 ||
				    strncmp(ip, "fe80:", 5) == 0)
					continue;
				sp += snprintf(sp, ep - sp, "%s ", ip);
				free(ip);
			}
		}
	}
	vznet_release_info(ni);
	return *buf != 0 ? strdup(buf) :  NULL;
#endif
	return 0;
}

static int get_quota_stat(void)
{
	int i;
	char buf[PATH_MAX];
	struct ploop_info info;

	for (i = 0; i < n_veinfo; i++) {
		/*FIXME: get root disk path */
		snprintf(buf, sizeof(buf), "%s/root.hdd/DiskDescriptor.xml", veinfo[i].ve_private);
		if (ploop_get_info_by_descr(buf, &info))
			continue;

		if (veinfo[i].quota == NULL) {
			veinfo[i].quota = x_malloc(sizeof(struct Cquota));
			memset(veinfo[i].quota, 0, sizeof(struct Cquota));
		}

		veinfo[i].quota->quota_block[0] = info.fs_bsize * (info.fs_blocks - info.fs_bfree) / 1024;
		veinfo[i].quota->quota_inode[0] = info.fs_inodes - info.fs_ifree;
	}
	return 0;
}

static int get_ves_la(void)
{
#if 0
	int i, ret;

	for (i = 0; i < n_veinfo; i++) {
		if (veinfo[i].hide)
			continue;
		veinfo[i].cpustat = x_malloc(sizeof(struct vzctl_cpustat));

		ret = vzctl_env_cpustat(veinfo[i].veid, veinfo[i].cpustat,
			sizeof(struct vzctl_cpustat));
		if (ret != 0) {
			free(veinfo[i].cpustat);
			veinfo[i].cpustat = NULL;
		}
	}
	vzctl_close();
#endif
	return 0;
}

static int get_ve_list(void)
{
	DIR *dp;
	struct dirent *ep;
	struct Cveinfo ve;
	int mask;

	dp = opendir(VZ_ENV_CONF_DIR);
	if (dp == NULL) {
		return -1;
	}
	memset(&ve, 0, sizeof(struct Cveinfo));
	mask = g_skip_owner ? ENV_SKIP_OWNER | ENV_STATUS_EXISTS : ENV_STATUS_EXISTS;
	while ((ep = readdir (dp))) {
		struct vzctl_env_status status = {};
		ctid_t id, ctid;
		char str[6];

		if (sscanf(ep->d_name, "%37[^.].%5s", id, str) != 2 ||
				strcmp(str, "conf"))
			continue;
		if (vzctl2_parse_ctid(id, ctid))
			continue;
		if (!check_veid_restr(ctid))
			continue;

		mask = check_param(RES_STATUS) ? ENV_STATUS_ALL :
							ENV_STATUS_EXISTS;
		if (vzctl2_get_env_status(ctid, &status, mask))
			continue;
		if (!(status.mask & ENV_STATUS_EXISTS))
			continue;
		SET_CTID(ve.ctid, ctid)
		ve.status = status.mask;
		add_elem(&ve);
	}
	closedir(dp);
	if (veinfo != NULL)
		qsort(veinfo, n_veinfo, sizeof(struct Cveinfo), eid_sort_fn);
	return 0;
}

static int update_ves_io_info(void)
{
#if 0
	int limit, i;
	for (i = 0; i < n_veinfo; i++) {
		if (veinfo[i].status != ENV_STATUS_RUNNING)
			continue;
		if (vzctl_get_iolimit(veinfo[i].ctid, &limit) == 0) {
			if (veinfo[i].io.limit == NULL)
				veinfo[i].io.limit = x_malloc(sizeof(int));
			veinfo[i].io.limit[0] = limit;
		}
		if (vzctl_get_iopslimit(veinfo[i].ctid, &limit) == 0) {
			if (veinfo[i].io.iopslimit == NULL)
				veinfo[i].io.iopslimit = x_malloc(sizeof(int));
			veinfo[i].io.iopslimit[0] = limit;
		}
	}
#endif
	return 0;
}

static int search_field(char *name)
{
	int i;

	if (name == NULL)
		return -1;
	for (i = 0; i < sizeof(field_names) / sizeof(*field_names); i++) {
		if (!strcmp(name, field_names[i].name))
			return i;
	}
	return -1;
}

static int build_field_order(char *fields)
{
	struct Cfield_order *tmp, *prev = NULL;
	char *sp, *ep, *p;
	char name[32];
	int order, nm_len;

	sp = fields;
	if (fields == NULL) {
		if (g_compat_mode)
			sp = default_compat_field_order;
		else if (netif_pattern != NULL)
			sp = default_netif_field_order;
		else
			sp = default_field_order;
	}
	ep = sp + strlen(sp);
	do {
		if ((p = strchr(sp, ',')) == NULL)
			p = ep;
		nm_len = p - sp + 1;
		if (nm_len > sizeof(name) - 1) {
			fprintf(stderr, "Field name %s is unknown.\n", sp);
			return 1;
		}
		snprintf(name, nm_len, "%s", sp);
		sp = p + 1;
		if (g_compat_mode &&
		    (!strcmp(name, "veid") || !strcmp(name, "ctid")))
			strcpy(name, "uuid");
		if ((order = search_field(name)) < 0) {
			fprintf(stderr, "Unknown field: %s\n", name);
			return 1;
		}
		tmp = x_malloc(sizeof(struct Cfield_order));
		tmp->order = order;
		tmp->next = NULL;
		if (prev == NULL)
			g_field_order = tmp;
		else
			prev->next = tmp;
		prev = tmp;
	} while (sp < ep);
	return 0;
}

static inline int check_param(int res_type)
{
	struct Cfield_order *p;

	for (p = g_field_order; p != NULL; p = p->next) {
		if (field_names[p->order].res_type == res_type)
			return 1;
	}
	return 0;
}

static void do_filter(void)
{
	if (host_pattern != NULL)
		filter_by_hostname();
	if (name_pattern != NULL)
		filter_by_name();
	if (desc_pattern != NULL)
		filter_by_description();
	if (netif_pattern != NULL)
		filter_by_netif();
}

static int collect(void)
{
	int update = 0;

	vzctl_open();

	if (all_ve || g_ve_list != NULL || only_stopped_ve) {
		get_ve_list();
		update = 1;
	}
	_get_run_ve(update);
	if (!only_stopped_ve)
		get_ubs();
	/* No Container found */
	if (!n_veinfo) {
		if (g_ve_list != NULL) {
			fprintf(stderr, "Container is not found\n");
			return 1;
		}
		return 0;
	}
	if (check_param(RES_LA) || check_param(RES_UPTIME))
		get_ves_la();
	read_ves_param();
	if (check_param(RES_IO))
		update_ves_io_info();
	if (check_param(RES_QUOTA))
		get_quota_stat();
	do_filter();

	return 0;
}

static void print_names(void)
{
	int i;

	for (i = 0; i < sizeof(field_names) / sizeof(*field_names); i++) {
		if (!strncmp(field_names[i].name, "slm", 3))
			continue;
		printf("%-15s %-15s\n", field_names[i].name,
					field_names[i].hdr);
	}
}

static void free_veinfo(void)
{
	int i;

	for (i = 0; i < n_veinfo; i++) {
		free(veinfo[i].ip);
		free(veinfo[i].ext_ip);
		free(veinfo[i].hostname);
		free(veinfo[i].name);
		free(veinfo[i].description);
		free(veinfo[i].ubc);
		free(veinfo[i].quota);
		free(veinfo[i].cpu);
		free(veinfo[i].ve_root);
		free(veinfo[i].ve_private);
		free(veinfo[i].ostmpl);
		free(veinfo[i].bootorder);
		free(veinfo[i].io.limit);
		free(veinfo[i].io.prio);
		free(veinfo[i].cpumask);
		free(veinfo[i].nodemask);
		free(veinfo[i].features);
		free(veinfo[i].ha_prio);
		free(veinfo[i].uuid);
	}
}

static struct option list_options[] =
{
	{"no-header",	no_argument, NULL, 'H'},
	{"stopped",	no_argument, NULL, 'S'},
	{"all",		no_argument, NULL, 'a'},
	{"name",	no_argument, NULL, 'n'},
	{"name_filter",	required_argument, NULL, 'N'},
	{"hostname",	required_argument, NULL, 'h'},
	{"description",	required_argument, NULL, 'd'},
	{"output",	required_argument, NULL, 'o'},
	{"sort",	required_argument, NULL, 's'},
	{"list",	no_argument, NULL, 'L'},
	{"netif",	required_argument, NULL, 'i'},
	{"help",	no_argument, NULL, 'e'},
	{"compat",	no_argument, NULL, 'c'},
	{"skipowner",	no_argument, NULL, 'w'},
	{ NULL, 0, NULL, 0 }
};

int main(int argc, char **argv)
{
	int ret;
	char *f_order = NULL;
	char *p;
	ctid_t ctid;
	int c, len;

	while (1) {
		int option_index = -1;
		c = getopt_long(argc, argv, "HSanN:h:d:o:s:i:Le1cw",
			list_options, &option_index);
		if (c == -1)
			break;

		switch(c) {
		case 'S'	:
			only_stopped_ve = 1;
			break;
		case 'H'	:
			show_hdr = 0;
			break;
		case 'L'	:
			print_names();
			return 0;
		case 'a'	:
			all_ve = 1;
			break;
		case 'h'	:
			host_pattern = strdup(optarg);
			break;
		case 'd'	:
			len = strlen(optarg) * 2 + 2;
			desc_pattern = malloc(len);
			if (vzctl_convertstr(optarg, desc_pattern, len))
				strcpy(desc_pattern, optarg);
			break;
		case 'o'	:
			f_order = strdup(optarg);
			break;
		case 's'	:
			p = optarg;
			if (p[0] == '-') {
				p++;
				sort_rev = 1;
			}
			if ((g_sort_field = search_field(p)) < 0) {
				fprintf(stderr, "%s is an invalid field name"
					" for this query.\n", optarg);
				return 1;
			}
			break;
		case '1'	:
			f_order = strdup("veid");
			veid_only = 1;
			break;
		case 'n'	:
			f_order = strdup(default_nm_field_order);
			with_names = 1;
			break;
		case 'i'	:
			netif_pattern = strdup(optarg);
			break;
		case 'N'	:
			len = strlen(optarg) * 2 + 2;
			name_pattern = malloc(len);
			if (vzctl_convertstr(optarg, name_pattern, len))
				strcpy(name_pattern, optarg);
			break;
		case 'c'	:
			g_compat_mode = 1;
			break;
		case 'w'	:
			g_skip_owner = 1;
			break;
		default		:
			usage();
			return 1;
		}
	}
	if (optind < argc) {
		while (optind < argc) {
			char name[STR_SIZE];

			if (vzctl_convertstr(argv[optind], name, sizeof(name)) ||
					vzctl2_get_envid_by_name(name, ctid) < 0)
			{
				fprintf(stderr, "Container ID %s is invalid.\n",
						argv[optind]);
				return 1;
			}
			optind++;
			g_ve_list = x_realloc(g_ve_list, sizeof(ctid_t) * ++n_ve_list);
			char *a = g_ve_list + (sizeof(ctid_t) * (n_ve_list -1));
			SET_CTID(a, ctid)
		}
		qsort(g_ve_list, n_ve_list, sizeof(ctid_t), eid_search_fn);
	}
	if (build_field_order(f_order))
		return 1;
	if (getuid()) {
		fprintf(stderr, "This program can be run by root only.\n");
		return 1;
	}
	vzctl_set_log_enable(0);	/* Disable messages from lib */
	if ((ret = collect()))
		return ret;

	print_ve();
	free_veinfo();
	return 0;
}
